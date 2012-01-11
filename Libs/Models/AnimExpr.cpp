/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "AnimExpr.hpp"
#include "Model_cmdl.hpp"


AnimExpressionT::AnimExpressionT(const CafuModelT& Model)
    : m_Model(Model),
      m_RefCount(0),
      m_ChangeNum(0)
{
    UpdateChangeNum();
}


void AnimExpressionT::UpdateChangeNum()
{
    static unsigned int s_ChangeCount=0;

    m_ChangeNum = ++s_ChangeCount;
}


/*************************/
/*** AnimExprStandardT ***/
/*************************/

AnimExprStandardT::AnimExprStandardT(const CafuModelT& Model, int SequNr, float FrameNr)
    : AnimExpressionT(Model),
      m_SequNr(SequNr),
      m_FrameNr(FrameNr)
{
    NormalizeInput();
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
        const int    Frame_0=int(m_FrameNr);                                        // If m_FrameNr == 17.83, then Frame_0 == 17
        const float  Frame_f=m_FrameNr-Frame_0;                                     //                             Frame_f ==  0.83
        const int    Frame_1=(Frame_0+1>=int(Anim.Frames.Size())) ? 0 : Frame_0+1;  //                             Frame_1 == 18

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

        // Interpolate the position and quaternion according to the fraction Frame_f.
        Weight=1.0f;
        Pos   =Data_0[0]*(1.0f-Frame_f) + Data_1[0]*Frame_f;
        Quat  =slerp(cf::math::QuaternionfT::FromXYZ(Data_0[1]), cf::math::QuaternionfT::FromXYZ(Data_1[1]), Frame_f);
        Scale =Data_0[2]*(1.0f-Frame_f) + Data_1[2]*Frame_f;
    }
}


void AnimExprStandardT::AdvanceTime(float Time, bool ForceLoop)
{
    // TODO: Beachte korrekte Wrap-Regeln für mit loopen und ohne.
    // TODO: Loops (next vs. ForceLoop) richtig behandeln
    const ArrayT<CafuModelT::AnimT>& Anims=GetModel().GetAnims();

    if (m_SequNr<0 || m_SequNr>=int(Anims.Size())) { SetFrameNr(0.0f); return; }
    if (Anims[m_SequNr].Frames.Size()<=1) { SetFrameNr(0.0f); return; }

    const float NumFrames=float(Anims[m_SequNr].Frames.Size());

    float FrameNr=m_FrameNr + Time*Anims[m_SequNr].FPS;

    if (ForceLoop)
    {
        // Wrap the sequence (it's a looping (repeating) sequence, like idle, walk, ...).
        FrameNr=fmod(FrameNr, NumFrames);
        if (FrameNr<0.0f) FrameNr+=NumFrames;
    }
    else
    {
        // Clamp the sequence (it's a play-once (non-repeating) sequence, like dying).
        // On clamping, stop the sequence 1/100th sec before the end of the last frame.
        if (FrameNr>=NumFrames-1.0f) FrameNr=NumFrames-1.0f-0.01f;
        if (FrameNr<0.0f) FrameNr=0.0f;
    }

    SetFrameNr(FrameNr);
}


IntrusivePtrT<AnimExpressionT> AnimExprStandardT::Clone() const
{
    return GetModel().GetAnimExprPool().GetStandard(m_SequNr, m_FrameNr);
}


bool AnimExprStandardT::IsEqual(const IntrusivePtrT<AnimExpressionT>& AE) const
{
    AnimExprStandardT* Other=dynamic_cast<AnimExprStandardT*>(AE.get());

    if (!Other) return false;
    return m_SequNr==Other->m_SequNr && m_FrameNr==Other->m_FrameNr;
}


void AnimExprStandardT::SetSequNr(int SequNr)
{
    if (m_SequNr==SequNr) return;

    m_SequNr=SequNr;
    NormalizeInput();

    UpdateChangeNum();
}


void AnimExprStandardT::SetFrameNr(float FrameNr)
{
    if (m_FrameNr==FrameNr) return;

    m_FrameNr=FrameNr;
    NormalizeInput();

    UpdateChangeNum();
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


AnimExprFilterT::AnimExprFilterT(const CafuModelT& Model, IntrusivePtrT<AnimExpressionT> SubExpr, unsigned int ChannelNr)
    : AnimExpressionT(Model),
      m_SubExpr(SubExpr),
      m_ChannelNr(ChannelNr)
{
}


AnimExprFilterT::AnimExprFilterT(const CafuModelT& Model, IntrusivePtrT<AnimExpressionT> SubExpr, const std::string& ChannelName)
    : AnimExpressionT(Model),
      m_SubExpr(SubExpr),
      m_ChannelNr(FindChannelByName(Model, ChannelName))
{
}


void AnimExprFilterT::ReInit(IntrusivePtrT<AnimExpressionT> SubExpr, unsigned int ChannelNr)
{
    if (m_SubExpr==SubExpr && m_ChannelNr==ChannelNr) return;

    m_SubExpr  =SubExpr;
    m_ChannelNr=ChannelNr;

    UpdateChangeNum();
}


unsigned int AnimExprFilterT::GetChangeNum() const
{
    return std::max(AnimExpressionT::GetChangeNum(), m_SubExpr->GetChangeNum());
}


void AnimExprFilterT::GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const
{
    Weight=0.0f;

    if (m_ChannelNr >= GetModel().GetChannels().Size() || GetModel().GetChannels()[m_ChannelNr].IsMember(JointNr))
    {
        m_SubExpr->GetData(JointNr, Weight, Pos, Quat, Scale);
    }
}


IntrusivePtrT<AnimExpressionT> AnimExprFilterT::Clone() const
{
    return GetModel().GetAnimExprPool().GetFilter(m_SubExpr->Clone(), m_ChannelNr);
}


bool AnimExprFilterT::IsEqual(const IntrusivePtrT<AnimExpressionT>& AE) const
{
    AnimExprFilterT* Other=dynamic_cast<AnimExprFilterT*>(AE.get());

    if (!Other) return false;
    return m_ChannelNr==Other->m_ChannelNr && m_SubExpr->IsEqual(Other->m_SubExpr);
}


/************************/
/*** AnimExprCombineT ***/
/************************/

AnimExprCombineT::AnimExprCombineT(const CafuModelT& Model, IntrusivePtrT<AnimExpressionT> A, IntrusivePtrT<AnimExpressionT> B)
    : AnimExpressionT(Model),
      m_A(A),
      m_B(B)
{
}


void AnimExprCombineT::ReInit(IntrusivePtrT<AnimExpressionT> A, IntrusivePtrT<AnimExpressionT> B)
{
    if (m_A==A && m_B==B) return;

    m_A=A;
    m_B=B;

    UpdateChangeNum();
}


unsigned int AnimExprCombineT::GetChangeNum() const
{
    return std::max(AnimExpressionT::GetChangeNum(),
                    std::max(m_A->GetChangeNum(), m_B->GetChangeNum()));
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


void AnimExprCombineT::AdvanceTime(float Time, bool ForceLoop)
{
    m_A->AdvanceTime(Time, ForceLoop);
    m_B->AdvanceTime(Time, ForceLoop);
}


IntrusivePtrT<AnimExpressionT> AnimExprCombineT::Clone() const
{
    return GetModel().GetAnimExprPool().GetCombine(m_A->Clone(), m_B->Clone());
}


bool AnimExprCombineT::IsEqual(const IntrusivePtrT<AnimExpressionT>& AE) const
{
    AnimExprCombineT* Other=dynamic_cast<AnimExprCombineT*>(AE.get());

    if (!Other) return false;
    return m_A->IsEqual(Other->m_A) && m_B->IsEqual(Other->m_B);
}


/**********************/
/*** AnimExprBlendT ***/
/**********************/

AnimExprBlendT::AnimExprBlendT(const CafuModelT& Model, IntrusivePtrT<AnimExpressionT> A, IntrusivePtrT<AnimExpressionT> B, float Duration)
    : AnimExpressionT(Model),
      m_A(A),
      m_B(B),
      m_Duration(Duration),
      m_Frac(0.0f)
{
}


void AnimExprBlendT::ReInit(IntrusivePtrT<AnimExpressionT> A, IntrusivePtrT<AnimExpressionT> B, float Duration)
{
    m_A=A;
    m_B=B;
    m_Duration=Duration;
    m_Frac=0.0f;

    UpdateChangeNum();
}


unsigned int AnimExprBlendT::GetChangeNum() const
{
    if (m_Frac >= 1.0f)
    {
        return std::max(AnimExpressionT::GetChangeNum(), m_B->GetChangeNum());
    }

    return std::max(AnimExpressionT::GetChangeNum(),
                    std::max(m_A->GetChangeNum(), m_B->GetChangeNum()));
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


void AnimExprBlendT::AdvanceTime(float Time, bool ForceLoop)
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
        m_A->AdvanceTime(Time, ForceLoop);
    }
    else
    {
        m_A=NULL;   // m_A is unused now that m_Frac >= 1.0.
    }

    m_B->AdvanceTime(Time, ForceLoop);


    UpdateChangeNum();
}


IntrusivePtrT<AnimExpressionT> AnimExprBlendT::Clone() const
{
    IntrusivePtrT<AnimExprBlendT> Blend=GetModel().GetAnimExprPool().GetBlend(m_A->Clone(), m_B->Clone(), m_Duration);

    Blend->m_Frac=m_Frac;
    return Blend;
}


bool AnimExprBlendT::IsEqual(const IntrusivePtrT<AnimExpressionT>& AE) const
{
    AnimExprBlendT* Other=dynamic_cast<AnimExprBlendT*>(AE.get());

    if (!Other) return false;
    return m_Frac==Other->m_Frac && m_Duration==Other->m_Duration && m_A->IsEqual(Other->m_A) && m_B->IsEqual(Other->m_B);
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

            return m_Standard[AENr];
        }
    }

    IntrusivePtrT<AnimExprStandardT> NewAE(new AnimExprStandardT(m_Model, SequNr, FrameNr));
    m_Standard.PushBack(NewAE);

    return NewAE;
}


IntrusivePtrT<AnimExprFilterT> AnimExprPoolT::GetFilter(IntrusivePtrT<AnimExpressionT> SubExpr, unsigned int ChannelNr)
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


IntrusivePtrT<AnimExprFilterT> AnimExprPoolT::GetFilter(IntrusivePtrT<AnimExpressionT> SubExpr, const std::string& ChannelName)
{
    return GetFilter(SubExpr, FindChannelByName(m_Model, ChannelName));
}


IntrusivePtrT<AnimExprCombineT> AnimExprPoolT::GetCombine(IntrusivePtrT<AnimExpressionT> A, IntrusivePtrT<AnimExpressionT> B)
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


IntrusivePtrT<AnimExprBlendT> AnimExprPoolT::GetBlend(IntrusivePtrT<AnimExpressionT> A, IntrusivePtrT<AnimExpressionT> B, float Duration)
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
