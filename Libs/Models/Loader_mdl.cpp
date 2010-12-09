/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "Loader_mdl.hpp"
#include "Loader_mdl.h"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Math3D/Quaternion.hpp"
#include "String.hpp"

#include <stdio.h>
#include <string.h>


LoaderHL1mdlT::LoaderHL1mdlT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : ModelLoaderT(FileName)
{
    // 1. Initialisiere einfache Variablen.
    // ************************************

    if (!cf::String::EndsWith(m_FileName, ".mdl")) throw ModelT::LoadError();

    std::string BaseName(m_FileName.c_str(), m_FileName.length()-4);


    // 2. Lade die Model-Daten.
    // ************************

    FILE* InFile=fopen(m_FileName.c_str(), "rb");
    if (InFile==NULL) throw ModelT::LoadError();

    fseek(InFile, 0, SEEK_END); ModelData.PushBackEmpty(ftell(InFile));
    fseek(InFile, 0, SEEK_SET);

    if (fread(&ModelData[0], ModelData.Size(), 1, InFile)==0) { }   // Must check the return value of fread() with GCC 4.3...
    fclose(InFile);


    // 3. Abkürzungen und Checks.
    // **************************

    StudioHeader=(StudioHeaderT*)(&ModelData[0]);   // Erste Abkürzung zur Vereinfachung des folgenden Codes

    // TODO: Sanity Checks (ist das auch wirklich ein vollständiges mdl Model? NumBodyParts>0?)
    // ...


    // 4. Lade ggf. Texturen nach.
    // ***************************

    // Falls das Model selbst keine Texturen mit sich bringt, hänge ein "T" an den Namen an und lade die Texturen daraus!
    if (StudioHeader->NumTextures==0)
    {
        InFile=fopen((BaseName+"T.mdl").c_str(), "rb");
        if (InFile==NULL) throw ModelT::LoadError();

        fseek(InFile, 0, SEEK_END); TextureData.PushBackEmpty(ftell(InFile));
        fseek(InFile, 0, SEEK_SET);

        if (fread(&TextureData[0], TextureData.Size(), 1, InFile)==0) { }   // Must check the return value of fread() with GCC 4.3...
        fclose(InFile);
    }


    // 5. Lade ggf. Sequences in anderen SequenceGroups nach.
    // ******************************************************

    // Falls es Sequences in mehr als einer SequenceGroup gibt, lade diese nun nach (d.h. KEIN "load on demand")!
    if (StudioHeader->NumSeqGroups>1)
    {
        AnimationData.PushBackEmpty(StudioHeader->NumSeqGroups);

        // AnimationData[0] wird niemals initialisiert oder verwendet, denn diese Information ist schon Bestandteil von ModelData!
        for (int SeqGroupNr=1; SeqGroupNr<StudioHeader->NumSeqGroups; SeqGroupNr++)
        {
            char NumStr[10];
            sprintf(NumStr, "%02d", SeqGroupNr);

            InFile=fopen((BaseName+NumStr+".mdl").c_str(), "rb");
            if (InFile==NULL) throw ModelT::LoadError();

            fseek(InFile, 0, SEEK_END); AnimationData[SeqGroupNr].PushBackEmpty(ftell(InFile));
            fseek(InFile, 0, SEEK_SET);

            if (fread(&AnimationData[SeqGroupNr][0], AnimationData[SeqGroupNr].Size(), 1, InFile)==0) { }   // Must check the return value of fread() with GCC 4.3...
            fclose(InFile);
        }
    }


    // 6. Laden erfolgreich.
    // *********************

    // Weitere Abkürzungen zur Vereinfachung des Worker-Codes
    StudioBones          =(StudioBoneT*          )(&ModelData[0]+StudioHeader->BoneIndex          );
    StudioBoneControllers=(StudioBoneControllerT*)(&ModelData[0]+StudioHeader->BoneControllerIndex);
 // StudioHitBoxes       =(StudioHitBoxT*        )(&ModelData[0]+StudioHeader->HitBoxIndex        );
    StudioSequences      =(StudioSequenceT*      )(&ModelData[0]+StudioHeader->SeqIndex           );
    StudioSequenceGroups =(StudioSequenceGroupT* )(&ModelData[0]+StudioHeader->SeqGroupIndex      );
    StudioTextureHeader  =TextureData.Size()>0 ? (StudioHeaderT*)&TextureData[0] : StudioHeader;
    StudioTextures       =(StudioTextureT*       )((char*)StudioTextureHeader+StudioTextureHeader->TextureIndex);
    StudioBodyParts      =(StudioBodyPartT*      )(&ModelData[0]+StudioHeader->BodyPartIndex      );
    StudioAttachments    =(StudioAttachmentT*    )(&ModelData[0]+StudioHeader->AttachmentIndex    );
 // StudioTransitions    =(StudioTransitionT*    )(&ModelData[0]+StudioHeader->TransitionIndex    );

    // Abschluss
    for (int TexNr=0; TexNr<StudioTextureHeader->NumTextures; TexNr++)
    {
        const char* RelFileName1=strstr(BaseName.c_str(), "Models");        // Strip the leading "Games/DeathMatch/", if present.
        std::string RelFileName2=RelFileName1 ? RelFileName1 : BaseName;
        std::string MaterialName=RelFileName2+"/"+StudioTextures[TexNr].Name;

        // Flip back-slashes.
        for (unsigned long i=0; i<MaterialName.length(); i++) if (MaterialName.at(i)=='\\') MaterialName.at(i)='/';

        // Strip any extension.
        for (size_t i=MaterialName.length(); i>0; i--)
        {
            if (MaterialName.at(i-1)=='/') break;

            if (MaterialName.at(i-1)=='.')
            {
                MaterialName=std::string(MaterialName.c_str(), i-1);
                break;
            }
        }

        MaterialT* Material=MaterialManager->GetMaterial(MaterialName);

        // if (Material==NULL) printf("WARNING: Material '%s' not found!\n", MaterialName.c_str());

        Materials.PushBack(Material);
        RenderMaterials.PushBack(MatSys::Renderer!=NULL ? MatSys::Renderer->RegisterMaterial(Material) : NULL);
    }
}


LoaderHL1mdlT::~LoaderHL1mdlT()
{
    for (unsigned long RMNr=0; RMNr<RenderMaterials.Size(); RMNr++)
        if (MatSys::Renderer!=NULL) MatSys::Renderer->FreeMaterial(RenderMaterials[RMNr]);
}


bool LoaderHL1mdlT::UseGivenTS() const
{
    // TODO...
    return false;
}


struct PosQtrT
{
    Vector3fT Pos;
    Vector3fT Ang;
    Vector3fT Qtr;
};


void LoaderHL1mdlT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims)
{
    ArrayT<MatrixT> Matrices;

    Matrices.PushBackEmptyExact(StudioHeader->NumBones);
    for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
    {
        const StudioBoneT& Bone=StudioBones[BoneNr];

        Matrices[BoneNr]=MatrixT(cf::math::QuaternionfT::Euler(Bone.Value[4], Bone.Value[5], Bone.Value[3]), Vector3fT(&Bone.Value[0]));

        if (Bone.Parent!=-1)
        {
            if (Bone.Parent>=BoneNr)
                throw ModelLoaderT::LoadErrorT("Bone hierarchy: Child bone preceeds parent bone!");

            Matrices[BoneNr]=Matrices[Bone.Parent] * Matrices[BoneNr];
        }
    }

    Joints.PushBackEmptyExact(StudioHeader->NumBones);
    for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
    {
        CafuModelT::JointT&   Joint=Joints[BoneNr];
        const MatrixT&        Mat  =Matrices[BoneNr];
        cf::math::Matrix3x3fT Mat3x3;

        for (unsigned long i=0; i<3; i++)
            for (unsigned long j=0; j<3; j++)
                Mat3x3[i][j]=Mat[i][j];

        Joint.Name  =StudioBones[BoneNr].Name;
        Joint.Parent=StudioBones[BoneNr].Parent;
        Joint.Pos   =Vector3fT(Mat[0][3], Mat[1][3], Mat[2][3]);
        Joint.Qtr   =cf::math::QuaternionfT(Mat3x3).GetXYZ();

        assert(MatrixT(cf::math::QuaternionfT::FromXYZ(Joint.Qtr), Joint.Pos).IsEqual(Mat, 0.01f));
    }


    Meshes.PushBackEmpty();
    Meshes[0].Material=NULL;
    Meshes[0].RenderMaterial=NULL;


    Anims.PushBackEmptyExact(StudioHeader->NumSeq);
    for (int SequNr=0; SequNr<StudioHeader->NumSeq; SequNr++)
    {
        const StudioSequenceT& Sequ=StudioSequences[SequNr];
        const StudioAnimT*     Animations=(Sequ.SeqGroup==0) ? (StudioAnimT*)(&ModelData[0]+StudioSequenceGroups[0].Data +Sequ.AnimIndex)
                                                             : (StudioAnimT*)(&AnimationData[Sequ.SeqGroup][0]+Sequ.AnimIndex);

        // Skip sequences with no frames (and make sure that we can safely access frame 0 below).
        if (Sequ.NumFrames<1) continue;

        // Gather all animation data of this sequence in uncompressed form in AllData[BoneNr][FrameNr].
        // That is, for each frame and for each bone, we store the position and quaterion.
        ArrayT< ArrayT<PosQtrT> > AllData;

        AllData.PushBackEmptyExact(StudioHeader->NumBones);
        for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
        {
            AllData[BoneNr].PushBackEmptyExact(Sequ.NumFrames);

            for (int FrameNr=0; FrameNr<Sequ.NumFrames; FrameNr++)
            {
                // Initialize with the defaults (which only depend on the bone, not on the sequence and not on the frame).
                // Note that this is just the default pose of the static, unanimated model.
                AllData[BoneNr][FrameNr].Pos = Vector3fT(&StudioBones[BoneNr].Value[0]);
                AllData[BoneNr][FrameNr].Ang = Vector3fT(&StudioBones[BoneNr].Value[3]);
            }

            for (int j=0; j<6; j++)
            {
                if (Animations[BoneNr].Offset[j]!=0)
                {
                    // There are non-default values for this bone's coordinate j in this sequence.
                    StudioAnimValueT* AnimValues=(StudioAnimValueT*)(((char*)&Animations[BoneNr])+Animations[BoneNr].Offset[j]);

                    int SpanValid=1;    // The values start at AnimValues[1].
                    int SpanTotal=1;

                    for (int FrameNr=0; FrameNr<Sequ.NumFrames; FrameNr++)
                    {
                        assert(AnimValues[0].Num.Valid>0 && AnimValues[0].Num.Total>=AnimValues[0].Num.Valid);

                        const float Delta=AnimValues[SpanValid].Value * StudioBones[BoneNr].Scale[j];

                        if (j<3) AllData[BoneNr][FrameNr].Pos[j  ] += Delta;
                            else AllData[BoneNr][FrameNr].Ang[j-3] += Delta;

                        if (SpanValid<AnimValues[0].Num.Valid) SpanValid++;
                        SpanTotal++;

                        if (SpanTotal>AnimValues[0].Num.Total)
                        {
                            // Advance to the next span.
                            SpanValid=1;
                            SpanTotal=1;

                            AnimValues+=AnimValues[0].Num.Valid+1;
                        }
                    }
                }
            }

            for (int FrameNr=0; FrameNr<Sequ.NumFrames; FrameNr++)
            {
                const Vector3fT Ang=AllData[BoneNr][FrameNr].Ang;

                // Compute the corresponding quaternion from the angles.
                AllData[BoneNr][FrameNr].Qtr=cf::math::QuaternionfT::Euler(Ang[1], Ang[2], Ang[0]).GetXYZ();
            }
        }


        CafuModelT::AnimT& Anim=Anims[SequNr];
        unsigned int       AnimData_Size=0;   // The current common value of Anim.Frames[FrameNr].AnimData.Size(), always the same for all frames.

     // Anim.Name=Sequ.Label;
        Anim.FPS =Sequ.FPS;
     // Anim.Next=-1;

        Anim.AnimJoints.PushBackEmptyExact(StudioHeader->NumBones);
        Anim.Frames.PushBackEmptyExact(Sequ.NumFrames);

        for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
        {
            const StudioBoneT&             Bone     =StudioBones[BoneNr];
            CafuModelT::AnimT::AnimJointT& AnimJoint=Anim.AnimJoints[BoneNr];

            // For (space) efficiency, the defaults are taken from frame 0, rather than from the "unrelated" Bone.Value[...] as in HL1 mdl:
            // the resulting AnimData then require only 1/4 to 1/3 of the original size! (tested with Trinity.mdl)
            for (unsigned int i=0; i<3; i++) AnimJoint.BaseValues[i  ]=AllData[BoneNr][0].Pos[i];
            for (unsigned int i=0; i<3; i++) AnimJoint.BaseValues[i+3]=AllData[BoneNr][0].Qtr[i];
            AnimJoint.Flags=0;
            AnimJoint.FirstDataIdx=AnimData_Size;

            for (int FrameNr=0; FrameNr<Sequ.NumFrames; FrameNr++)
            {
                for (unsigned int i=0; i<6; i++)
                {
                    const float Value=(i<3) ? AllData[BoneNr][FrameNr].Pos[i]
                                            : AllData[BoneNr][FrameNr].Qtr[i-3];

                    if (Value!=AnimJoint.BaseValues[i]) AnimJoint.Flags|=(1u << i);
                }
            }

            for (unsigned int i=0; i<6; i++)
            {
                if (((AnimJoint.Flags >> i) & 1)==0) continue;

                for (int FrameNr=0; FrameNr<Sequ.NumFrames; FrameNr++)
                {
                    assert(Anim.Frames[FrameNr].AnimData.Size()==AnimData_Size);

                    Anim.Frames[FrameNr].AnimData.PushBack(i<3 ? AllData[BoneNr][FrameNr].Pos[i]
                                                               : AllData[BoneNr][FrameNr].Qtr[i-3]);
                }

                AnimData_Size++;
            }
        }

        // If it is a looping sequence whose last frame is a repetition of the first,
        // remove the redundant frame (our own code automatically wrap at the end of the sequence).
        if (Sequ.Flags & 1) Anim.Frames.DeleteBack();
    }

    // TODO!
    //   - AnimT::FrameT::BB[6]
    //   - Sequ.LinearMovement ?
}
