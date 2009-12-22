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

/************************/
/*** mdl Model (Code) ***/
/************************/

// TODO: Nimmt das MatSys in der RenderAction AMBIENT die LightColor als Ambient Color, oder wird die ignoriert???
// TODO: Can we get rid of all *T.mdl files???
// TODO: Derive StudioModelExtendedT from StudioModelT, and use that instead of StudioModelTs....!!!!!

// For printing slowing-down diagnostics
#define EMIT_DIAGNOSTICS 0

// This is for "IsUpdateRequired_Transformed...".
#define ENABLE_OPT_CACHING 1

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Vector3.hpp"
#include "Model_mdl.hpp"
#include "Model_mdl.h"
#include "Model_proxy.hpp"
#include "String.hpp"

#ifndef _WIN32
#define _stricmp strcasecmp
#endif


/***********************/
/*** Hilfsfunktionen ***/
/***********************/

static void AnglesToQuaternion(const ModelMdlT::Vec3T Angles, ModelMdlT::Vec4T Quaternion)
{
    // FIXME: rescale the inputs to 1/2 angle
    const float SinRoll =sin(0.5f*Angles[0]);
    const float CosRoll =cos(0.5f*Angles[0]);
    const float SinPitch=sin(0.5f*Angles[1]);
    const float CosPitch=cos(0.5f*Angles[1]);
    const float SinYaw  =sin(0.5f*Angles[2]);
    const float CosYaw  =cos(0.5f*Angles[2]);

    Quaternion[0]=SinRoll*CosPitch*CosYaw-CosRoll*SinPitch*SinYaw;  // x
    Quaternion[1]=CosRoll*SinPitch*CosYaw+SinRoll*CosPitch*SinYaw;  // y
    Quaternion[2]=CosRoll*CosPitch*SinYaw-SinRoll*SinPitch*CosYaw;  // z
    Quaternion[3]=CosRoll*CosPitch*CosYaw+SinRoll*SinPitch*SinYaw;  // w
}


static void QuaternionSlerp(const ModelMdlT::Vec4T p, ModelMdlT::Vec4T q, float t, ModelMdlT::Vec4T qt)
{
    int   i;
    float a=0;
    float b=0;

    // Decide if one of the quaternions is backwards.
    for (i=0; i<4; i++)
    {
        a+=(p[i]-q[i])*(p[i]-q[i]);
        b+=(p[i]+q[i])*(p[i]+q[i]);
    }

    if (a>b) for (i=0; i<4; i++) q[i]=-q[i];

    const float CosOmega=p[0]*q[0]+p[1]*q[1]+p[2]*q[2]+p[3]*q[3];

    if (1.0+CosOmega>0.00000001)
    {
        float sclp;
        float sclq;

        if (1.0-CosOmega>0.00000001)
        {
            float    Omega=acos(CosOmega);
            float SinOmega=sin (Omega);

            sclp=sin((1.0f-t)*Omega)/SinOmega;
            sclq=sin(      t *Omega)/SinOmega;
        }
        else
        {
            sclp=1.0f-t;
            sclq=     t;
        }

        for (i=0; i<4; i++) qt[i]=sclp*p[i]+sclq*q[i];
    }
    else
    {
        qt[0]=-p[1];
        qt[1]= p[0];
        qt[2]=-p[3];
        qt[3]= p[2];

        const float sclp=sin((1.0f-t)*0.5f*3.14159265358979323846f);
        const float sclq=sin(      t *0.5f*3.14159265358979323846f);

        for (i=0; i<3; i++) qt[i]=sclp*p[i]+sclq*qt[i];
    }
}


// This function is currently unused.
// static void RotateVectorInverse(const ModelMdlT::Vec3T In1, float In2[3][4], ModelMdlT::Vec3T Out)
// {
//     // In2 ist eine 4x4 Transformationsmatrix, deren letzte triviale Zeile [0 0 0 1] ausgelassen wurde,
//     // und die im linken 3x3-Teil eine Rotation und im rechten 3x1-Teil eine Translation beschreibt.
//     // Wende die in In2 beschriebene Rotation invertiert auf In1 an: Out=In1*invert(In2)
//     Out[0]=In1[0]*In2[0][0]+In1[1]*In2[1][0]+In1[2]*In2[2][0];
//     Out[1]=In1[0]*In2[0][1]+In1[1]*In2[1][1]+In1[2]*In2[2][1];
//     Out[2]=In1[0]*In2[0][2]+In1[1]*In2[1][2]+In1[2]*In2[2][2];
// }


/*****************/
/*** mdl Model ***/
/*****************/

ModelMdlT::ModelMdlT(const std::string& FileName_) /*throw (ModelT::LoadError)*/
    : FileName(FileName_),

      // These variables must be set to invalid values for the very first frame.
      SequenceNrOfTBUpdate(-1), FrameNrOfTBUpdate(-1.0),
      SequenceNrOfTVUpdate(-1), FrameNrOfTVUpdate(-1.0),
      SequenceNrOfTNUpdate(-1), FrameNrOfTNUpdate(-1.0),
      SequenceNrOfTTUpdate(-1), FrameNrOfTTUpdate(-1.0),

      // Proper initialization is required e.g. for default models.
      TransformedVerticesOfAllBP_(NULL),
      TransformedNormalsOfAllBP_(NULL),
      TransformedTangentsOfAllBP_(NULL),
      TransformedBiNormalsOfAllBP_(NULL)
{
    // 1. Initialisiere einfache Variablen.
    // ************************************

    if (!cf::String::EndsWith(FileName, ".mdl")) throw ModelT::LoadError();

    std::string BaseName(FileName.c_str(), FileName.length()-4);


    // 2. Lade die Model-Daten.
    // ************************

    FILE* InFile=fopen(FileName.c_str(), "rb");
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
        for (unsigned long i=MaterialName.length(); i>0; i--)
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


    // 7. (Pre-)Compute the triangle aux. data for all body parts.
    // ***********************************************************

    unsigned long NumVerticesInBodyParts=0;     // Count the total number of vertices of (the first models of) all body parts.
    unsigned long NumNormalsInBodyParts =0;     // Count the total number of normals  of (the first models of) all body parts.

    TriangleInfos.PushBackEmpty(StudioHeader->NumBodyParts);

    int BodyPartNr;
    for (BodyPartNr=0; BodyPartNr<StudioHeader->NumBodyParts; BodyPartNr++)
    {
        // Note that each body part can have multiple models (e.g. high- and low-resolution variants),
        // each model has vertices, normals, and multiple meshes,
        // and each mesh has multiple triangle strips and triangle fans.
        // We always choose the default model ('BodyNr==0') for each body part.
        const StudioBodyPartT& CurrentBodyPart=StudioBodyParts[BodyPartNr];
        const StudioModelT*    StudioModels   =(StudioModelT*)(&ModelData[0]+CurrentBodyPart.ModelIndex);
        const int              ModelNr        =(/*BodyNr*/ 0/CurrentBodyPart.Base) % CurrentBodyPart.NumModels;
        const StudioModelT&    CurrentModel   =StudioModels[ModelNr];
        const StudioMeshT*     StudioMeshes   =(StudioMeshT*)(&ModelData[0]+CurrentModel.MeshIndex);
        const int              SkinNr         =0;
        const short*           SkinRefs       =((short*)((char*)StudioTextureHeader+StudioTextureHeader->SkinIndex)) + (SkinNr<StudioTextureHeader->NumSkinFamilies ? SkinNr*StudioTextureHeader->NumSkinRef : 0);

        for (int MeshNr=0; MeshNr<CurrentModel.NumMesh; MeshNr++)
        {
            const StudioMeshT&  CurrentMesh   =StudioMeshes[MeshNr];
            const signed short* TriangleStrips=(signed short*)(&ModelData[0]+CurrentMesh.TriIndex);

            while (*TriangleStrips)
            {
                // Anzahl und Art (FAN vs. STRIP) der Vertices im nächsten TriangleStrip.
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
                    TriangleInfoT TI;

                    TI.Neighbours[0]=-1;
                    TI.Neighbours[1]=-1;
                    TI.Neighbours[2]=-1;

                    TI.MaterialIndex=SkinRefs[CurrentMesh.SkinRef];

                    if (IsTriangleFan)
                    {
                        TI.Vertices[0]=VertexDefs[0];
                        TI.Vertices[1]=VertexDefs[TriNr+1];
                        TI.Vertices[2]=VertexDefs[TriNr+2];
                    }
                    else
                    {
                        if (TriNr & 1)
                        {
                            // 'TriNr' is odd.
                            TI.Vertices[0]=VertexDefs[TriNr+1];
                            TI.Vertices[1]=VertexDefs[TriNr  ];
                            TI.Vertices[2]=VertexDefs[TriNr+2];
                        }
                        else
                        {
                            // 'TriNr' is even.
                            TI.Vertices[0]=VertexDefs[TriNr  ];
                            TI.Vertices[1]=VertexDefs[TriNr+1];
                            TI.Vertices[2]=VertexDefs[TriNr+2];
                        }
                    }

                    TriangleInfos[BodyPartNr].PushBack(TI);
                }

                TriangleStrips+=4*NrOfVertices;
            }
        }

        // Compute additional triangle information (neighbours).
        // Note that we cannot precompute the normal vectors of the triangles,
        // as this is, due to the animation of the models, only possible after transformation.
        for (int TriNr=0; TriNr<int(TriangleInfos[BodyPartNr].Size()); TriNr++)
        {
            TriangleInfoT& T=TriangleInfos[BodyPartNr][TriNr];

            // Init with "no neighbours".
            T.Neighbours[0]=-1;
            T.Neighbours[1]=-1;
            T.Neighbours[2]=-1;
        }

        // Important note: If three triangles share a common edge, the relevant edge of *all* three triangles must be flagged with -2
        // (have multiple neighbours at this edge, treat like it was a free edge with no neighbour).
        // However, the fact that the three triangles share a common edge IS TYPICALLY DETECTED FOR ONLY *ONE* OF THE THREE TRIANGLES,
        // namely the one that has an orientation different from the two others.
        // Therefore, both Tri1Nr and Tri2Nr below must loop over the full range (instead of Tri2Nr starting at Tri1Nr+1).
        // We therefore also have to modify other triangles except for Tri1 at iteration Tri1Nr in order to make sure that all
        // triangle-edges at a triply-shared edge are set to -2 when such a case is detected.
        for (unsigned long Tri1Nr=0; Tri1Nr<TriangleInfos[BodyPartNr].Size(); Tri1Nr++)
        {
            TriangleInfoT& Tri1=TriangleInfos[BodyPartNr][Tri1Nr];

            for (unsigned long Tri2Nr=0; Tri2Nr<TriangleInfos[BodyPartNr].Size(); Tri2Nr++)
            {
                TriangleInfoT& Tri2=TriangleInfos[BodyPartNr][Tri2Nr];

                if (Tri1Nr==Tri2Nr) continue;

                for (unsigned long v1=0; v1<3; v1++)
                    for (unsigned long v2=0; v2<3; v2++)
                        if (Tri1.Vertices[v1][0]==Tri2.Vertices[(v2+1) % 3][0] && Tri1.Vertices[(v1+1) % 3][0]==Tri2.Vertices[v2][0])
                        {
                            // Tri1 and Tri2 are neighbours!
                            if (Tri1.Neighbours[v1]==-1)
                            {
                                // Tri1 had no neighbour at this edge before, set it now.
                                // This is the normal case.
                                Tri1.Neighbours[v1]=Tri2Nr;
                            }
                            else if (Tri1.Neighbours[v1]>=0)
                            {
                                // Tri1 had a single valid neighbour at this edge before, but we just found a second.
                                // That means that three triangles share a common edge!
                                // printf("WARNING: Triangle %lu has two neighbours at edge %lu: triangles %lu and %lu.\n", Tri1Nr, v1, Tri1.Neighbours[v1], Tri2Nr);

                                // Re-find the matching edge in the old neighbour.
                                TriangleInfoT& Tri3=TriangleInfos[BodyPartNr][Tri1.Neighbours[v1]];
                                unsigned long  v3;

                                for (v3=0; v3<2; v3++)      // The  v3<2  instead of  v3<3  is intentional, to be safe that v3 never gets 3 (out-of-range).
                                    if (Tri1.Vertices[v1][0]==Tri3.Vertices[(v3+1) % 3][0] && Tri1.Vertices[(v1+1) % 3][0]==Tri3.Vertices[v3][0])
                                        break;

                                // Set the shared edge of ALL THREE triangles to -2 in order to indicate that this edge leads to more than one neighbour.
                                Tri3.Neighbours[v3]=-2;
                                Tri2.Neighbours[v2]=-2;
                                Tri1.Neighbours[v1]=-2;
                            }
                            else /* (Tri1.Neighbours[v1]==-2) */
                            {
                                // This edge of Tri1 was either determined to be a triply-shared edge by some an earlier neighbour triangle,
                                // or there are even more than two neighbours at this edge...
                                // In any case, be sure to properly flag the relevant edge at the neighbour!
                                Tri2.Neighbours[v2]=-2;
                            }
                            break;
                        }
            }
        }

        /* for (unsigned long TriNr=0; TriNr<TriangleInfos[BodyPartNr].Size(); TriNr++)
        {
            TriangleInfoT& Tri=TriangleInfos[BodyPartNr][TriNr];

            // Check if Tri has degenerate edges, i.e. non-distinct vertices.
            if (Tri.Vertices[0][0]==Tri.Vertices[1][0] || Tri.Vertices[0][0]==Tri.Vertices[2][0] || Tri.Vertices[1][0]==Tri.Vertices[2][0])
                printf("WARNING: Tri %lu has degenerate edges! Vertices: %i %i %i\n", TriNr, Tri.Vertices[0][0], Tri.Vertices[1][0], Tri.Vertices[2][0]);

            // Do a quick check for debugging if everybody has a neighbour.
            if (Tri.Neighbours[0]<0 || Tri.Neighbours[1]<0 || Tri.Neighbours[2]<0)
                printf("WARNING: Tri %lu has not neighbours everywhere: %i %i %i\n", TriNr, Tri.Neighbours[0], Tri.Neighbours[1], Tri.Neighbours[2]);
        } */


        // (Pre-)Compute the contents of the TrianglesReferingToNormal array.
        TrianglesReferingToNormal.PushBackEmpty();
        TrianglesReferingToNormal[BodyPartNr].PushBackEmpty(CurrentModel.NumNorms);

        for (unsigned long TriangleNr=0; TriangleNr<TriangleInfos[BodyPartNr].Size(); TriangleNr++)
        {
            const TriangleInfoT& TI=TriangleInfos[BodyPartNr][TriangleNr];

            for (unsigned long VertexNr=0; VertexNr<3; VertexNr++)
            {
                const unsigned short NormalIndex=TI.Vertices[VertexNr][1];

                TrianglesReferingToNormal[BodyPartNr][NormalIndex].PushBack(TriangleNr);
            }
        }


        // Sum up the number of vertices and normals for the first model of all body parts.
        NumVerticesInBodyParts+=CurrentModel.NumVerts;
        NumNormalsInBodyParts +=CurrentModel.NumNorms;
    }

    TransformedVerticesOfAllBP_ =new Vec3T[NumVerticesInBodyParts];
    TransformedNormalsOfAllBP_  =new Vec3T[NumNormalsInBodyParts ];
    TransformedTangentsOfAllBP_ =new Vec3T[NumNormalsInBodyParts ];
    TransformedBiNormalsOfAllBP_=new Vec3T[NumNormalsInBodyParts ];

    NumVerticesInBodyParts=0;
    NumNormalsInBodyParts =0;

    for (BodyPartNr=0; BodyPartNr<StudioHeader->NumBodyParts; BodyPartNr++)
    {
        // Note that each body part can have multiple models (e.g. high- and low-resolution variants),
        // each model has vertices, normals, and multiple meshes,
        // and each mesh has multiple triangle strips and triangle fans.
        // We always choose the default model ('BodyNr==0') for each body part.
        const StudioBodyPartT& CurrentBodyPart=StudioBodyParts[BodyPartNr];
        const StudioModelT*    StudioModels   =(StudioModelT*)(&ModelData[0]+CurrentBodyPart.ModelIndex);
        const int              ModelNr        =(/*BodyNr*/ 0/CurrentBodyPart.Base) % CurrentBodyPart.NumModels;
        const StudioModelT&    CurrentModel   =StudioModels[ModelNr];

        // &a[i] instead of a+i works, too, but triggers warning 007 ("&array" may not produce intended result),
        // which is unnecessary in this case.
        TransformedVerticesOfAllBP .PushBack(TransformedVerticesOfAllBP_ +NumVerticesInBodyParts);
        TransformedNormalsOfAllBP  .PushBack(TransformedNormalsOfAllBP_  +NumNormalsInBodyParts );
        TransformedTangentsOfAllBP .PushBack(TransformedTangentsOfAllBP_ +NumNormalsInBodyParts );
        TransformedBiNormalsOfAllBP.PushBack(TransformedBiNormalsOfAllBP_+NumNormalsInBodyParts );

        NumVerticesInBodyParts+=CurrentModel.NumVerts;
        NumNormalsInBodyParts +=CurrentModel.NumNorms;
    }
}


ModelMdlT::~ModelMdlT()
{
    for (unsigned long RMNr=0; RMNr<RenderMaterials.Size(); RMNr++)
        if (MatSys::Renderer!=NULL) MatSys::Renderer->FreeMaterial(RenderMaterials[RMNr]);

    delete[] TransformedVerticesOfAllBP_;
    delete[] TransformedNormalsOfAllBP_;
    delete[] TransformedTangentsOfAllBP_;
    delete[] TransformedBiNormalsOfAllBP_;
}


// Berechne für das Frame 'FrameNr' der CurrentSequence die Positionen und Quaternions für alle Bones relativ zum jeweiligen Parent-Bone.
void ModelMdlT::CalcRelativeBoneTransformations(Vec3T* Positions, Vec4T* Quaternions, const StudioSequenceT& CurrentSequence, const StudioAnimT* Animations, const float FrameNr) const
{
    int   FrameNr_Int =int(FrameNr);                //  Vorkomma-Anteil der FrameNr
    float FrameNr_Frac=FrameNr-float(FrameNr_Int);  // Nachkomma-Anteil der FrameNr

    // Add in programatic controllers
    // TODO: CalcBoneAdj();

    // Beachte, daß es für jeden Bone eine Animation gibt!
    for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
    {
        Vec3T Angle1;   // Drei Winkel, kein (Richtungs-)Vektor
        Vec3T Angle2;   // Drei Winkel, kein (Richtungs-)Vektor
        int   j;

        // Calculate Quaternions
        for (j=0; j<3; j++)
        {
            if (Animations[BoneNr].Offset[j+3]==0)
            {
                Angle1[j]=StudioBones[BoneNr].Value[j+3];   // default
                Angle2[j]=StudioBones[BoneNr].Value[j+3];   // default
            }
            else
            {
                StudioAnimValueT* AnimValues=(StudioAnimValueT*)(((char*)&Animations[BoneNr])+Animations[BoneNr].Offset[j+3]);
                int k=FrameNr_Int;

                // Find span of values that includes the frame we want.
                // Das gesuchte Frame entspricht der FrameNr_Int, d.h. sollte FrameNr=5.2816 sein, suchen wir nun nach Frame k=5.
                // Es scheint, daß AnimValues->Num.Total angibt, wieviele Frames von (??? "span of values") beschrieben werden.
                while (AnimValues->Num.Total<=k)
                {
                    k-=AnimValues->Num.Total;
                    AnimValues+=AnimValues->Num.Valid+1;
                }

                // Bah, missing blend!
                if (AnimValues->Num.Valid>k)
                {
                    Angle1[j]=AnimValues[k+1].Value;

                    if (AnimValues->Num.Valid>k+1) Angle2[j]=AnimValues[k+2].Value;
                                              else if (AnimValues->Num.Total>k+1) Angle2[j]=Angle1[j];
                                                                             else Angle2[j]=AnimValues[AnimValues->Num.Valid+2].Value;
                }
                else
                {
                    Angle1[j]=AnimValues[AnimValues->Num.Valid].Value;

                    if (AnimValues->Num.Total>k+1) Angle2[j]=Angle1[j];
                                              else Angle2[j]=AnimValues[AnimValues->Num.Valid+2].Value;
                }

                Angle1[j]=StudioBones[BoneNr].Value[j+3] + Angle1[j]*StudioBones[BoneNr].Scale[j+3];
                Angle2[j]=StudioBones[BoneNr].Value[j+3] + Angle2[j]*StudioBones[BoneNr].Scale[j+3];
            }

            if (StudioBones[BoneNr].BoneController[j+3]!=-1)
            {
                // TODO: Angle1[j]+=m_adj[StudioBones[BoneNr].BoneController[j+3]];
                // TODO: Angle2[j]+=m_adj[StudioBones[BoneNr].BoneController[j+3]];
            }
        }

        // VectorCompare: Angle1!=Angle2 ?
        if (fabs(Angle1[0]-Angle2[0])>0.001 || fabs(Angle1[1]-Angle2[1])>0.001 || fabs(Angle1[2]-Angle2[2])>0.001)
        {
            Vec4T Quaternion1; AnglesToQuaternion(Angle1, Quaternion1);
            Vec4T Quaternion2; AnglesToQuaternion(Angle2, Quaternion2);

            QuaternionSlerp(Quaternion1, Quaternion2, FrameNr_Frac, Quaternions[BoneNr]);
        }
        else AnglesToQuaternion(Angle1, Quaternions[BoneNr]);


        // Calculate Position
        for (j=0; j<3; j++)
        {
            Positions[BoneNr][j]=StudioBones[BoneNr].Value[j];  // default

            if (Animations[BoneNr].Offset[j]!=0)    // Es gibt einen anderen Wert als den Default-Wert
            {
                StudioAnimValueT* AnimValues=(StudioAnimValueT*)(((char*)&Animations[BoneNr])+Animations[BoneNr].Offset[j]);
                int k=FrameNr_Int;

                // Find span of values that includes the frame we want.
                // Das gesuchte Frame entspricht der FrameNr_Int, d.h. sollte FrameNr=5.2816 sein, suchen wir nun nach Frame k=5.
                // Es scheint, daß AnimValues->Num.Total angibt, wieviele Frames von (??? "span of values") beschrieben werden.
                while (AnimValues->Num.Total<=k)
                {
                    k-=AnimValues->Num.Total;
                    AnimValues+=AnimValues->Num.Valid+1;
                }

                // If we're inside the span...
                if (AnimValues->Num.Valid>k)
                {
                    // ...is there more data in the span?
                    if (AnimValues->Num.Valid>k+1)
                             Positions[BoneNr][j]+=(AnimValues[k+1                  ].Value*(1.0f-FrameNr_Frac)+FrameNr_Frac*AnimValues[k+2                    ].Value)*StudioBones[BoneNr].Scale[j];
                        else Positions[BoneNr][j]+= AnimValues[k+1                  ].Value                                                                            *StudioBones[BoneNr].Scale[j];
                }
                else
                {
                    // Not inside the span: Are we at the end of the repeating values section and there's another section with data?
                    if (AnimValues->Num.Total<=k+1)
                             Positions[BoneNr][j]+=(AnimValues[AnimValues->Num.Valid].Value*(1.0f-FrameNr_Frac)+FrameNr_Frac*AnimValues[AnimValues->Num.Valid+2].Value)*StudioBones[BoneNr].Scale[j];
                        else Positions[BoneNr][j]+= AnimValues[AnimValues->Num.Valid].Value                                                                            *StudioBones[BoneNr].Scale[j];
                }
            }

            // TODO: if (StudioBones[BoneNr].BoneController[j]!=-1) Positions[BoneNr][j]+=m_adj[StudioBones[BoneNr].BoneController[j]];
        }
    }

    // Was geht hier eigentlich ab?
    if (CurrentSequence.MotionType & /*STUDIO_X*/ 1) Positions[CurrentSequence.MotionBone][0]=0;
    if (CurrentSequence.MotionType & /*STUDIO_Y*/ 2) Positions[CurrentSequence.MotionBone][1]=0;
    if (CurrentSequence.MotionType & /*STUDIO_Z*/ 4) Positions[CurrentSequence.MotionBone][2]=0;
}


void ModelMdlT::DoDraw(const int SequenceNr, const float FrameNr, const ModelMdlT* SubModel,
                       const int SuperModel_NumBones, const StudioBoneT* SuperModel_StudioBones, float SuperModel_TransformedBones[][3][4]) const
{
    /* if (IsDefaultModel)
    {
        if (MatSys::Renderer->GetCurrentRenderAction()!=MatSys::RendererI::AMBIENT) return;

        static MatSys::MeshT Mesh(MatSys::MeshT::Lines);

        if (Mesh.Vertices.Size()==0)
        {
            Mesh.Vertices.PushBackEmpty(6);

            Mesh.Vertices[0].SetColor(1.0, 0.0, 0.0); Mesh.Vertices[0].SetOrigin( 0.0,  0.0,  0.0);
            Mesh.Vertices[1].SetColor(1.0, 0.0, 0.0); Mesh.Vertices[1].SetOrigin(20.0,  0.0,  0.0);     // X-Axis
            Mesh.Vertices[2].SetColor(0.0, 1.0, 0.0); Mesh.Vertices[2].SetOrigin( 0.0,  0.0,  0.0);
            Mesh.Vertices[3].SetColor(0.0, 1.0, 0.0); Mesh.Vertices[3].SetOrigin( 0.0, 20.0,  0.0);     // Y-Axis
            Mesh.Vertices[4].SetColor(0.0, 0.0, 1.0); Mesh.Vertices[4].SetOrigin( 0.0,  0.0,  0.0);
            Mesh.Vertices[5].SetColor(0.0, 0.0, 1.0); Mesh.Vertices[5].SetOrigin( 0.0,  0.0, 20.0);     // Z-Axis
        }

        // MatSys::Renderer->SetCurrentMaterial(SolidColorRM);
        MatSys::Renderer->RenderMesh(Mesh);
        return;
    } */


    const float* Light_Pos   =MatSys::Renderer->GetCurrentLightSourcePosition();
    const float  Light_Radius=MatSys::Renderer->GetCurrentLightSourceRadius();


    // Prüfe, ob das Model in Reichweite (im Radius) der Lichtquelle liegt.
    // This assumes that the per-sequence BB contains the entire animation sequence.
    // TODO: Does this check make sense for sub-models? How large is the sequence BB for sub-models after all?
    if (MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::STENCILSHADOW || MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING)
    {
        const Vector3T<float> SequenceBBMin(StudioSequences[SequenceNr].BBMin);
        const Vector3T<float> SequenceBBMax(StudioSequences[SequenceNr].BBMax);

        const Vector3T<float> TempPos(Light_Pos);
        const Vector3T<float> TempVec(Light_Radius, Light_Radius, Light_Radius);

        if (!BoundingBox3T<float>(SequenceBBMin, SequenceBBMax).Intersects(BoundingBox3T<float>(TempPos+TempVec, TempPos-TempVec))) return;
    }


    // What comes next is a REALLY TOUGH to understand problem: Figuring out what components need updating.
    // Initially, we are given with some "algorithmic components": TransformedBones, TransformedVertices, TransformedNormals,
    // and TransformedTangents (the latter also include the transformed binormals).
    // For n instances of a static (non-animated) model, this reduces the transformation computations from n passes when unlit,
    // and (2+NumOfLightSource)*n passes when dynamically lit, effectively to zero (except for the very first frame).
    // For n instances of an animated model, there is no gain, except if n==1 and the model is also dynamically lit.
    // In this case, the transformation computations are reduced by 1/(2+NumOfLightSources), because only the ambient pass will trigger
    // the recomputations, not the subsequent shadow or lighting passes (NumOfLightSources many).
    // [ The above discussion is possibly not entirely correct, as it was written early during development and does NOT take the
    // various, complicated component dependencies (detailed below) into account. ]
    // On the one hand, our drawing requirements and the render mode dictate which of the components are needed for drawing.
    // On the other hand, the components are computationally dependent. That means, for example, in order to be able to update
    // the TransformedTangents, up-to-date TransformedVertices are required, which in turn require up-to-date TransformedBones.
    // I've come to a solution that's more generic than it probably needs to be, but specializing it would only make it *very*
    // hard to understand while yielding no significant improvement.
    // Start with imagining the computational dependencies as a directed graph: The computation of the TransformedVertices and
    // TransformedNormals depends on the TransformedBones (and thus the TV and TN are children of the TB), and the Transformed-
    // Tangents depend on the TransformedVertices.
    // Now, attach two labels (or "colors") to each node: One for indicating whether this component is needed FOR DRAWING or not,
    // the other for indicating if this component is up-to-date or not.
    // SPECIAL CASE: Sub-models. Sub-models are currently(!) not animated by themselves. If they were treated as regular models,
    // they'd therefore reach a state where they are entirely up-to-date. This however would skip the take-over of the (possibly
    // animated) skeleton of the super-model. To prevent this, we flag all transformed data of sub-models as never being up-to-date,
    // effectively disabling the entire optimization (for sub-models).
    const bool IsSubModel   =(SuperModel_StudioBones!=NULL);
    const bool HaveNormalMap=Materials[0] ? !Materials[0]->NormMapComp.IsEmpty() : false;

    const bool IsNeeded_TransformedBones   =false;  // Never  needed for rendering.
    const bool IsNeeded_TransformedVertices=true;   // Always needed for rendering.
    const bool IsNeeded_TransformedNormals =(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING);
    const bool IsNeeded_TransformedTangents=(IsNeeded_TransformedNormals && HaveNormalMap);     // See below at [*].

    // [*] This is a DIRTY, UGLY HACK, as the question if tangent space is needed is actually a per-mesh matter,
    // and it is also highly unsafe to assume that Materials[...].NormMapComp.IsEmpty() implies that tangent space
    // is NOT needed by the appropriate MatSys shader!
    // Thus, the proper way to do this would be something like this:
    //
    // for all models
    //     for (int MeshNr=0; MeshNr<CurrentModel.NumMesh; MeshNr++)
    //     {
    //         const StudioMeshT& CurrentMesh=StudioMeshes[MeshNr];
    //
    //         MatSys::Renderer->SetCurrentMaterial(RenderMaterials[SkinRefs[CurrentMesh.SkinRef]]);
    //         if (IsNeeded_TransformedNormals && MatSys::Renderer->GetCurrentShader()->NeedsTangentSpace())
    //             IsNeeded_TransformedTangents=true;
    //     }

    const bool IsUpToDate_TransformedBones   =(SequenceNr==SequenceNrOfTBUpdate && FrameNr==FrameNrOfTBUpdate && !IsSubModel);
    const bool IsUpToDate_TransformedVertices=(SequenceNr==SequenceNrOfTVUpdate && FrameNr==FrameNrOfTVUpdate && !IsSubModel);
    const bool IsUpToDate_TransformedNormals =(SequenceNr==SequenceNrOfTNUpdate && FrameNr==FrameNrOfTNUpdate && !IsSubModel);
    const bool IsUpToDate_TransformedTangents=(SequenceNr==SequenceNrOfTTUpdate && FrameNr==FrameNrOfTTUpdate && !IsSubModel);

    // Finally, we state for each component if it needs updating or not.
    // For leaves (TransformedNormals and TransformedTangents), this is easy, as nothing else depends on them:
    // A leaf requires update if it is needed for drawing, but is not up-to-date.
    // A node requires update if it is needed, either for drawing or because a dependend child requires update, but is not up-to-date itself.
    // Observe that this is a kind of recursion, and closely dependent on the implementing code below.
    const bool IsUpdateRequired_TransformedTangents=(IsNeeded_TransformedTangents                                                                               ) && !IsUpToDate_TransformedTangents;
    const bool IsUpdateRequired_TransformedNormals =(IsNeeded_TransformedNormals                                                                                ) && !IsUpToDate_TransformedNormals;
    const bool IsUpdateRequired_TransformedVertices=(IsNeeded_TransformedVertices || IsUpdateRequired_TransformedTangents                                       ) && !IsUpToDate_TransformedVertices;
    const bool IsUpdateRequired_TransformedBones   =(IsNeeded_TransformedBones    || IsUpdateRequired_TransformedVertices || IsUpdateRequired_TransformedNormals) && !IsUpToDate_TransformedBones;

    // This is WRONG, because it unconditionally flags components that are not up-to-date and not needed (and thus not updated below) as updated.
    // SequenceNrOfTBUpdate=SequenceNrOfTVUpdate=SequenceNrOfTNUpdate=SequenceNrOfTTUpdate=SequenceNr;
    // FrameNrOfTBUpdate   =FrameNrOfTVUpdate   =FrameNrOfTNUpdate   =FrameNrOfTTUpdate   =FrameNr;


    // 1. Prüfe den OpenGLWindow_RenderingContextCounter
    // *************************************************

    // OBSOLETE.


    // 2. Berechne die absoluten Bone-Transformationen (Ergebnisse in 'TransformedBones').
    // ***********************************************************************************

#if EMIT_DIAGNOSTICS
    printf("XX  Entering Draw()... RM %u, up-to-date %u %u %u %u   %f %f\n", RenderMode, IsUpToDate_TransformedBones, IsUpToDate_TransformedVertices, IsUpToDate_TransformedNormals, IsUpToDate_TransformedTangents, FrameNr, FrameNrOfTVUpdate);
#endif

#if ENABLE_OPT_CACHING
    if (IsUpdateRequired_TransformedBones)
#endif
    {
        // CurrentSequence und Animationen (für jeden Bone und jedes 'Blending(?)') zur CurrentSequence, ggf. aus "load on demand" Daten
        const StudioSequenceT& CurrentSequence=StudioSequences[SequenceNr];
        const StudioAnimT*     Animations=(CurrentSequence.SeqGroup==0) ? (StudioAnimT*)(&ModelData[0]+StudioSequenceGroups[0].Data +CurrentSequence.AnimIndex)
                                                                        : (StudioAnimT*)(&AnimationData[CurrentSequence.SeqGroup][0]+CurrentSequence.AnimIndex);

        // 2.1. Zuerst die *relativen* Bone-Transformationen berechnen (Ergebnisse in 'Positions' und 'Quaternions').
        // TODO: Alle Konstanten des alten Rendering Codes auflösen und die Arrays ersetzen durch ArrayT's!
        static Vec3T Positions  [/*MAXSTUDIOBONES*/ 128];
        static Vec4T Quaternions[/*MAXSTUDIOBONES*/ 128];

        // Berechne für das Frame 'FrameNr' der CurrentSequence die Positionen und Quaternions für alle Bones relativ zum jeweiligen Parent-Bone.
        CalcRelativeBoneTransformations(Positions, Quaternions, CurrentSequence, Animations, FrameNr);

        if (CurrentSequence.NumBlends>1)
        {
            // TODO: Siehe alter (Original-) Code!
        }

        // 2.2. Nun für alle Bones die *absolute* Transformationsmatrix (TransformedBones[BoneNr]) bestimmen.
        // TransformedBones[BoneNr][Row][Column] ist ein Array von ganz normalen Transformationsmatrizen,
        // bei denen die letzte Zeile [ 0 0 0 1 ] ausgelassen wurde.
        for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
        {
            // Erst mal gucken, ob wir von dem SuperModel eine Transformationsmatrix übernehmen können!
            int SuperModelBoneNr;

            for (SuperModelBoneNr=0; SuperModelBoneNr<SuperModel_NumBones; SuperModelBoneNr++)
                if (_stricmp(StudioBones[BoneNr].Name, SuperModel_StudioBones[SuperModelBoneNr].Name)==0)
                {
                    // TransformedBones[BoneNr] = SuperModel_TransformedBones[SuperModelBoneNr];
                    memcpy(TransformedBones[BoneNr], SuperModel_TransformedBones[SuperModelBoneNr], sizeof(float)*12);
                    break;
                }

            if (SuperModelBoneNr<SuperModel_NumBones) continue;


            // Konnte für StudioBones[BoneNr] nichts verwertbares im SuperModel finden,
            // mache also ganz normal weiter (selbstständig die Transformation berechnen).
            float  BoneMatrix[3][4];
            float* Quaternion=Quaternions[BoneNr];  // zur Abkürzung

            // In den linken 3x3-Rotationsteil die sich aus Quaternions[BoneNr] ergebende Rotation bringen
            BoneMatrix[0][0]=1.0f - 2.0f*Quaternion[1]*Quaternion[1] - 2.0f*Quaternion[2]*Quaternion[2];
            BoneMatrix[1][0]=       2.0f*Quaternion[0]*Quaternion[1] + 2.0f*Quaternion[3]*Quaternion[2];
            BoneMatrix[2][0]=       2.0f*Quaternion[0]*Quaternion[2] - 2.0f*Quaternion[3]*Quaternion[1];

            BoneMatrix[0][1]=       2.0f*Quaternion[0]*Quaternion[1] - 2.0f*Quaternion[3]*Quaternion[2];
            BoneMatrix[1][1]=1.0f - 2.0f*Quaternion[0]*Quaternion[0] - 2.0f*Quaternion[2]*Quaternion[2];
            BoneMatrix[2][1]=       2.0f*Quaternion[1]*Quaternion[2] + 2.0f*Quaternion[3]*Quaternion[0];

            BoneMatrix[0][2]=       2.0f*Quaternion[0]*Quaternion[2] + 2.0f*Quaternion[3]*Quaternion[1];
            BoneMatrix[1][2]=       2.0f*Quaternion[1]*Quaternion[2] - 2.0f*Quaternion[3]*Quaternion[0];
            BoneMatrix[2][2]=1.0f - 2.0f*Quaternion[0]*Quaternion[0] - 2.0f*Quaternion[1]*Quaternion[1];

            // In den rechten 3x1-Translationsteil die Position kopieren
            BoneMatrix[0][3]=Positions[BoneNr][0];
            BoneMatrix[1][3]=Positions[BoneNr][1];
            BoneMatrix[2][3]=Positions[BoneNr][2];

            // Die BoneMatrix beschreibt die relative Transformation eines Bones bezüglich seines Parent-Bones!
            if (StudioBones[BoneNr].Parent==-1)
            {
                // TransformedBones[BoneNr] = BoneMatrix;
                memcpy(TransformedBones[BoneNr], BoneMatrix, sizeof(float)*12);
            }
            else
            {
                // TransformedBones[BoneNr] = TransformedBones[StudioBones[BoneNr].Parent]*BoneMatrix;  // "Konkatenation"
                // Dies setzt voraus, daß stets StudioBones[BoneNr].Parent<BoneNr gilt!
                float (*Out)[4]=TransformedBones[BoneNr];
                float (*In1)[4]=TransformedBones[StudioBones[BoneNr].Parent];
                float (*In2)[4]=BoneMatrix;

                // Es handelt sich um eine ganz normale Transformations-Matrizenmultiplikation Out=In1*In2,
                // wobei bei allen drei Matrizen lediglich die letzte Zeile [ 0 0 0 1 ] weggelassen wurde.
                Out[0][0] = In1[0][0]*In2[0][0] + In1[0][1]*In2[1][0] + In1[0][2]*In2[2][0];
                Out[0][1] = In1[0][0]*In2[0][1] + In1[0][1]*In2[1][1] + In1[0][2]*In2[2][1];
                Out[0][2] = In1[0][0]*In2[0][2] + In1[0][1]*In2[1][2] + In1[0][2]*In2[2][2];
                Out[0][3] = In1[0][0]*In2[0][3] + In1[0][1]*In2[1][3] + In1[0][2]*In2[2][3] + In1[0][3];
                Out[1][0] = In1[1][0]*In2[0][0] + In1[1][1]*In2[1][0] + In1[1][2]*In2[2][0];
                Out[1][1] = In1[1][0]*In2[0][1] + In1[1][1]*In2[1][1] + In1[1][2]*In2[2][1];
                Out[1][2] = In1[1][0]*In2[0][2] + In1[1][1]*In2[1][2] + In1[1][2]*In2[2][2];
                Out[1][3] = In1[1][0]*In2[0][3] + In1[1][1]*In2[1][3] + In1[1][2]*In2[2][3] + In1[1][3];
                Out[2][0] = In1[2][0]*In2[0][0] + In1[2][1]*In2[1][0] + In1[2][2]*In2[2][0];
                Out[2][1] = In1[2][0]*In2[0][1] + In1[2][1]*In2[1][1] + In1[2][2]*In2[2][1];
                Out[2][2] = In1[2][0]*In2[0][2] + In1[2][1]*In2[1][2] + In1[2][2]*In2[2][2];
                Out[2][3] = In1[2][0]*In2[0][3] + In1[2][1]*In2[1][3] + In1[2][2]*In2[2][3] + In1[2][3];
            }
        }

        SequenceNrOfTBUpdate=SequenceNr;
        FrameNrOfTBUpdate   =FrameNr;

#if EMIT_DIAGNOSTICS
        printf("XX  TBones updated\n");
#endif
    }


    // 3. Show Bones
    // *************

    /* if (ShowBones)
    {
        for (int BoneNr=0; BoneNr<StudioHeader->NumBones; BoneNr++)
        {
            if (StudioBones[BoneNr].Parent>=0)
            {
                glPointSize(3.0);
                glColor3f(1.0, 0.7, 0.0);

                glBegin(GL_LINES);
                    glVertex3f(TransformedBones[StudioBones[BoneNr].Parent][0][3],
                               TransformedBones[StudioBones[BoneNr].Parent][1][3],
                               TransformedBones[StudioBones[BoneNr].Parent][2][3]);
                    glVertex3f(TransformedBones[            BoneNr        ][0][3],
                               TransformedBones[            BoneNr        ][1][3],
                               TransformedBones[            BoneNr        ][2][3]);
                glEnd();

                glColor3f(0.0, 0.0, 0.8);

                glBegin(GL_POINTS);
                    if (StudioBones[StudioBones[BoneNr].Parent].Parent!=-1)
                        glVertex3f(TransformedBones[StudioBones[BoneNr].Parent][0][3],
                                   TransformedBones[StudioBones[BoneNr].Parent][1][3],
                                   TransformedBones[StudioBones[BoneNr].Parent][2][3]);

                    glVertex3f(TransformedBones[BoneNr][0][3],
                               TransformedBones[BoneNr][1][3],
                               TransformedBones[BoneNr][2][3]);
                glEnd();
            }
            else
            {
                // Draw parent bone node
                glPointSize(5.0);
                glColor3f(0.8, 0.0, 0.0);

                glBegin(GL_POINTS);
                    glVertex3f(TransformedBones[BoneNr][0][3],
                               TransformedBones[BoneNr][1][3],
                               TransformedBones[BoneNr][2][3]);
                glEnd();
            }
        }

        glPointSize(1.0);
        return;
    } */


    // 4. Zeichne alle BodyParts
    // *************************

    // Dies ist konfigurierbar mit BodyNr und SkinNr, wird aber zur Zeit nicht unterstützt!
    for (int BodyPartNr=0; BodyPartNr<StudioHeader->NumBodyParts; BodyPartNr++)
    {
        const StudioBodyPartT& CurrentBodyPart=StudioBodyParts[BodyPartNr];

        // Die StudioModels zum CurrentBodyPart besorgen. Es kann mehrere Models für ein BodyPart geben,
        // z.B. eine Low-Res und eine Hi-Res Variante. Diese werden über die BodyNr ausgewählt!
        // Wir legen uns hier auf den Default (BodyNr=0) fest und besorgen gleich das entsprechende Model.
        const StudioModelT* StudioModels=(StudioModelT*)(&ModelData[0]+CurrentBodyPart.ModelIndex);
        const int           ModelNr     =(/*BodyNr*/ 0/CurrentBodyPart.Base) % CurrentBodyPart.NumModels;
        const StudioModelT& CurrentModel=StudioModels[ModelNr];

        // Daten zum CurrentModel besorgen.
        const char*        VertexInfos   =(&ModelData[0]+CurrentModel.VertInfoIndex);       // Ordnet jedem Vertex                des CurrentModels den Bone zu, zu dem der Vertex  gehört
        const char*        NormalInfos   =(&ModelData[0]+CurrentModel.NormInfoIndex);       // Ordnet jeder Normalen aller Meshes des CurrentModels den Bone zu, zu dem die Normale gehört
        const StudioMeshT* StudioMeshes  =(StudioMeshT*)(&ModelData[0]+CurrentModel.MeshIndex);
        const Vec3T*       StudioVertices=(Vec3T*)(&ModelData[0]+CurrentModel.VertIndex);   // Liste aller Vertices              des CurrentModels
        const Vec3T*       StudioNormals =(Vec3T*)(&ModelData[0]+CurrentModel.NormIndex);   // Liste aller Normalen aller Meshes des CurrentModels
        const int          SkinNr        =0;
        const short*       SkinRefs      =((short*)((char*)StudioTextureHeader+StudioTextureHeader->SkinIndex)) + (SkinNr<StudioTextureHeader->NumSkinFamilies ? SkinNr*StudioTextureHeader->NumSkinRef : 0);

        // Zeiger auf die Transformed...-Arrays dieses Models bzw. BodyParts besorgen.
        Vec3T* TransformedVertices =TransformedVerticesOfAllBP [BodyPartNr];
        Vec3T* TransformedNormals  =TransformedNormalsOfAllBP  [BodyPartNr];
        Vec3T* TransformedTangents =TransformedTangentsOfAllBP [BodyPartNr];
        Vec3T* TransformedBiNormals=TransformedBiNormalsOfAllBP[BodyPartNr];

        // 4.1. Berechne die bzgl. der Bones transformierten Vertices (Ergebnisse in 'TransformedVertices').
        // Wende die Transformationsmatrix der Bones (Rotation + Translation) auf die StudioVertices an.
#if ENABLE_OPT_CACHING
        if (IsUpdateRequired_TransformedVertices)
#endif
        {
            for (int VertexNr=0; VertexNr<CurrentModel.NumVerts; VertexNr++)
            {
                // Der Bone, zu dem dieser Vertex gehört.
                const char BoneNr=VertexInfos[VertexNr];

                // TransformedVertices[VertexNr]=StudioVertices[VertexNr]*TransformedBones[VertexInfos[VertexNr]],
                // wobei hier wieder auf die letzten trivialen [ 0 0 0 1 ] Zeilen der 4x4 Matrizen verzichtet wurde.
                TransformedVertices[VertexNr][0]= StudioVertices[VertexNr][0]*TransformedBones[BoneNr][0][0]
                                                 +StudioVertices[VertexNr][1]*TransformedBones[BoneNr][0][1]
                                                 +StudioVertices[VertexNr][2]*TransformedBones[BoneNr][0][2]
                                                 +                            TransformedBones[BoneNr][0][3];
                TransformedVertices[VertexNr][1]= StudioVertices[VertexNr][0]*TransformedBones[BoneNr][1][0]
                                                 +StudioVertices[VertexNr][1]*TransformedBones[BoneNr][1][1]
                                                 +StudioVertices[VertexNr][2]*TransformedBones[BoneNr][1][2]
                                                 +                            TransformedBones[BoneNr][1][3];
                TransformedVertices[VertexNr][2]= StudioVertices[VertexNr][0]*TransformedBones[BoneNr][2][0]
                                                 +StudioVertices[VertexNr][1]*TransformedBones[BoneNr][2][1]
                                                 +StudioVertices[VertexNr][2]*TransformedBones[BoneNr][2][2]
                                                 +                            TransformedBones[BoneNr][2][3];
            }

            SequenceNrOfTVUpdate=SequenceNr;
            FrameNrOfTVUpdate   =FrameNr;

#if EMIT_DIAGNOSTICS
            printf("XX  TVertices updated\n");
#endif
        }

        // 4.X. Berechne die bzgl. der Bones transformierten Normals (Ergebnisse in 'TransformedNormals').
#if ENABLE_OPT_CACHING
        if (IsUpdateRequired_TransformedNormals)
#else
        if (IsNeeded_TransformedNormals)
#endif
        {
            // Wende die Transformationsmatrix der Bones (Rotation + Translation) auf die StudioNormals an.
            for (int NormalNr=0; NormalNr<CurrentModel.NumNorms; NormalNr++)
            {
                // Der Bone, zu dem dieser Vertex gehört
                const char BoneNr=NormalInfos[NormalNr];

                // TransformedNormals[NormalNr]=StudioNormals[NormalNr]*TransformedBones[VertexInfos[VertexNr]],
                // wobei hier wieder auf die letzten trivialen [ 0 0 0 1 ] Zeilen der 4x4 Matrizen verzichtet wurde,
                // UND nur der linke obere 3x3 Teil für die Rotation (nicht aber die Translation) betrachtet wird!
                TransformedNormals[NormalNr][0]= StudioNormals[NormalNr][0]*TransformedBones[BoneNr][0][0]
                                                +StudioNormals[NormalNr][1]*TransformedBones[BoneNr][0][1]
                                                +StudioNormals[NormalNr][2]*TransformedBones[BoneNr][0][2];
                TransformedNormals[NormalNr][1]= StudioNormals[NormalNr][0]*TransformedBones[BoneNr][1][0]
                                                +StudioNormals[NormalNr][1]*TransformedBones[BoneNr][1][1]
                                                +StudioNormals[NormalNr][2]*TransformedBones[BoneNr][1][2];
                TransformedNormals[NormalNr][2]= StudioNormals[NormalNr][0]*TransformedBones[BoneNr][2][0]
                                                +StudioNormals[NormalNr][1]*TransformedBones[BoneNr][2][1]
                                                +StudioNormals[NormalNr][2]*TransformedBones[BoneNr][2][2];
            }

            SequenceNrOfTNUpdate=SequenceNr;
            FrameNrOfTNUpdate   =FrameNr;

#if EMIT_DIAGNOSTICS
            printf("XX  TNormals updated\n");
#endif
        }

        // 4.Z. Berechne die bzgl. der Bones transformierten Tangents und BiNormals (Ergebnisse in 'TransformedTangents' und 'TransformedBiNormals').
        // Goal: Compute the three vectors that span the tangent/texture space (the "basis").
        // Note that we already know the normal vectors as they are stored in the TransformedNormals array.
        // Also note that we want to keep these normal vectors, and do NOT want to replace them e.g. by SxT.
        // Thus, all we have to do is to augment the TransformedNormals array by computing a TransformedTangents
        // and TransformedBiNormals array.
        // For more details, see NVidias "Texture Space On Real Models" and my diploma thesis.
#if ENABLE_OPT_CACHING
        if (IsUpdateRequired_TransformedTangents)
#else
        if (IsNeeded_TransformedTangents)
#endif
        {
            // In a first step, compute the Tangent and BiNormal for each triangle.
            static Vec3T TriangleTangents [/*MAXSTUDIOVERTS/TRIS???*/ 2048];
            static Vec3T TriangleBiNormals[/*MAXSTUDIOVERTS/TRIS???*/ 2048];

            for (unsigned long TriangleNr=0; TriangleNr<TriangleInfos[BodyPartNr].Size(); TriangleNr++)
            {
                const TriangleInfoT& TI=TriangleInfos[BodyPartNr][TriangleNr];

                // Understanding what's going on here is easy. The key statement is
                // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
                // First, there is a short explanation in "The Cg Tutorial", chapter 8.
                // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations.
                const float* Vertices[3]={ TransformedVertices[TI.Vertices[0][0]], TransformedVertices[TI.Vertices[1][0]], TransformedVertices[TI.Vertices[2][0]] };

                const float  Span01  [5]={ Vertices[1][0]-Vertices[0][0], Vertices[1][1]-Vertices[0][1], Vertices[1][2]-Vertices[0][2], float(TI.Vertices[1][2]-TI.Vertices[0][2]), float(TI.Vertices[1][3]-TI.Vertices[0][3]) };
                const float  Span02  [5]={ Vertices[2][0]-Vertices[0][0], Vertices[2][1]-Vertices[0][1], Vertices[2][2]-Vertices[0][2], float(TI.Vertices[2][2]-TI.Vertices[0][2]), float(TI.Vertices[2][3]-TI.Vertices[0][3]) };

                const float  f          =Span01[3]*Span02[4]-Span01[4]*Span02[3]>0.0 ? 1.0f : -1.0f;

                const float  Tangent [3]={ -Span01[4]*Span02[0]+Span01[0]*Span02[4], -Span01[4]*Span02[1]+Span01[1]*Span02[4], -Span01[4]*Span02[2]+Span01[2]*Span02[4] };
                const float  BiNormal[3]={  Span01[3]*Span02[0]-Span01[0]*Span02[3],  Span01[3]*Span02[1]-Span01[1]*Span02[3],  Span01[3]*Span02[2]-Span01[2]*Span02[3] };

                const float  TangentL2  =Tangent [0]*Tangent [0] + Tangent [1]*Tangent [1] + Tangent [2]*Tangent [2];
                const float  BiNormalL2 =BiNormal[0]*BiNormal[0] + BiNormal[1]*BiNormal[1] + BiNormal[2]*BiNormal[2];

                if (TangentL2>0.000001)
                {
                    const float OneDivTangentL=f/sqrt(TangentL2);

                    TriangleTangents[TriangleNr][0]=Tangent[0]*OneDivTangentL;
                    TriangleTangents[TriangleNr][1]=Tangent[1]*OneDivTangentL;
                    TriangleTangents[TriangleNr][2]=Tangent[2]*OneDivTangentL;
                }
                else
                {
                    // Should never occur...
                    TriangleTangents[TriangleNr][0]=1.0;
                    TriangleTangents[TriangleNr][1]=0.0;
                    TriangleTangents[TriangleNr][2]=0.0;
                }

                if (BiNormalL2>0.000001)
                {
                    const float OneDivBiNormalL=f/sqrt(BiNormalL2);

                    TriangleBiNormals[TriangleNr][0]=BiNormal[0]*OneDivBiNormalL;
                    TriangleBiNormals[TriangleNr][1]=BiNormal[1]*OneDivBiNormalL;
                    TriangleBiNormals[TriangleNr][2]=BiNormal[2]*OneDivBiNormalL;
                }
                else
                {
                    // Should never occur...
                    TriangleBiNormals[TriangleNr][0]=0.0;
                    TriangleBiNormals[TriangleNr][1]=1.0;
                    TriangleBiNormals[TriangleNr][2]=0.0;
                }
            }

            // Then compute averages of the tangents and binormals.
            for (int NormalNr=0; NormalNr<CurrentModel.NumNorms; NormalNr++)
            {
                int i;

                for (i=0; i<3; i++) TransformedTangents [NormalNr][i]=0.0;
                for (i=0; i<3; i++) TransformedBiNormals[NormalNr][i]=0.0;

                for (unsigned long TriangleCount=0; TriangleCount<TrianglesReferingToNormal[BodyPartNr][NormalNr].Size(); TriangleCount++)
                {
                    const unsigned long TriangleNr=TrianglesReferingToNormal[BodyPartNr][NormalNr][TriangleCount];

                    for (i=0; i<3; i++) TransformedTangents [NormalNr][i]+=TriangleTangents [TriangleNr][i];
                    for (i=0; i<3; i++) TransformedBiNormals[NormalNr][i]+=TriangleBiNormals[TriangleNr][i];
                }

                float TangentL2 =0.0; for (i=0; i<3; i++) TangentL2 +=TransformedTangents [NormalNr][i]*TransformedTangents [NormalNr][i];
                float BiNormalL2=0.0; for (i=0; i<3; i++) BiNormalL2+=TransformedBiNormals[NormalNr][i]*TransformedBiNormals[NormalNr][i];

                if (TangentL2 >0.000001) { const float OneDivTangentL =1.0f/sqrt(TangentL2 ); for (i=0; i<3; i++) TransformedTangents [NormalNr][i]*=OneDivTangentL;  }
                if (BiNormalL2>0.000001) { const float OneDivBiNormalL=1.0f/sqrt(BiNormalL2); for (i=0; i<3; i++) TransformedBiNormals[NormalNr][i]*=OneDivBiNormalL; }
            }

            SequenceNrOfTTUpdate=SequenceNr;
            FrameNrOfTTUpdate   =FrameNr;

#if EMIT_DIAGNOSTICS
            printf("XX  TTangents updated\n");
#endif
        }


        // If this is a shadow pass, things work a little different than on ambient or light passes.
        if (MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::STENCILSHADOW)
        {
            static ArrayT<bool> TriangleIsFrontFacing;

            if (TriangleIsFrontFacing.Size()==0) TriangleIsFrontFacing.PushBackEmpty(2000);
            if (TriangleIsFrontFacing.Size()<TriangleInfos[BodyPartNr].Size()) TriangleIsFrontFacing.PushBackEmpty(TriangleInfos[BodyPartNr].Size()-TriangleIsFrontFacing.Size());

            for (unsigned long TriNr=0; TriNr<TriangleInfos[BodyPartNr].Size(); TriNr++)
            {
                const TriangleInfoT& T=TriangleInfos[BodyPartNr][TriNr];

                if (Materials[T.MaterialIndex]==NULL || Materials[T.MaterialIndex]->NoShadows)
                {
                    // Triangles whose materials don't cast shadows are treated as back-facing.
                    TriangleIsFrontFacing[TriNr]=false;
                    continue;
                }

                const float* Vertices[3]={ TransformedVertices[T.Vertices[0][0]], TransformedVertices[T.Vertices[1][0]], TransformedVertices[T.Vertices[2][0]] };
                const float  Span01  [3]={ Vertices[1][0]-Vertices[0][0], Vertices[1][1]-Vertices[0][1], Vertices[1][2]-Vertices[0][2] };
                const float  Span02  [3]={ Vertices[2][0]-Vertices[0][0], Vertices[2][1]-Vertices[0][1], Vertices[2][2]-Vertices[0][2] };
                const float  Normal  [3]={ Span02[1]*Span01[2]-Span02[2]*Span01[1], Span02[2]*Span01[0]-Span02[0]*Span01[2], Span02[0]*Span01[1]-Span02[1]*Span01[0] };
                const float  LightDist  =(Light_Pos[0]-Vertices[0][0])*Normal[0] + (Light_Pos[1]-Vertices[0][1])*Normal[1] + (Light_Pos[2]-Vertices[0][2])*Normal[2];

                TriangleIsFrontFacing[TriNr]=LightDist<0;   // Front-facing triangles in OpenGL are ordered CCW (contrary to Ca3DE)!
            }


            // Note that we have to cull the following polygons wrt. the *VIEWER* (not the light source)!
            static MatSys::MeshT MeshSilhouette(MatSys::MeshT::Quads);

            MeshSilhouette.Vertices.Overwrite();

            for (unsigned long TriNr=0; TriNr<TriangleInfos[BodyPartNr].Size(); TriNr++)
            {
                if (!TriangleIsFrontFacing[TriNr]) continue;

                // This triangle is front-facing wrt. the light source.
                const TriangleInfoT& T=TriangleInfos[BodyPartNr][TriNr];
                const float* Vertices[3]={ TransformedVertices[T.Vertices[0][0]], TransformedVertices[T.Vertices[1][0]], TransformedVertices[T.Vertices[2][0]] };

                for (unsigned long EdgeNr=0; EdgeNr<3; EdgeNr++)
                {
                    const bool IsSilhouetteEdge=(T.Neighbours[EdgeNr]<0) ? true : !TriangleIsFrontFacing[T.Neighbours[EdgeNr]];

                    if (IsSilhouetteEdge)
                    {
                        // The neighbour at edge 'EdgeNr' is back-facing (or non-existant), so we have found a possible silhouette edge.
                        const int   v1   =EdgeNr;
                        const int   v2   =(EdgeNr+1) % 3;
                        const float LA[4]={ Vertices[v1][0]-Light_Pos[0], Vertices[v1][1]-Light_Pos[1], Vertices[v1][2]-Light_Pos[2], 0.0 };
                        const float LB[4]={ Vertices[v2][0]-Light_Pos[0], Vertices[v2][1]-Light_Pos[1], Vertices[v2][2]-Light_Pos[2], 0.0 };

                        MeshSilhouette.Vertices.PushBackEmpty(4);

                        const unsigned long MeshSize=MeshSilhouette.Vertices.Size();

                        MeshSilhouette.Vertices[MeshSize-4].SetOrigin(Vertices[v2][0], Vertices[v2][1], Vertices[v2][2]);
                        MeshSilhouette.Vertices[MeshSize-3].SetOrigin(Vertices[v1][0], Vertices[v1][1], Vertices[v1][2]);
                        MeshSilhouette.Vertices[MeshSize-2].SetOrigin(LA[0], LA[1], LA[2], LA[3]);
                        MeshSilhouette.Vertices[MeshSize-1].SetOrigin(LB[0], LB[1], LB[2], LB[3]);
                    }
                }
            }

            if (MeshSilhouette.Vertices.Size()>0) MatSys::Renderer->RenderMesh(MeshSilhouette);


            static MatSys::MeshT MeshCaps(MatSys::MeshT::Triangles);

            MeshCaps.Vertices.Overwrite();

            for (unsigned long TriNr=0; TriNr<TriangleInfos[BodyPartNr].Size(); TriNr++)
            {
                if (!TriangleIsFrontFacing[TriNr]) continue;

                // This triangle is front-facing wrt. the light source.
                const TriangleInfoT& T=TriangleInfos[BodyPartNr][TriNr];
                const float* Vertices[3]={ TransformedVertices[T.Vertices[0][0]], TransformedVertices[T.Vertices[1][0]], TransformedVertices[T.Vertices[2][0]] };

                MeshCaps.Vertices.PushBackEmpty(6);

                const unsigned long MeshSize=MeshCaps.Vertices.Size();

                // Render the occluder (front-facing wrt. the light source).
                MeshCaps.Vertices[MeshSize-6].SetOrigin(Vertices[0][0], Vertices[0][1], Vertices[0][2]);
                MeshCaps.Vertices[MeshSize-5].SetOrigin(Vertices[1][0], Vertices[1][1], Vertices[1][2]);
                MeshCaps.Vertices[MeshSize-4].SetOrigin(Vertices[2][0], Vertices[2][1], Vertices[2][2]);

                // Render the occluder (back-facing wrt. the light source).
                MeshCaps.Vertices[MeshSize-3].SetOrigin(Vertices[2][0]-Light_Pos[0], Vertices[2][1]-Light_Pos[1], Vertices[2][2]-Light_Pos[2], 0.0);
                MeshCaps.Vertices[MeshSize-2].SetOrigin(Vertices[1][0]-Light_Pos[0], Vertices[1][1]-Light_Pos[1], Vertices[1][2]-Light_Pos[2], 0.0);
                MeshCaps.Vertices[MeshSize-1].SetOrigin(Vertices[0][0]-Light_Pos[0], Vertices[0][1]-Light_Pos[1], Vertices[0][2]-Light_Pos[2], 0.0);
            }

            if (MeshCaps.Vertices.Size()>0) MatSys::Renderer->RenderMesh(MeshCaps);
            continue;
        }


        // 4.2. Zeichne die Triangle-Meshes
        for (int MeshNr=0; MeshNr<CurrentModel.NumMesh; MeshNr++)
        {
            const StudioMeshT&  CurrentMesh   =StudioMeshes[MeshNr];
            const signed short* TriangleStrips=(signed short*)(&ModelData[0]+CurrentMesh.TriIndex);

         // const int   TextureFlags=StudioTextures[SkinRefs[CurrentMesh.SkinRef]].Flags;
            const float s=1.0f/(float)StudioTextures[SkinRefs[CurrentMesh.SkinRef]].Width;
            const float t=1.0f/(float)StudioTextures[SkinRefs[CurrentMesh.SkinRef]].Height;


            MatSys::Renderer->SetCurrentMaterial(RenderMaterials[SkinRefs[CurrentMesh.SkinRef]]);

            while (*TriangleStrips)
            {
                // Anzahl und Art (FAN vs. STRIP) der Vertices im nächsten TriangleStrip.
                signed short NrOfVertices=*(TriangleStrips++);

                static MatSys::MeshT Mesh;

                Mesh.Type=NrOfVertices<0 ? MatSys::MeshT::TriangleFan : MatSys::MeshT::TriangleStrip;

                if (NrOfVertices<0) NrOfVertices=-NrOfVertices;

                Mesh.Vertices.Overwrite();
                Mesh.Vertices.PushBackEmpty(NrOfVertices);

                for (unsigned long VertexNr=0; VertexNr<(unsigned long)NrOfVertices; VertexNr++, TriangleStrips+=4)
                {
                    MatSys::MeshT::VertexT& V=Mesh.Vertices[VertexNr];

                    // TriangleStrips[0] -- Index ins (Transformed-)Vertices-Array
                    // TriangleStrips[1] -- Index ins (Transformed-)Normals-Array
                    // TriangleStrips[2] -- s-Texture-Coord (in Pixel)
                    // TriangleStrips[3] -- t-Texture-Coord (in Pixel)

                    V.SetOrigin      (TransformedVertices[TriangleStrips[0]][0], TransformedVertices[TriangleStrips[0]][1], TransformedVertices[TriangleStrips[0]][2]);
                    V.SetTextureCoord(TriangleStrips[2]*s, TriangleStrips[3]*t);

                    if (IsNeeded_TransformedNormals ) V.SetNormal  (TransformedNormals  [TriangleStrips[1]][0], TransformedNormals  [TriangleStrips[1]][1], TransformedNormals  [TriangleStrips[1]][2]);
                    if (IsNeeded_TransformedTangents) V.SetTangent (TransformedTangents [TriangleStrips[1]][0], TransformedTangents [TriangleStrips[1]][1], TransformedTangents [TriangleStrips[1]][2]);
                    if (IsNeeded_TransformedTangents) V.SetBiNormal(TransformedBiNormals[TriangleStrips[1]][0], TransformedBiNormals[TriangleStrips[1]][1], TransformedBiNormals[TriangleStrips[1]][2]);
                }

                MatSys::Renderer->RenderMesh(Mesh);
            }
        }
    }


    // 5. Zeichne ein eventuelles Sub-Model
    // ************************************

    if (SubModel!=NULL)
    {
        // Currently, we MUST pass-in 0 for 'SequenceNr' and 0.0 for 'FrameNr',
        // because there is no range-check for these values in the (recursively called) Draw() function.
        // The other parameters we pass-in indicate that there is no sub-sub-model, but a super-model.
        SubModel->DoDraw(0, 0.0, NULL, StudioHeader->NumBones, StudioBones, TransformedBones);
    }
}


/***********************************************/
/*** Implementation of the ModelT interface. ***/
/***********************************************/

const std::string& ModelMdlT::GetFileName() const
{
    return FileName;
}


void ModelMdlT::Draw(int SequenceNr, float FrameNr, float /*LodDist*/, const ModelT* SubModel) const
{
    const ModelMdlT* SubMdl=NULL;

    if (SubModel)
    {
     // if (!SubMdl)
        {
            const ModelProxyT* ProxySubModel=dynamic_cast<const ModelProxyT*>(SubModel);
            if (ProxySubModel) SubMdl=dynamic_cast<const ModelMdlT*>(ProxySubModel->GetRealModel());
        }

        if (!SubMdl)
        {
            SubMdl=dynamic_cast<const ModelMdlT*>(SubModel);
        }
    }

    if (SequenceNr>=GetNrOfSequences()) SequenceNr=0;

    DoDraw(SequenceNr, FrameNr, SubMdl, 0, NULL, NULL);
}


bool ModelMdlT::GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    // To be implemented... (is it worth the effort??)
    return false;
}


void ModelMdlT::Print() const
{
    struct HelperT
    {
        char PrintBuffer[256];

        const char* GetStr(const float* v, const char Anz)
        {
                 if (Anz==3) sprintf(PrintBuffer, "%8.2f %8.2f %8.2f"                  , v[0], v[1], v[2]                  );
            else if (Anz==4) sprintf(PrintBuffer, "%8.2f %8.2f %8.2f %8.2f"            , v[0], v[1], v[2], v[3]            );
            else if (Anz==6) sprintf(PrintBuffer, "%8.2f %8.2f %8.2f %8.2f %8.2f %8.2f", v[0], v[1], v[2], v[3], v[4], v[5]);
            else PrintBuffer[0]=0;

            return PrintBuffer;
        }
    } Helper;


    printf("   HEADER            (size %lu bytes)\n", long(sizeof(StudioHeaderT)));
    printf("************\n");
    printf("\n");
    printf("ID      : %.4s\n", (char*)&StudioHeader->ID);
    printf("Version : %u\n"  ,  StudioHeader->Version);
    printf("Name    : %s\n"  ,  StudioHeader->Name   );
    printf("FileSize: %u\n"  ,  StudioHeader->Length );
    printf("\n");
    printf("EyePosition: %s\n", Helper.GetStr(StudioHeader->EyePosition, 3));
    printf("Hull    Min: %s\n", Helper.GetStr(StudioHeader->  Min, 3));
    printf("Hull    Max: %s\n", Helper.GetStr(StudioHeader->  Max, 3));
    printf("BB      Min: %s\n", Helper.GetStr(StudioHeader->BBMin, 3));
    printf("BB      Max: %s\n", Helper.GetStr(StudioHeader->BBMax, 3));
    printf("\n");
    printf("Flags : 0x%X\n", StudioHeader->Flags);
    printf("\n");
    printf("NumBones           : %2d  ", StudioHeader->NumBones);           printf("BoneIndex          : %2d\n", StudioHeader->BoneIndex);
    printf("NumBoneControllers : %2d  ", StudioHeader->NumBoneControllers); printf("BoneControllerIndex: %2d\n", StudioHeader->BoneControllerIndex);
    printf("NumHitBoxes        : %2d  ", StudioHeader->NumHitBoxes);        printf("HitBoxIndex        : %2d\n", StudioHeader->HitBoxIndex);
    printf("NumSequences       : %2d  ", StudioHeader->NumSeq);             printf("SequenceIndex      : %2d\n", StudioHeader->SeqIndex);
    printf("NumSeqGroups       : %2d  ", StudioHeader->NumSeqGroups);       printf("SeqGroupIndex      : %2d\n", StudioHeader->SeqGroupIndex);
    printf("NumBodyParts       : %2d  ", StudioHeader->NumBodyParts);       printf("BodyPartIndex      : %2d\n", StudioHeader->BodyPartIndex);
    printf("NumAttachments     : %2d  ", StudioHeader->NumAttachments);     printf("AttachmentIndex    : %2d\n", StudioHeader->AttachmentIndex);
    printf("NumTransitions     : %2d  ", StudioHeader->NumTransitions);     printf("TransitionIndex    : %2d\n", StudioHeader->TransitionIndex);
    printf("NumTextures        : %2d  ", StudioHeader->NumTextures);        printf("TextureIndex       : %2d\n", StudioHeader->TextureIndex);
    printf("                         ");                                    printf("TextureDataIndex   : %2d\n", StudioHeader->TextureDataIndex);
    printf("NumSkinRef         : %2d\n", StudioHeader->NumSkinRef);
    printf("NumSkinFamilies    : %2d  ", StudioHeader->NumSkinFamilies);    printf("SkinIndex          : %2d\n", StudioHeader->SkinIndex);
    printf("SoundTable         : %2d  ", StudioHeader->SoundTable);         printf("SoundIndex         : %2d\n", StudioHeader->SoundIndex);
    printf("SoundGroups        : %2d  ", StudioHeader->SoundGroups);        printf("SoundGroupIndex    : %2d\n", StudioHeader->SoundGroupIndex);
    printf("\n");
    printf("\n");
    printf("   BONES             (size %lu bytes)\n", long(sizeof(StudioBoneT)));
    printf("***********\n");
    printf("\n");
    int i;
    for (i=0; i<StudioHeader->NumBones; i++)
    {
        // Achtung! Darf Helper.GetStr() nicht mehrfach in printf("...", Helper.GetStr(), Helper.GetStr()) aufrufen!
        char ValueString[256]; strcpy(ValueString, Helper.GetStr(StudioBones[i].Value, 6));
        char ScaleString[256]; strcpy(ScaleString, Helper.GetStr(StudioBones[i].Scale, 6));

        printf("%2d%32s %2d 0x%08X | %2d %2d %2d %2d %2d %2d | %s | %s\n", i, StudioBones[i].Name, StudioBones[i].Parent, StudioBones[i].Flags,
                StudioBones[i].BoneController[0], StudioBones[i].BoneController[1], StudioBones[i].BoneController[2], StudioBones[i].BoneController[3], StudioBones[i].BoneController[4], StudioBones[i].BoneController[5],
                ValueString, ScaleString);
    }
    printf("\n");
    printf("\n");
    printf("   BONE CONTROLLERS             (size %lu bytes)\n", long(sizeof(StudioBoneControllerT)));
    printf("**********************\n");
    printf("\n");
    for (i=0; i<StudioHeader->NumBoneControllers; i++)
    {
        printf("%2d, Bone %2u, Type 0x%08X, Start %8.2f, End %8.2f, Rest %2d, Index %2d\n", i, StudioBoneControllers[i].Bone, StudioBoneControllers[i].Type, StudioBoneControllers[i].Start, StudioBoneControllers[i].End, StudioBoneControllers[i].Rest, StudioBoneControllers[i].Index);
    }
    printf("\n");
    printf("\n");
    printf("   SEQUENCES             (size %lu bytes)\n", long(sizeof(StudioSequenceT)));
    printf("***************\n");
    printf("\n");
    for (i=0; i<StudioHeader->NumSeq; i++)
    {
        const StudioSequenceT& S=StudioSequences[i];

        printf("%2d%32s, ", i, S.Label);
        printf("FPS %4.1f, Flags%2u, Activity %2u, ActWeight%2u, ", S.FPS, S.Flags, S.Activity, S.ActWeight);
        printf("#Events%2u, EventIdx %2u, #Frames %2u, #Pivots%2u, PivotIdx%7u, ", S.NumEvents, S.EventIndex, S.NumFrames, S.NumPivots, S.PivotIndex);
        printf("MotionT %2u, MBone%2u, LinMvmnt (%6.2f, %3.1f, %3.1f), ", S.MotionType, S.MotionBone, S.LinearMovement[0], S.LinearMovement[1], S.LinearMovement[2]);
        printf("AMPI%2u, AMAI%2u, ", S.AutoMovePosIndex, S.AutoMoveAngleIndex);
        printf("BB (%7.3f %7.3f %7.3f)-", S.BBMin[0], S.BBMin[1], S.BBMin[2]);
        printf("(%7.3f %7.3f %7.3f), ", S.BBMax[0], S.BBMax[1], S.BBMax[2]);
        printf("#Blends%2u, AnimIdx%7u, BType %2u %2u, ", S.NumBlends, S.AnimIndex, S.BlendType[0], S.BlendType[1]);
        printf("BStart %5.1f %3.1f, BEnd %5.1f %3.1f, ", S.BlendStart[0], S.BlendStart[1], S.BlendEnd[0], S.BlendEnd[1]);
        printf("BParent%2u, SeqGroup%2u, EntryNode%2u, ExitNode%2u, NodeFlags%2u, NextSeq%2u\n", S.BlendParent, S.SeqGroup, S.EntryNode, S.ExitNode, S.NodeFlags, S.NextSeq);

        // Anim zur Sequence betrachten
        const StudioSequenceGroupT& SG=StudioSequenceGroups[S.SeqGroup];

        if (S.SeqGroup==0)
        {
            StudioAnimT* StudioAnim=(StudioAnimT*)(&ModelData[0]+SG.Data+S.AnimIndex);

            // Das StudioAnim Array hat genauso viele Einträge, wie es Knochen gibt!
            for (int b=0; b<StudioHeader->NumBones; b++)
            {
                printf("    Bone %2u, Anim.Offsets ", b);
                for (int c=0; c<6; c++)
                {
                    StudioAnimValueT AV=*(StudioAnimValueT*)(&StudioAnim[b]+StudioAnim[b].Offset[c]);

                    printf("%4u (%3u %3u %5d) ", StudioAnim[b].Offset[c], AV.Num.Valid, AV.Num.Total, AV.Value);
                }
                printf("\n");
            }
        }
        else printf("ERROR! S.SeqGroup>0 !\n");
    }
    printf("\n");
    printf("\n");
    printf("   SEQUENCE GROUPS             (size %lu bytes)\n", long(sizeof(StudioSequenceGroupT)));
    printf("*********************\n");
    printf("\n");
    for (i=0; i<StudioHeader->NumSeqGroups; i++)
    {
        const StudioSequenceGroupT& SG=StudioSequenceGroups[i];

        printf("%2d%32s %64s    Data %4u\n", i, SG.Label, SG.Name, SG.Data);
    }
    printf("\n");
    printf("\n");
    printf("   TEXTURES             (size %lu bytes)\n", long(sizeof(StudioTextureT)));
    printf("**************\n");
    printf("\n");
    for (i=0; i<StudioHeader->NumTextures; i++)
    {
        const StudioTextureT& Tex=StudioTextures[i];

        printf("%2d%64s 0x%08X, %3ux%3u, Index %8u\n", i, Tex.Name, Tex.Flags, Tex.Width, Tex.Height, Tex.Index);
    }
    printf("\n");
    printf("\n");
    printf("   BODYPARTS             (size %lu bytes)\n", long(sizeof(StudioBodyPartT)));
    printf("***************\n");
    printf("\n");
    for (i=0; i<StudioHeader->NumBodyParts; i++)
    {
        const StudioBodyPartT& BP=StudioBodyParts[i];

        printf("%2d%64s, NumModels %4u, Base %4d, ModelIndex %8u\n", i, BP.Name, BP.NumModels, BP.Base, BP.ModelIndex);

        // Die Models für diesen BodyPart BP
        const StudioModelT* StudioModels=(StudioModelT*)(&ModelData[0]+BP.ModelIndex);

        printf("\n");
        printf("       MODELS             (size %lu bytes)\n", long(sizeof(StudioModelT)));
        printf("    ************\n");
        for (int j=0; j<BP.NumModels; j++)
        {
            const StudioModelT& M=StudioModels[j];

            printf("    Model %2d%64s, Type %4u, BoundingRadius %8.2f, NumMesh %4u, MeshIndex %8u, NumGroups %4u, GroupIndex %8u\n", j, M.Name, M.Type, M.BoundingRadius, M.NumMesh, M.MeshIndex, M.NumGroups, M.GroupIndex);
            printf("    NumVerts %4u, VertInfoIndex %8u, VertIndex %8u, NumNorms %4u, NormInfoIndex %8u, NormIndex %8u\n", M.NumVerts, M.VertInfoIndex, M.VertIndex, M.NumNorms, M.NormInfoIndex, M.NormIndex);

            // Die Meshes für dieses Model.
            const StudioMeshT* Meshes=(StudioMeshT*)(&ModelData[0]+M.MeshIndex);

            printf("\n");
            printf("           MESHES             (size %lu bytes)\n", long(sizeof(StudioMeshT)));
            printf("        ************\n");
            for (int MeshNr=0; MeshNr<M.NumMesh; MeshNr++)
            {
                printf("        NumTris %4u, TriIndex %8u, SkinRef %4u, NumNorms %4u, NormIndex %8u\n", Meshes[MeshNr].NumTris, Meshes[MeshNr].TriIndex, Meshes[MeshNr].SkinRef, Meshes[MeshNr].NumNorms_UNUSED, Meshes[MeshNr].NormIndex_UNUSED);

                const signed short* TriangleStrips=(signed short*)(&ModelData[0]+Meshes[MeshNr].TriIndex);


                while (*TriangleStrips)
                {
                    // Anzahl und Art (FAN vs. STRIP) der Vertices im nächsten TriangleStrip.
                    signed short NrOfVertices=*(TriangleStrips++);

                    if (NrOfVertices<0)
                    {
                        printf("FAN  ");
                        NrOfVertices=-NrOfVertices;
                    }
                    else printf("STRIP");

                    printf(" %4u: ", NrOfVertices);

                    for(; NrOfVertices>0; NrOfVertices--, TriangleStrips+=4)
                    {
                        // TriangleStrips[0] -- Index ins (Transformed-)Vertices-Array
                        // TriangleStrips[1] -- Index ins Normals-Array
                        // TriangleStrips[2] -- Multiplikator für s-Texture-Coord (?)
                        // TriangleStrips[3] -- Multiplikator für t-Texture-Coord (?)
                        printf("(%3u, %3u) ", TriangleStrips[0], TriangleStrips[1]);
                    }

                    printf("\n");
                }
            }
        }
    }
    printf("\n");
    printf("\n");

    printf("   ATTACHMENTS             (size %lu bytes)\n", long(sizeof(StudioAttachmentT)));
    printf("*****************\n");
    printf("\n");
    for (i=0; i<StudioHeader->NumAttachments; i++)
    {
        const StudioAttachmentT& At=StudioAttachments[i];

        // Achtung! Darf Helper.GetStr() nicht mehrfach in printf("...", Helper.GetStr(), Helper.GetStr()) aufrufen!
        char OrgString    [256]; strcpy(OrgString    , Helper.GetStr(At.Org,        3));
        char Vector1String[256]; strcpy(Vector1String, Helper.GetStr(At.Vectors[0], 3));
        char Vector2String[256]; strcpy(Vector2String, Helper.GetStr(At.Vectors[1], 3));
        char Vector3String[256]; strcpy(Vector3String, Helper.GetStr(At.Vectors[2], 3));

        printf("%2d%64s, Type %4u, Bone %4d, Origin %s, Vec0 %s, Vec1 %s, Vec2 %s\n", i, At.Name, At.Type, At.Bone, OrgString, Vector1String, Vector2String, Vector3String);
    }
}


int ModelMdlT::GetNrOfSequences() const
{
    return StudioHeader->NumSeq;
}


const float* ModelMdlT::GetSequenceBB(int SequenceNr, float /*FrameNr*/) const
{
    static float BB[6];

    if (SequenceNr<0 || SequenceNr>=StudioHeader->NumSeq)
    {
        BB[0]=0.0;
        BB[1]=0.0;
        BB[2]=0.0;

        BB[3]=0.0;
        BB[4]=0.0;
        BB[5]=0.0;
    }
    else
    {
        BB[0]=StudioSequences[SequenceNr].BBMin[0];
        BB[1]=StudioSequences[SequenceNr].BBMin[1];
        BB[2]=StudioSequences[SequenceNr].BBMin[2];

        BB[3]=StudioSequences[SequenceNr].BBMax[0];
        BB[4]=StudioSequences[SequenceNr].BBMax[1];
        BB[5]=StudioSequences[SequenceNr].BBMax[2];
    }

    return BB;
}


/* float ModelMdlT::GetNrOfFrames(int SequenceNr) const
{
    if (IsDefaultModel) return 0;   // WICHTIG: Dies muß unbedingt als ERSTES hier stehen!

    int NumFrames=StudioSequences[SequenceNr].NumFrames;
    if (NumFrames<=1) return 0;

    // Return 1/100th sec before the end of the last frame.
    return float(NumFrames-1)-0.01;
} */


float ModelMdlT::AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop) const
{
    int NumFrames=StudioSequences[SequenceNr].NumFrames;
    if (NumFrames<=1) return 0;

    float NewFrameNr=FrameNr+DeltaTime*StudioSequences[SequenceNr].FPS;

    if (Loop)
    {
        // Wrap the sequence (it's a looping (repeating) sequence, like idle, walk, ...).
        NewFrameNr-=(int)(NewFrameNr/(NumFrames-1))*(NumFrames-1);
    }
    else
    {
        // Clamp the sequence (it's a play-once (non-repeating) sequence, like dying).
        // On clamping, stop the sequence 1/100th sec before the end of the last frame.
        if (NewFrameNr>=float(NumFrames-1)) NewFrameNr=float(NumFrames-1)-0.01f;
    }

    return NewFrameNr;
}
