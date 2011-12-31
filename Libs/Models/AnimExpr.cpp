/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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


/*************************/
/*** AnimExprStandardT ***/
/*************************/

AnimExprStandardT::AnimExprStandardT(const CafuModelT& Model, int SequNr, float FrameNr)
    : AnimExpressionT(Model),
      m_SequNr(SequNr),
      m_FrameNr(FrameNr),
      m_ChangeCount(1)      // The user code inits its own count with 0.
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


void AnimExprStandardT::SetSequNr(int SequNr)
{
    if (m_SequNr==SequNr) return;

    m_SequNr=SequNr;
    NormalizeInput();

    m_ChangeCount++;
}


void AnimExprStandardT::SetFrameNr(float FrameNr)
{
    if (m_FrameNr==FrameNr) return;

    m_FrameNr=FrameNr;
    NormalizeInput();

    m_ChangeCount++;
}
