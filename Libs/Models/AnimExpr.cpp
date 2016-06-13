/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "AnimExpr.hpp"
#include "Model_cmdl.hpp"


AnimExpressionT::AnimExpressionT(const CafuModelT& Model)
    : m_Model(Model),
      m_RefCount(0)
{
}


/*************************/
/*** AnimExprStandardT ***/
/*************************/

AnimExprStandardT::AnimExprStandardT(const CafuModelT& Model, int SequNr, float FrameNr)
    : AnimExpressionT(Model),
      m_SequNr(SequNr),
      m_FrameNr(FrameNr),
      m_ForceLoop(false)
{
    NormalizeInput();
}


void AnimExprStandardT::GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const
{
    typedef CafuModelT::JointT JointT;
    typedef CafuModelT::AnimT  AnimT;

    const ArrayT<JointT>& Joints=GetModel().GetJoints();
    const ArrayT<AnimT>&  Anims =GetModel().GetAnims();

    if (m_SequNr==-1)
    {
        // Don't animate, just use the bind pose defined in the model file.
        const JointT& J=Joints[JointNr];

        Weight=1.0f;
        Pos   =J.Pos;
        Quat  =cf::math::QuaternionfT::FromXYZ(J.Qtr);
        Scale =J.Scale;
    }
    else
    {
        // m_SequNr is a valid index into Anims -- use it.
        const AnimT& Anim=Anims[m_SequNr];
        const int    Frame_0=int(m_FrameNr);                        // If m_FrameNr == 17.83, then Frame_0 == 17
        const float  Frame_f=m_FrameNr-Frame_0;                     //                             Frame_f ==  0.83
        const int    Frame_1=(Frame_0+1) % int(Anim.Frames.Size()); //                             Frame_1 == 18

        const AnimT::AnimJointT& AJ=Anim.AnimJoints[JointNr];

        Vector3fT Data_0[3]={ AJ.DefaultPos, AJ.DefaultQtr, AJ.DefaultScale };
        Vector3fT Data_1[3]={ AJ.DefaultPos, AJ.DefaultQtr, AJ.DefaultScale };

        // Determine the position, quaternion and scale for Frame_0 and Frame_1.
        unsigned int FlagCount=0;

        for (int i=0; i<9; i++)
        {
            if ((AJ.Flags >> i) & 1)
            {
                Data_0[i/3][i % 3]=Anim.Frames[Frame_0].AnimData[AJ.FirstDataIdx+FlagCount];
                Data_1[i/3][i % 3]=Anim.Frames[Frame_1].AnimData[AJ.FirstDataIdx+FlagCount];

                FlagCount++;
            }
        }

        if (Frame_1==0 && Frame_f<0.001f)
        {
            // This is most likely the end of a non-wrapping sequence.
            Weight=1.0f;
            Pos   =Data_0[0];
            Quat  =cf::math::QuaternionfT::FromXYZ(Data_0[1]);
            Scale =Data_0[2];
        }
        else
        {
            // Interpolate the position and quaternion according to the fraction Frame_f.
            Weight=1.0f;
            Pos   =Data_0[0]*(1.0f-Frame_f) + Data_1[0]*Frame_f;
            Quat  =slerp(cf::math::QuaternionfT::FromXYZ(Data_0[1]), cf::math::QuaternionfT::FromXYZ(Data_1[1]), Frame_f);
            Scale =Data_0[2]*(1.0f-Frame_f) + Data_1[2]*Frame_f;
        }
    }
}


bool AnimExprStandardT::AdvanceTime(float Time)
{
    const ArrayT<CafuModelT::AnimT>& Anims = GetModel().GetAnims();

    if (m_SequNr < 0 || m_SequNr >= int(Anims.Size())) { SetFrameNr(0.0f); return false; }
    if (Anims[m_SequNr].Frames.Size() <= 1) { SetFrameNr(0.0f); return false; }

    const float NumFrames = float(Anims[m_SequNr].Frames.Size());
    const int   Next      = m_ForceLoop ? m_SequNr : Anims[m_SequNr].Next;
    float       FrameNr   = m_FrameNr + Time*Anims[m_SequNr].FPS;
    bool        Wrapped   = false;

    if (Next < 0)
    {
        // This is a play-once (non-repeating) sequence, there is no "next" sequence.
        // Thus clamp the frame number to NumFrames-1.
        if (FrameNr >= NumFrames - 1.0f)
        {
            FrameNr = NumFrames - 1.0f;
            if (m_FrameNr < FrameNr) Wrapped = true;
        }
    }
    else
    {
        // There is a "next" sequence following this.
        if (FrameNr >= NumFrames)
        {
            FrameNr -= NumFrames;
            Wrapped = true;

            /* Calling SetSequNr(Next) below would technically be the right thing to do to
             * progress to the next sequence.
             * However, this is not supported in AnimExprStandardT for several reasons:
             *   - Backwards-compatibility. See class documentation for details.
             *   - Requires changes to GetData(). The implementation as-is is not sufficient.
             */
            // SetSequNr(Next);
        }
    }

    SetFrameNr(FrameNr);
    return Wrapped;
}


AnimExpressionPtrT AnimExprStandardT::Clone() const
{
    IntrusivePtrT<AnimExprStandardT> StdAE=GetModel().GetAnimExprPool().GetStandard(m_SequNr, m_FrameNr);

    StdAE->SetForceLoop(m_ForceLoop);
    return StdAE;
}


bool AnimExprStandardT::IsEqual(const AnimExpressionPtrT& AE) const
{
    AnimExprStandardT* Other=dynamic_cast<AnimExprStandardT*>(AE.get());

    if (!Other) return false;
    return m_SequNr==Other->m_SequNr && m_FrameNr==Other->m_FrameNr && m_ForceLoop==Other->m_ForceLoop;
}


void AnimExprStandardT::SetSequNr(int SequNr)
{
    if (m_SequNr==SequNr) return;

    m_SequNr=SequNr;
    NormalizeInput();
}


void AnimExprStandardT::SetFrameNr(float FrameNr)
{
    if (m_FrameNr==FrameNr) return;

    m_FrameNr=FrameNr;
    NormalizeInput();
}


void AnimExprStandardT::SetForceLoop(bool ForceLoop)
{
    m_ForceLoop=ForceLoop;
}


void AnimExprStandardT::NormalizeInput()
{
    const ArrayT<CafuModelT::AnimT>& Anims=GetModel().GetAnims();

    // m_SequNr==-1 means "use the bind pose from the model file only (no anim)".
    if (m_SequNr < -1) m_SequNr = -1;
    if (m_SequNr >= int(Anims.Size())) m_SequNr = -1;
    if (m_SequNr != -1 && (Anims[m_SequNr].FPS<0.0 || Anims[m_SequNr].Frames.Size()==0)) m_SequNr = -1;

    m_FrameNr=std::max(m_FrameNr, 0.0f);
    m_FrameNr=(m_SequNr==-1) ? 0.0f : fmod(m_FrameNr, float(Anims[m_SequNr].Frames.Size()));
}


/***********************/
/*** AnimExprFilterT ***/
/***********************/

namespace
{
    unsigned int FindChannelByName(const CafuModelT& Model, const std::string& ChannelName)
    {
        unsigned int ChannelNr;

        for (ChannelNr=0; ChannelNr<Model.GetChannels().Size(); ChannelNr++)
            if (ChannelName == Model.GetChannels()[ChannelNr].Name)
                return ChannelNr;

        return ChannelNr;
    }
}


AnimExprFilterT::AnimExprFilterT(const CafuModelT& Model, AnimExpressionPtrT SubExpr, unsigned int ChannelNr)
    : AnimExpressionT(Model),
      m_SubExpr(SubExpr),
      m_ChannelNr(ChannelNr)
{
}


AnimExprFilterT::AnimExprFilterT(const CafuModelT& Model, AnimExpressionPtrT SubExpr, const std::string& ChannelName)
    : AnimExpressionT(Model),
      m_SubExpr(SubExpr),
      m_ChannelNr(FindChannelByName(Model, ChannelName))
{
}


void AnimExprFilterT::ReInit(AnimExpressionPtrT SubExpr, unsigned int ChannelNr)
{
    if (m_SubExpr==SubExpr && m_ChannelNr==ChannelNr) return;

    m_SubExpr  =SubExpr;
    m_ChannelNr=ChannelNr;
}


void AnimExprFilterT::GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const
{
    Weight=0.0f;

    if (m_ChannelNr >= GetModel().GetChannels().Size() || GetModel().GetChannels()[m_ChannelNr].IsMember(JointNr))
    {
        m_SubExpr->GetData(JointNr, Weight, Pos, Quat, Scale);
    }
}


AnimExpressionPtrT AnimExprFilterT::Clone() const
{
    return GetModel().GetAnimExprPool().GetFilter(m_SubExpr->Clone(), m_ChannelNr);
}


bool AnimExprFilterT::IsEqual(const AnimExpressionPtrT& AE) const
{
    AnimExprFilterT* Other=dynamic_cast<AnimExprFilterT*>(AE.get());

    if (!Other) return false;
    return m_ChannelNr==Other->m_ChannelNr && m_SubExpr->IsEqual(Other->m_SubExpr);
}


/************************/
/*** AnimExprCombineT ***/
/************************/

AnimExprCombineT::AnimExprCombineT(const CafuModelT& Model, AnimExpressionPtrT A, AnimExpressionPtrT B)
    : AnimExpressionT(Model),
      m_A(A),
      m_B(B)
{
}


void AnimExprCombineT::ReInit(AnimExpressionPtrT A, AnimExpressionPtrT B)
{
    if (m_A==A && m_B==B) return;

    m_A=A;
    m_B=B;
}


void AnimExprCombineT::GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const
{
    m_A->GetData(JointNr, Weight, Pos, Quat, Scale);

    float                  WeightB;
    Vector3fT              PosB;
    cf::math::QuaternionfT QuatB;
    Vector3fT              ScaleB;

    // Pick the expression with the largest weight.
    m_B->GetData(JointNr, WeightB, PosB, QuatB, ScaleB);

    if (WeightB > Weight)
    {
        Weight = WeightB;
        Pos    = PosB;
        Quat   = QuatB;
        Scale  = ScaleB;
    }
}


bool AnimExprCombineT::AdvanceTime(float Time)
{
    const bool WrapA = m_A->AdvanceTime(Time);
    const bool WrapB = m_B->AdvanceTime(Time);

    return WrapA || WrapB;
}


AnimExpressionPtrT AnimExprCombineT::Clone() const
{
    return GetModel().GetAnimExprPool().GetCombine(m_A->Clone(), m_B->Clone());
}


bool AnimExprCombineT::IsEqual(const AnimExpressionPtrT& AE) const
{
    AnimExprCombineT* Other=dynamic_cast<AnimExprCombineT*>(AE.get());

    if (!Other) return false;
    return m_A->IsEqual(Other->m_A) && m_B->IsEqual(Other->m_B);
}


/**********************/
/*** AnimExprBlendT ***/
/**********************/

AnimExprBlendT::AnimExprBlendT(const CafuModelT& Model, AnimExpressionPtrT A, AnimExpressionPtrT B, float Duration)
    : AnimExpressionT(Model),
      m_A(A),
      m_B(B),
      m_Duration(Duration),
      m_Frac(0.0f)
{
}


void AnimExprBlendT::ReInit(AnimExpressionPtrT A, AnimExpressionPtrT B, float Duration)
{
    m_A=A;
    m_B=B;
    m_Duration=Duration;
    m_Frac=0.0f;
}


void AnimExprBlendT::GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const
{
    float                  w[2];
    Vector3fT              p[2];
    cf::math::QuaternionfT q[2];
    Vector3fT              s[2];

    if (m_Frac <= 0.0f)
    {
        m_A->GetData(JointNr, Weight, Pos, Quat, Scale);
        return;
    }

    if (m_Frac >= 1.0f)
    {
        m_B->GetData(JointNr, Weight, Pos, Quat, Scale);
        return;
    }

    m_A->GetData(JointNr, w[0], p[0], q[0], s[0]);
    m_B->GetData(JointNr, w[1], p[1], q[1], s[1]);

    const float f0 = 1.0f - m_Frac;
    const float f1 = m_Frac;

    Weight = w[0]*f0 + w[1]*f1;
    Pos    = p[0]*f0 + p[1]*f1;
    Quat   = slerp(q[0], q[1], f1);   // slerp() is why we cannot have generic "add" and "mul" AnimExpressionT's.
    Scale  = s[0]*f0 + s[1]*f1;
}


bool AnimExprBlendT::AdvanceTime(float Time)
{
    // Advance the blend fraction.
    if (m_Duration < 0.001f)
    {
        m_Frac = 1.0f;
    }
    else
    {
        m_Frac += Time/m_Duration;
    }

    m_Frac = std::min(m_Frac, 1.0f);


    // Advance the sub-expressions.
    if (m_Frac < 1.0f)
    {
        m_A->AdvanceTime(Time);
    }
    else
    {
        m_A=NULL;   // m_A is unused now that m_Frac >= 1.0.
    }

    return m_B->AdvanceTime(Time);
}


AnimExpressionPtrT AnimExprBlendT::Clone() const
{
    IntrusivePtrT<AnimExprBlendT> Blend=GetModel().GetAnimExprPool().GetBlend(
        m_A==NULL ? NULL : m_A->Clone(), m_B->Clone(), m_Duration);

    Blend->m_Frac=m_Frac;
    return Blend;
}


bool AnimExprBlendT::IsEqual(const AnimExpressionPtrT& AE) const
{
    AnimExprBlendT* Other=dynamic_cast<AnimExprBlendT*>(AE.get());

    if (!Other) return false;
    return m_Frac==Other->m_Frac && m_Duration==Other->m_Duration &&
           m_B->IsEqual(Other->m_B) &&
           (m_A==NULL ? Other->m_A==NULL : m_A->IsEqual(Other->m_A));
}


/*********************/
/*** AnimExprPoolT ***/
/*********************/

void AnimExprPoolT::FlattenUnused()
{
    for (unsigned int AENr=0; AENr<m_Filter.Size(); AENr++)
        if (m_Filter[AENr]->GetRefCount()==1)
            m_Filter[AENr]->ReInit(NULL, 0);

    for (unsigned int AENr=0; AENr<m_Combine.Size(); AENr++)
        if (m_Combine[AENr]->GetRefCount()==1)
            m_Combine[AENr]->ReInit(NULL, NULL);

    for (unsigned int AENr=0; AENr<m_Blend.Size(); AENr++)
        if (m_Blend[AENr]->GetRefCount()==1)
            m_Blend[AENr]->ReInit(NULL, NULL, 0.0f);
}


IntrusivePtrT<AnimExprStandardT> AnimExprPoolT::GetStandard(int SequNr, float FrameNr)
{
    FlattenUnused();

    for (unsigned int AENr=0; AENr<m_Standard.Size(); AENr++)
    {
        if (m_Standard[AENr]->GetRefCount()==1)
        {
            m_Standard[AENr]->SetSequNr(SequNr);
            m_Standard[AENr]->SetFrameNr(FrameNr);
            m_Standard[AENr]->SetForceLoop(false);

            return m_Standard[AENr];
        }
    }

    IntrusivePtrT<AnimExprStandardT> NewAE(new AnimExprStandardT(m_Model, SequNr, FrameNr));
    m_Standard.PushBack(NewAE);

    return NewAE;
}


IntrusivePtrT<AnimExprFilterT> AnimExprPoolT::GetFilter(AnimExpressionPtrT SubExpr, unsigned int ChannelNr)
{
    FlattenUnused();

    for (unsigned int AENr=0; AENr<m_Filter.Size(); AENr++)
    {
        if (m_Filter[AENr]->GetRefCount()==1)
        {
            m_Filter[AENr]->ReInit(SubExpr, ChannelNr);

            return m_Filter[AENr];
        }
    }

    IntrusivePtrT<AnimExprFilterT> NewAE(new AnimExprFilterT(m_Model, SubExpr, ChannelNr));
    m_Filter.PushBack(NewAE);

    return NewAE;
}


IntrusivePtrT<AnimExprFilterT> AnimExprPoolT::GetFilter(AnimExpressionPtrT SubExpr, const std::string& ChannelName)
{
    return GetFilter(SubExpr, FindChannelByName(m_Model, ChannelName));
}


IntrusivePtrT<AnimExprCombineT> AnimExprPoolT::GetCombine(AnimExpressionPtrT A, AnimExpressionPtrT B)
{
    FlattenUnused();

    for (unsigned int AENr=0; AENr<m_Combine.Size(); AENr++)
    {
        if (m_Combine[AENr]->GetRefCount()==1)
        {
            m_Combine[AENr]->ReInit(A, B);

            return m_Combine[AENr];
        }
    }

    IntrusivePtrT<AnimExprCombineT> NewAE(new AnimExprCombineT(m_Model, A, B));
    m_Combine.PushBack(NewAE);

    return NewAE;
}


IntrusivePtrT<AnimExprBlendT> AnimExprPoolT::GetBlend(AnimExpressionPtrT A, AnimExpressionPtrT B, float Duration)
{
    FlattenUnused();

    for (unsigned int AENr=0; AENr<m_Blend.Size(); AENr++)
    {
        if (m_Blend[AENr]->GetRefCount()==1)
        {
            m_Blend[AENr]->ReInit(A, B, Duration);

            return m_Blend[AENr];
        }
    }

    IntrusivePtrT<AnimExprBlendT> NewAE(new AnimExprBlendT(m_Model, A, B, Duration));
    m_Blend.PushBack(NewAE);

    return NewAE;
}
