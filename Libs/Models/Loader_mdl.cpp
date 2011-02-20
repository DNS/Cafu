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
    // 1. Initialize auxiliary variables.
    // **********************************

    if (!cf::String::EndsWith(m_FileName, ".mdl")) throw ModelT::LoadError();

    const std::string BaseName(m_FileName.c_str(), m_FileName.length()-4);


    // 2. Load the model data.
    // ***********************

    FILE* InFile=fopen(m_FileName.c_str(), "rb");
    if (InFile==NULL) throw ModelT::LoadError();

    fseek(InFile, 0, SEEK_END); ModelData.PushBackEmpty(ftell(InFile));
    fseek(InFile, 0, SEEK_SET);

    if (fread(&ModelData[0], ModelData.Size(), 1, InFile)==0) { }   // Must check the return value of fread() with GCC 4.3...
    fclose(InFile);


    // 3. Abbreviations and checks.
    // ****************************

    StudioHeader=(StudioHeaderT*)(&ModelData[0]);   // First abbreviation, simplifies the following code.

    // TODO: Sanity checks (is this really a complete, valid model? NumBodyParts>0?)
    // ...


    // 4. Optionally load additional textures.
    // ***************************************

    // If the model brings no textures itself, append "T" to the file name and get the textures from there!
    if (StudioHeader->NumTextures==0)
    {
        InFile=fopen((BaseName+"T.mdl").c_str(), "rb");
        if (InFile==NULL) throw ModelT::LoadError();

        fseek(InFile, 0, SEEK_END); TextureData.PushBackEmpty(ftell(InFile));
        fseek(InFile, 0, SEEK_SET);

        if (fread(&TextureData[0], TextureData.Size(), 1, InFile)==0) { }   // Must check the return value of fread() with GCC 4.3...
        fclose(InFile);
    }


    // 5. Optionally load sequences from other sequence groups.
    // ********************************************************

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

    // Additional abbreviations for our worker code.
    StudioBones          =(StudioBoneT*          )(&ModelData[0]+StudioHeader->BoneIndex          );
 // StudioBoneControllers=(StudioBoneControllerT*)(&ModelData[0]+StudioHeader->BoneControllerIndex);
 // StudioHitBoxes       =(StudioHitBoxT*        )(&ModelData[0]+StudioHeader->HitBoxIndex        );
    StudioSequences      =(StudioSequenceT*      )(&ModelData[0]+StudioHeader->SeqIndex           );
    StudioSequenceGroups =(StudioSequenceGroupT* )(&ModelData[0]+StudioHeader->SeqGroupIndex      );
    StudioTextureHeader  =TextureData.Size()>0 ? (StudioHeaderT*)&TextureData[0] : StudioHeader;
    StudioTextures       =(StudioTextureT*       )((char*)StudioTextureHeader+StudioTextureHeader->TextureIndex);
    StudioBodyParts      =(StudioBodyPartT*      )(&ModelData[0]+StudioHeader->BodyPartIndex      );
 // StudioAttachments    =(StudioAttachmentT*    )(&ModelData[0]+StudioHeader->AttachmentIndex    );
 // StudioTransitions    =(StudioTransitionT*    )(&ModelData[0]+StudioHeader->TransitionIndex    );

    // Extract the materials.
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

        m_Materials.PushBack(Material);
    }
}


bool LoaderHL1mdlT::UseGivenTS() const
{
    // We don't provide the model with a precomputed, fixed (non-animated) tangent space.
    // Instead, have the CafuModelT code compute it for each animation frame anew.
    return false;
}


void LoaderHL1mdlT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims)
{
    // For clarity and better readibility, break the loading into three separate functions.
    Load(Joints);
    Load(Meshes);
    Load(Anims);
}


void LoaderHL1mdlT::Load(ArrayT<CafuModelT::JointT>& Joints) const
{
    ArrayT<MatrixT> Matrices;

    Matrices.PushBackEmptyExact(StudioHeader->NumBones);
    for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
    {
        const StudioBoneT& Bone=StudioBones[BoneNr];

        Matrices[BoneNr]=MatrixT(Vector3fT(&Bone.Value[0]), cf::math::QuaternionfT::Euler(Bone.Value[4], Bone.Value[5], Bone.Value[3]));

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
        Joint.Scale =Vector3fT(1.0f, 1.0f, 1.0f);

        assert(MatrixT(Joint.Pos, cf::math::QuaternionfT::FromXYZ(Joint.Qtr), Joint.Scale).IsEqual(Mat, 0.01f));
    }
}


void LoaderHL1mdlT::Load(ArrayT<CafuModelT::MeshT>& Meshes) const
{
    // This is configurable with BodyNr and SkinNr, but at this time we always take the defaults (index 0 each).
    for (int BodyPartNr=0; BodyPartNr<StudioHeader->NumBodyParts; BodyPartNr++)
    {
        const StudioBodyPartT& CurrentBodyPart=StudioBodyParts[BodyPartNr];
        const StudioModelT*    StudioModels   =(StudioModelT*)(&ModelData[0]+CurrentBodyPart.ModelIndex);
        const int              ModelNr        =(/*BodyNr*/ 0/CurrentBodyPart.Base) % CurrentBodyPart.NumModels;
        const StudioModelT&    CurrentModel   =StudioModels[ModelNr];
        const int              SkinNr         =0;
        const short*           SkinRefs       =((short*)((char*)StudioTextureHeader+StudioTextureHeader->SkinIndex)) + (SkinNr<StudioTextureHeader->NumSkinFamilies ? SkinNr*StudioTextureHeader->NumSkinRef : 0);

        const Vec3T*           StudioVertices =(Vec3T*)(&ModelData[0]+CurrentModel.VertIndex);  // List of all vertices of the CurrentModel.
        const char*            BoneForStVertex=(&ModelData[0]+CurrentModel.VertInfoIndex);      // BoneForStVertex[VertexNr] yields the bone number for vertex VertexNr.
     // const Vec3T*           StudioNormals  =(Vec3T*)(&ModelData[0]+CurrentModel.NormIndex);  // List of all normals of the CurrentModel.
     // const char*            BoneForStNormal=(&ModelData[0]+CurrentModel.NormInfoIndex);      // BoneForStNormal[NormalNr] yields the bone number for normal NormalNr.
        const StudioMeshT*     StudioMeshes   =(StudioMeshT*)(&ModelData[0]+CurrentModel.MeshIndex);

        const unsigned long FirstMeshInModel=Meshes.Size();

        for (int StudioMeshNr=0; StudioMeshNr<CurrentModel.NumMesh; StudioMeshNr++)
        {
            const StudioMeshT& StudioMesh=StudioMeshes[StudioMeshNr];
            const float        MatWidth  =float(StudioTextures[SkinRefs[StudioMesh.SkinRef]].Width);
            const float        MatHeight =float(StudioTextures[SkinRefs[StudioMesh.SkinRef]].Height);
            MaterialT*         Material  =m_Materials[SkinRefs[StudioMesh.SkinRef]];

            // Find a CafuModelT::MeshT with the same material (and the same set of weights, i.e. another one created from CurrentModel).
            unsigned long CafuMeshNr;

            for (CafuMeshNr=FirstMeshInModel; CafuMeshNr<Meshes.Size(); CafuMeshNr++)
                if (Meshes[CafuMeshNr].Material==Material)
                    break;

            // None found, create a new one.
            if (CafuMeshNr>=Meshes.Size())
            {
                Meshes.PushBackEmpty();

                Meshes[CafuMeshNr].Material      =Material;
                Meshes[CafuMeshNr].RenderMaterial=MatSys::Renderer->RegisterMaterial(Material);

                Meshes[CafuMeshNr].Weights.PushBackEmptyExact(CurrentModel.NumVerts);
                for (int VertexNr=0; VertexNr<CurrentModel.NumVerts; VertexNr++)
                {
                    CafuModelT::MeshT::WeightT& Weight=Meshes[CafuMeshNr].Weights[VertexNr];

                    Weight.JointIdx=BoneForStVertex[VertexNr];
                    Weight.Weight  =1.0f;
                    Weight.Pos     =Vector3fT(StudioVertices[VertexNr]);
                }
            }

            // Now add all triangles of all triangle strips in StudioMesh to CafuMesh.
            CafuModelT::MeshT&  CafuMesh      =Meshes[CafuMeshNr];
            const signed short* TriangleStrips=(signed short*)(&ModelData[0]+StudioMesh.TriIndex);

            while (*TriangleStrips)
            {
                // Number and type (FAN vs. STRIP) of vertices in next TriangleStrip.
                signed short       NrOfVertices    =*(TriangleStrips++);
                bool               IsTriangleFan   =false;
                const signed short (*VertexDefs)[4]=(const signed short (*)[4])TriangleStrips;

                if (NrOfVertices<0)
                {
                    NrOfVertices =-NrOfVertices;
                    IsTriangleFan=true;
                }

                // 'NrOfVertices-2' triangles will be created (both for FANs as well as STRIPs).
                for (int TriNr=0; TriNr<NrOfVertices-2; TriNr++)
                {
                    const signed short* TriangleVerts[3];

                    if (IsTriangleFan)
                    {
                        TriangleVerts[0]=VertexDefs[0];
                        TriangleVerts[1]=VertexDefs[TriNr+1];
                        TriangleVerts[2]=VertexDefs[TriNr+2];
                    }
                    else
                    {
                        if (TriNr & 1)
                        {
                            // 'TriNr' is odd.
                            TriangleVerts[0]=VertexDefs[TriNr+1];
                            TriangleVerts[1]=VertexDefs[TriNr  ];
                            TriangleVerts[2]=VertexDefs[TriNr+2];
                        }
                        else
                        {
                            // 'TriNr' is even.
                            TriangleVerts[0]=VertexDefs[TriNr  ];
                            TriangleVerts[1]=VertexDefs[TriNr+1];
                            TriangleVerts[2]=VertexDefs[TriNr+2];
                        }
                    }

                    CafuModelT::MeshT::TriangleT CafuTri;

                    for (unsigned int i=0; i<3; i++)
                    {
                        // TriangleVerts[i][0] -- index into the StudioVertices (or CafuMesh.Weights) array.
                        // TriangleVerts[i][1] -- index into the StudioNormals array.
                        // TriangleVerts[i][2] -- s-texture-coordinate (in pixel).
                        // TriangleVerts[i][3] -- t-texture-coordinate (in pixel).
                        unsigned long CafuVertexNr;

                        for (CafuVertexNr=0; CafuVertexNr<CafuMesh.Vertices.Size(); CafuVertexNr++)
                        {
                            const CafuModelT::MeshT::VertexT& CafuVertex=CafuMesh.Vertices[CafuVertexNr];

                            if (CafuVertex.FirstWeightIdx==(unsigned int)(TriangleVerts[i][0]) &&
                                CafuVertex.u             ==TriangleVerts[i][2]/MatWidth &&
                                CafuVertex.v             ==TriangleVerts[i][3]/MatHeight) break;
                        }

                        if (CafuVertexNr>=CafuMesh.Vertices.Size())
                        {
                            CafuModelT::MeshT::VertexT CafuVertex;

                            CafuVertex.FirstWeightIdx=TriangleVerts[i][0];
                            CafuVertex.NumWeights    =1;
                            CafuVertex.u             =TriangleVerts[i][2]/MatWidth;
                            CafuVertex.v             =TriangleVerts[i][3]/MatHeight;
                            CafuVertex.Polarity      =false;    // Don't leave this uninitialized now, it's re-initialized in the CafuModelT code later.

                            CafuMesh.Vertices.PushBack(CafuVertex);
                        }

                        CafuTri.VertexIdx[i]=CafuVertexNr;
                    }

                    CafuMesh.Triangles.PushBack(CafuTri);
                }

                TriangleStrips+=4*NrOfVertices;
            }
        }
    }
}


struct PosQtrT
{
    Vector3fT Pos;
    Vector3fT Ang;
    Vector3fT Qtr;
};


void LoaderHL1mdlT::Load(ArrayT<CafuModelT::AnimT>& Anims) const
{
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
            CafuModelT::AnimT::AnimJointT& AnimJoint=Anim.AnimJoints[BoneNr];

            // For (space) efficiency, the defaults are taken from frame 0, rather than from the "unrelated" Bone.Value[...] as in HL1 mdl:
            // the resulting AnimData then require only 1/4 to 1/3 of the original size! (tested with Trinity.mdl)
            AnimJoint.DefaultPos=AllData[BoneNr][0].Pos;
            AnimJoint.DefaultQtr=AllData[BoneNr][0].Qtr;
            AnimJoint.DefaultScale=Vector3fT(1.0f, 1.0f, 1.0f);
            AnimJoint.Flags=0;
            AnimJoint.FirstDataIdx=AnimData_Size;

            for (int FrameNr=0; FrameNr<Sequ.NumFrames; FrameNr++)
            {
                for (unsigned int i=0; i<3; i++)
                {
                    if (AllData[BoneNr][FrameNr].Pos[i]!=AnimJoint.DefaultPos[i]) AnimJoint.Flags|=(1u << (i+0));
                    if (AllData[BoneNr][FrameNr].Qtr[i]!=AnimJoint.DefaultQtr[i]) AnimJoint.Flags|=(1u << (i+3));
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

        // Fill in the bounding-box for each frame from the sequence BB from the mdl file, which doesn't keep per-frame BBs.
        // It would be more accurate of course if we re-computed the proper per-frame BB ourselves...
        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
            Anim.Frames[FrameNr].BB=BoundingBox3fT(Vector3fT(Sequ.BBMin), Vector3fT(Sequ.BBMax));
    }

    // TODO: Sequ.LinearMovement ?
}
