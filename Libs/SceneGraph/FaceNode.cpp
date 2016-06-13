/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "FaceNode.hpp"
#include "_aux.hpp"
#include "LightMapMan.hpp"
#include "SHLMapMan.hpp"
#include "Bitmap/Bitmap.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"

#include <cassert>

using namespace cf::SceneGraph;


const double FaceNodeT::ROUND_EPSILON = 0.08;       // Make sure that this matches MapT::RoundEpsilon in Common/World.hpp!


FaceNodeT::FaceNodeT(LightMapManT& LMM, SHLMapManT& SMM)
    : Polygon(),
      Material(NULL),
      TI(),
      LightMapInfo(),
      SHLMapInfo(),
      LightMapMan(LMM),
      SHLMapMan(SMM),
      BB(),
      DrawIndices(),
      Mesh(),
      RenderMat(NULL)
{
}


FaceNodeT::FaceNodeT(LightMapManT& LMM, SHLMapManT& SMM, const Polygon3T<double>& Poly_, MaterialT* Material_, const TexInfoT& TI_)
    : Polygon(Poly_),
      Material(Material_),
      TI(TI_),
      LightMapInfo(),
      SHLMapInfo(),
      LightMapMan(LMM),
      SHLMapMan(SMM),
      BB(),
      DrawIndices(),
      Mesh(),
      RenderMat(NULL)
{
}


FaceNodeT::FaceNodeT(const FaceNodeT& Other)
    : Polygon(Other.Polygon),
      Material(Other.Material),
      TI(Other.TI),
      LightMapInfo(Other.LightMapInfo),
      SHLMapInfo(Other.SHLMapInfo),
      LightMapMan(Other.LightMapMan),
      SHLMapMan(Other.SHLMapMan),
      BB(Other.BB),
      DrawIndices(Other.DrawIndices),
      Mesh(Other.Mesh),
      RenderMat(NULL)
{
    if (Other.RenderMat!=NULL)
    {
        RenderMat=MatSys::Renderer->RegisterMaterial(Other.Material);
    }
}


FaceNodeT* FaceNodeT::CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, LightMapManT& LMM, SHLMapManT& SMM)
{
    FaceNodeT* FN=new FaceNodeT(LMM, SMM);

    FN->Polygon.Plane.Normal=Pool.ReadVector3d(InFile);
    InFile.read((char*)&FN->Polygon.Plane.Dist, sizeof(FN->Polygon.Plane.Dist));

    for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
    {
        FN->Polygon.Vertices.PushBack(Pool.ReadVector3d(InFile));
    }

    std::string MaterialName=Pool.ReadString(InFile);
    FN->Material=MaterialManager->GetMaterial(MaterialName);
    if (FN->Material==NULL)
    {
        printf("Material %s not found!\n", MaterialName.c_str());
        delete FN;
        return NULL;
    }

    // Texture Information
    FN->TI.U=Pool.ReadVector3f(InFile);
    FN->TI.V=Pool.ReadVector3f(InFile);
    InFile.read((char*)&FN->TI.OffsetU, sizeof(FN->TI.OffsetU));
    InFile.read((char*)&FN->TI.OffsetV, sizeof(FN->TI.OffsetV));

    // LightMaps
    {
        LightMapInfoT& LMI=FN->LightMapInfo;

        InFile.read((char*)&LMI.SizeS, sizeof(LMI.SizeS));
        InFile.read((char*)&LMI.SizeT, sizeof(LMI.SizeT));

        unsigned long LmNr;
        unsigned int  LmPosS;
        unsigned int  LmPosT;

        if (!FN->LightMapMan.Allocate(LMI.SizeS, LMI.SizeT, LmNr, LmPosS, LmPosT))
        {
            printf("Could not allocate LightMap.\n");
            delete FN;
            return NULL;
        }

        LMI.LightMapNr=(unsigned short)LmNr;
        LMI.PosS      =(unsigned short)LmPosS;
        LMI.PosT      =(unsigned short)LmPosT;

        for (unsigned int t=0; t<LMI.SizeT; t++)
            for (unsigned int s=0; s<LMI.SizeS; s++)
            {
                char Red, Green, Blue;

                InFile.read(&Red  , sizeof(Red  ));
                InFile.read(&Green, sizeof(Green));
                InFile.read(&Blue , sizeof(Blue ));

                FN->LightMapMan.Bitmaps[LMI.LightMapNr]->SetPixel(LMI.PosS+s, LMI.PosT+t, Red, Green, Blue);
            }

        for (unsigned int t=0; t<LMI.SizeT; t++)
            for (unsigned int s=0; s<LMI.SizeS; s++)
            {
                char Red, Green, Blue, Alpha;

                InFile.read(&Red  , sizeof(Red  ));
                InFile.read(&Green, sizeof(Green));
                InFile.read(&Blue , sizeof(Blue ));
                InFile.read(&Alpha, sizeof(Alpha));

                FN->LightMapMan.Bitmaps2[LMI.LightMapNr]->SetPixel(LMI.PosS+s, LMI.PosT+t, Red, Green, Blue, Alpha);
            }
    }

    // SHLMaps
    {
        SHLMapInfoT& SMI=FN->SHLMapInfo;

        InFile.read((char*)&SMI.SizeS, sizeof(SMI.SizeS));
        InFile.read((char*)&SMI.SizeT, sizeof(SMI.SizeT));

        unsigned long shlNr;
        unsigned long shlPosS;
        unsigned long shlPosT;

        if (!FN->SHLMapMan.Allocate(SMI.SizeS, SMI.SizeT, shlNr, shlPosS, shlPosT))
        {
            printf("Could not allocate SHLMap.\n");
            delete FN;
            return NULL;
        }

        SMI.SHLMapNr=(unsigned short)shlNr;
        SMI.PosS    =(unsigned short)shlPosS;
        SMI.PosT    =(unsigned short)shlPosT;

        const unsigned long NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;

        for (unsigned long t=0; t<SMI.SizeT; t++)
            for (unsigned long s=0; s<SMI.SizeS; s++)
                if (cf::SceneGraph::SHLMapManT::NrOfRepres>0)
                {
                    // Compressed, read the indices.
                    InFile.read((char*)&FN->SHLMapMan.SHLMaps[SMI.SHLMapNr]->Indices[(SMI.PosT+t)*cf::SceneGraph::SHLMapManT::SIZE_S+SMI.PosS+s], sizeof(unsigned short));
                }
                else
                {
                    // Not compressed, read NR_OF_SH_COEFFS coefficients for each element.
                    for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                    {
                        float ReadCoeff; InFile.read((char*)&ReadCoeff, sizeof(ReadCoeff));

                        FN->SHLMapMan.SHLMaps[SMI.SHLMapNr]->Coeffs[((SMI.PosT+t)*cf::SceneGraph::SHLMapManT::SIZE_S+SMI.PosS+s)*NR_OF_SH_COEFFS+CoeffNr]=ReadCoeff;
                    }
                }
    }

    // FacesDrawIndices
    for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
        FN->DrawIndices.PushBack(aux::ReadUInt32(InFile));

 // FN->Init();
    return FN;
}


FaceNodeT::~FaceNodeT()
{
    if (RenderMat!=NULL)
    {
        // Note that also the MatSys::Renderer pointer can be NULL, but then the RenderMat must be NULL, too.
        assert(MatSys::Renderer!=NULL);

        MatSys::Renderer->FreeMaterial(RenderMat);
        RenderMat=NULL;
    }
}


void FaceNodeT::InitRenderMeshesAndMats(const ArrayT<Vector3dT>& SharedVertices, const float LightMapPatchSize)
{
    // **********************
    // Create the basic mesh.
    // **********************

    Mesh.Type=MatSys::MeshT::TriangleFan;
    Mesh.Vertices.PushBackEmpty(DrawIndices.Size());

    for (unsigned long VertexNr=0; VertexNr<DrawIndices.Size(); VertexNr++)
    {
        Mesh.Vertices[VertexNr].Origin[0]=SharedVertices[DrawIndices[VertexNr]].x;
        Mesh.Vertices[VertexNr].Origin[1]=SharedVertices[DrawIndices[VertexNr]].y;
        Mesh.Vertices[VertexNr].Origin[2]=SharedVertices[DrawIndices[VertexNr]].z;
        Mesh.Vertices[VertexNr].Origin[3]=1.0;

        Mesh.Vertices[VertexNr].Color[0]=1.0;
        Mesh.Vertices[VertexNr].Color[1]=1.0;
        Mesh.Vertices[VertexNr].Color[2]=1.0;
        Mesh.Vertices[VertexNr].Color[3]=1.0;
    }


    // ****************************************************************
    // Calculate and fill-in the 'TextureCoord' components of the Mesh.
    // ****************************************************************

    // Calculate and fill-in the 'TextureST' coordinate components of the 'FaceVertexInfos'.
    // Note that this is really mis-placed here, a relict of the old Q1 "naturally aligned textures" paradigm.
    // I think that the assignment of s/t-coordinates really belongs into the world editors, in the hands of the artists.
    // However, the current editors do not support this.
    // The next best solution would probably be to precalculate this in CaBSP, but then CaBSP would need access to all textures files.
    const Vector3dT        TI_U   =TI.U.AsVectorOfDouble();
    const Vector3dT        TI_V   =TI.V.AsVectorOfDouble();
    const double           LengthU=length(TI_U);            // Länge des U-Vektors (X-Richtung).
    const double           LengthV=length(TI_V);            // Länge des V-Vektors (Y-Richtung).

    double SmallestS=dot(SharedVertices[DrawIndices[0]], TI_U)/(LengthU*LengthU)+TI.OffsetU;
    double SmallestT=dot(SharedVertices[DrawIndices[0]], TI_V)/(LengthV*LengthV)+TI.OffsetV;

    for (unsigned long VertexNr=1; VertexNr<DrawIndices.Size(); VertexNr++)
    {
        const double s=dot(SharedVertices[DrawIndices[VertexNr]], TI_U)/(LengthU*LengthU)+TI.OffsetU;
        const double t=dot(SharedVertices[DrawIndices[VertexNr]], TI_V)/(LengthV*LengthV)+TI.OffsetV;

        if (s<SmallestS) SmallestS=s;
        if (t<SmallestT) SmallestT=t;
    }

    SmallestS=floor(SmallestS);
    SmallestT=floor(SmallestT);

    // Bestimmen der Texturkoodinaten s und t für alle Vertices (Punkte) P eines Polygons:
    // Seien die Vektoren U und V gegeben, die die TextureSpace-Ebene aufspannen. P ist idR Element dieser Ebene und
    // ergibt sich aus einer zu bestimmenden Linearkombination P = s * U + t * V. Mit dot(U, P)/|U| erhalten
    // wir die Projektion von P auf den Normalenvektor von U. Weil wir s (== Vielfaches von U, U skaliert die Texel
    // und kann wesentlich größer als 1.0 sein) erhalten wollen, wird nochmals durch |U| dividiert:
    // dot(U, P)/(|U|^2). Zu diesen Vielfachen von U wird der zugehörige Offset (Verschiebung entlang U) addiert.
    // Analog für t.
    for (unsigned long VertexNr=0; VertexNr<DrawIndices.Size(); VertexNr++)
    {
        const double s=dot(SharedVertices[DrawIndices[VertexNr]], TI_U)/(LengthU*LengthU)+TI.OffsetU-SmallestS;
        const double t=dot(SharedVertices[DrawIndices[VertexNr]], TI_V)/(LengthV*LengthV)+TI.OffsetV-SmallestT;

        Mesh.Vertices[VertexNr].TextureCoord[0]=float(s);
        Mesh.Vertices[VertexNr].TextureCoord[1]=float(t);
    }


    // *****************************************************************************************************
    // Calculate and fill-in the 'LightMapST' and 'SHLMapST' coordinate components of the 'FaceVertexInfos'.
    // *****************************************************************************************************

    // (Dieser Teil ist ähnlich, aber nicht isomorph zu Phase 3, Berechnung der TextureST-Koordinaten.)
    // Eigentlich wäre es aber besser, diese Dinge schon in CaBSP vorzuberechnen.
    VectorT U;
    VectorT V;

    Polygon.Plane.GetSpanVectors(U, V);

    double SmallestU=dot(SharedVertices[DrawIndices[0]], U);
    double SmallestV=dot(SharedVertices[DrawIndices[0]], V);

    for (unsigned long VertexNr=1; VertexNr<DrawIndices.Size(); VertexNr++)
    {
        const double u=dot(SharedVertices[DrawIndices[VertexNr]], U);
        const double v=dot(SharedVertices[DrawIndices[VertexNr]], V);

        if (u<SmallestU) SmallestU=u;
        if (v<SmallestV) SmallestV=v;
    }

    // Bestimmen der LightMap-Koodinaten s und t für alle Vertices (Punkte) P eines Polygons:
    // (Vergleiche auch mit der Dokumentation zu Phase 3 oben!)
    // Seien die Einheitsvektoren U und V gegeben, die den UV-Space der Ebene aufspannen. P ist idR Element dieser
    // Ebene und ergibt sich aus einer zu bestimmenden Linearkombination P = u*U + v*V.
    // (Stimmt nicht ganz, setzt nämlich voraus, daß der UV-Space nicht durch den Ursprung geht, sondern in die
    //  tatsächliche Ebene eingebettet ist. Richtig: P = u*U + v*V + Stützvektor. Dies spielt aber keine Rolle!)
    // Mit dot(U, P) erhalten wir die Projektion u von P auf den (Normalen-)Vektor U.
    // Davon subtrahieren wir SmallestU und erhalten u', was letztendlich eine Verschiebung der ganzen Face an die
    // V-Achse bewirkt, d.h. am Schleifenende wird mind. eines der P eine u'-Koordinate von 0 haben.
    // Die Division überführt u' in den ST-Space der LightMap. Schließlich korrigieren wir die Koordinate noch,
    // um dem Rand der LightMap sowie ihrer Position innerhalb der "großen" LightMap Rechnung zu tragen und erhalten s.
    // Ganz zum Schluß sorgt die letzte Division für die richtige Skalierung für OpenGL. Analog für t.
    for (unsigned long VertexNr=0; VertexNr<DrawIndices.Size(); VertexNr++)
    {
        const double PosS=LightMapInfo.PosS;
        const double PosT=LightMapInfo.PosT;

        const double s=((dot(SharedVertices[DrawIndices[VertexNr]], U)-SmallestU)/LightMapPatchSize+1.0+PosS)/double(cf::SceneGraph::LightMapManT::SIZE_S);
        const double t=((dot(SharedVertices[DrawIndices[VertexNr]], V)-SmallestV)/LightMapPatchSize+1.0+PosT)/double(cf::SceneGraph::LightMapManT::SIZE_T);

        Mesh.Vertices[VertexNr].LightMapCoord[0]=float(s);
        Mesh.Vertices[VertexNr].LightMapCoord[1]=float(t);
    }

#if SHL_ENABLED
    // Bestimmen der SHLMap-Koodinaten s und t für alle Vertices (Punkte) P eines Polygons.
    // This works analogously to the code for LightMaps, except that we use floor(SmallestU/V) instead of simply SmallestU/V.
    // Note that wrt. overlapping patches, this is the proper solution (and should be adapted for LightMaps, too).
    for (unsigned long VertexNr=0; VertexNr<DrawIndices.Size(); VertexNr++)
    {
        const double PosS=Map.Faces[FaceNr].SHLMapInfo.PosS;
        const double PosT=Map.Faces[FaceNr].SHLMapInfo.PosT;

        const double s=(dot(SharedVertices[DrawIndices[VertexNr]], U)/SHLMapPatchSize-floor(SmallestU/SHLMapPatchSize)+1.0+PosS)/double(cf::SceneGraph::SHLMapManT::SIZE_S);
        const double t=(dot(SharedVertices[DrawIndices[VertexNr]], V)/SHLMapPatchSize-floor(SmallestV/SHLMapPatchSize)+1.0+PosT)/double(cf::SceneGraph::SHLMapManT::SIZE_T);

        Mesh.Vertices[VertexNr].SHLMapCoord[0]=float(s);
        Mesh.Vertices[VertexNr].SHLMapCoord[1]=float(t);
    }
#endif


    // Fill-in the 'FaceInfos'.
    BB=BoundingBox3T<double>(Polygon.Vertices);


    // *******************************
    // Compute the texture-space axes.
    // *******************************

    const VectorT& TexNormal=Polygon.Plane.Normal;
    const VectorT  TexU     =TI.U.AsVectorOfDouble();
    const VectorT  TexV     =TI.V.AsVectorOfDouble();

    // Note that
    // a) our intention here is to compute the proper coordinate axes of the texture-space,
    // b) the above used 'U' and 'V' span vectors are absolutely useless for this purpose (they are entirely arbitrary and unrelated), and
    // c) the 'TexU' and 'TexV' vectors (together with 'TexNormal') are not correct either.
    // Item c) is true because 'TexU' and 'TexV' span a texture-space plane through the origin,
    // which is NOT NECESSARILY PARALLEL to 'Map.Faces[FaceNr].Plane' (catchphrase "Naturally Aligned Textures").
    // In other words: 'TexNormal' is fine, but 'TexU' and/or 'TexV' are possibly not orthogonal to it.
    // Thus, we have to find new 'TexU' and 'TexV' vectors, that are
    // a) orthogonal to 'TexNormal' (but not necessarily to each other) (and thus span a plane that is parallel to 'Map.Faces[FaceNr].Plane'),
    // b) and map points in the plane to the same values than the original vectors did.
    // (These are essentially what the level editor should have provided in the first place.)
    // To do so, first find the normal vector 'UxV' of the plane spanned by 'TexU' and 'TexV',
    // then project 'TexU' and 'TexV' along 'UxV' onto the plane 'Map.Faces[FaceNr].Plane'.
    // (For 'TexU', this is done by solving the equation 'Map.Faces[FaceNr].Plane.GetDistance(TexU + r*UxV)==0' for 'r',
    //  yielding 'r=-dot(TexNormal, TexU)/dot(TexNormal, UxV)'. Analogous for 'TexV'.)
    const VectorT UxV =cross(TexU, TexV);
    const VectorT NewU=TexU+scale(UxV, -dot(TexNormal, TexU)/dot(TexNormal, UxV));
    const VectorT NewV=TexV+scale(UxV, -dot(TexNormal, TexV)/dot(TexNormal, UxV));

    // Here, for our purposes, we only need the unit vectors, not the real lengths.
    // TODO: Well, could calculate all this much earlier, e.g. in CaBSP or even in WcMap2Ca!
    // This MIGHT also render 'Plane3T<T>::GetSpanVectors()' obsolete, but this in turn would require
    // that 'NewU' and 'NewV' HAVE to be restricted and verified to be orthogonal (for general purpose use elsewhere).
    // On the other hand, this collides with the idea to compute s/t coordinates in CaBSP directly...
    const double LengthNewU=length(NewU);
    const double LengthNewV=length(NewV);

    // TODO: Emit warning message to console if NewU or NewV cannot be normalized!
    const Vector3dT FaceTangent =LengthNewU>0.000000001 ? scale(NewU, 1.0/LengthNewU) : VectorT();
    const Vector3dT FaceBiNormal=LengthNewV>0.000000001 ? scale(NewV, 1.0/LengthNewV) : VectorT();

    // Fill-in the 'NormalXYZ', 'TangentXYZ', and 'BiNormalXYZ' components of the 'FaceVertexInfos', taking SmoothGroups into account.
    for (unsigned long VertexNr=0; VertexNr<DrawIndices.Size(); VertexNr++)
    {
        Mesh.Vertices[VertexNr].SetNormal  (TexNormal   .AsVectorOfFloat());
        Mesh.Vertices[VertexNr].SetTangent (FaceTangent .AsVectorOfFloat());
        Mesh.Vertices[VertexNr].SetBiNormal(FaceBiNormal.AsVectorOfFloat());
    }


    // ***********************
    // Allocate the RenderMat.
    // ***********************

    RenderMat=MatSys::Renderer->RegisterMaterial(Material);
}


bool FaceNodeT::GetLightmapColorNearPosition(const Vector3dT& Ground, Vector3fT& LightMapColor, const float LightMapPatchSize) const
{
    if (!Material->UsesGeneratedLightMap()) return false;
    if (LightMapInfo.SizeS==0) return false;
    if (LightMapInfo.SizeT==0) return false;
    if (LightMapInfo.LightMapNr>=LightMapMan.Bitmaps.Size()) return false;

 // if (Polygon.Plane.Normal.z<0.0) return false;   // This is too restrictive for some uses.
    if (Polygon.Plane.GetDistance(Ground)>0.5) return false;

    for (unsigned long VertexNr=0; VertexNr<Polygon.Vertices.Size(); VertexNr++)
        if (Polygon.GetEdgePlane(VertexNr, 0.0).GetDistance(Ground)<-0.1)
            return false;

    // 'Ground' is "inside" 'Face'.
    VectorT SpanU;
    VectorT SpanV;

    Polygon.Plane.GetSpanVectors(SpanU, SpanV);

    double SmallestU=dot(Polygon.Vertices[0], SpanU);
    double SmallestV=dot(Polygon.Vertices[0], SpanV);

    for (unsigned long VertexNr=1; VertexNr<Polygon.Vertices.Size(); VertexNr++)
    {
        const double u=dot(Polygon.Vertices[VertexNr], SpanU);
        const double v=dot(Polygon.Vertices[VertexNr], SpanV);

        if (u<SmallestU) SmallestU=u;
        if (v<SmallestV) SmallestV=v;
    }

    const double       s =(dot(Ground, SpanU)-SmallestU)/LightMapPatchSize+1.0+LightMapInfo.PosS;
    const double       t =(dot(Ground, SpanV)-SmallestV)/LightMapPatchSize+1.0+LightMapInfo.PosT;
    const unsigned int s_=(unsigned int)(s+0.5);
    const unsigned int t_=(unsigned int)(t+0.5);
    const BitmapT*     Bm=LightMapMan.Bitmaps[LightMapInfo.LightMapNr];

    if (s_>Bm->SizeX) return false;
    if (t_>Bm->SizeY) return false;

    Bm->GetPixel(s_, t_, LightMapColor.x, LightMapColor.y, LightMapColor.z);

#if 0   // This code should actually be removed entirely - gamma for lightmaps is now properly applied in CaLight, and thus there is no need for toying around with it elsewhere.
    // Correct the gamma value.
    // IMPORTANT: This must be done here explicitly, because the LightMapMan does *NOT* modify its lightmap bitmaps with any gamma value
    // (only the textures that are uploaded to the gfx board are modified). This Gamma stuff should probably be entirely be moved into CaLight.
    // ALSO NOTE that we just duplicate the constant from ClientWorld.cpp here... which is *BAD*!
    const float Gamma=1.6f;

    LightMapColor.x=pow(LightMapColor.x, 1.0f/Gamma);
    LightMapColor.y=pow(LightMapColor.y, 1.0f/Gamma);
    LightMapColor.z=pow(LightMapColor.z, 1.0f/Gamma);
#endif
    return true;
}


void FaceNodeT::WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const
{
    aux::Write(OutFile, "Face");

    Pool.Write(OutFile, Polygon.Plane.Normal);
    OutFile.write((char*)&Polygon.Plane.Dist, sizeof(Polygon.Plane.Dist));

    aux::Write(OutFile, aux::cnc_ui32(Polygon.Vertices.Size()));
    for (unsigned long VertexNr=0; VertexNr<Polygon.Vertices.Size(); VertexNr++)
        Pool.Write(OutFile, Polygon.Vertices[VertexNr]);

    Pool.Write(OutFile, Material->Name);

    // Texture Information
    Pool.Write(OutFile, TI.U);
    Pool.Write(OutFile, TI.V);
    OutFile.write((char*)&TI.OffsetU, sizeof(TI.OffsetU));
    OutFile.write((char*)&TI.OffsetV, sizeof(TI.OffsetV));

    // LightMaps
    {
        const LightMapInfoT& LMI=LightMapInfo;

        OutFile.write((char*)&LMI.SizeS, sizeof(LMI.SizeS));
        OutFile.write((char*)&LMI.SizeT, sizeof(LMI.SizeT));

        for (unsigned long t=0; t<LMI.SizeT; t++)
            for (unsigned long s=0; s<LMI.SizeS; s++)
            {
                int  Red, Green, Blue;
                char v;

                LightMapMan.Bitmaps[LMI.LightMapNr]->GetPixel(LMI.PosS+s, LMI.PosT+t, Red, Green, Blue);

                v=Red;   OutFile.write(&v, sizeof(v));
                v=Green; OutFile.write(&v, sizeof(v));
                v=Blue;  OutFile.write(&v, sizeof(v));
            }

        for (unsigned long t=0; t<LMI.SizeT; t++)
            for (unsigned long s=0; s<LMI.SizeS; s++)
            {
                int  Red, Green, Blue, Alpha;
                char v;

                LightMapMan.Bitmaps2[LMI.LightMapNr]->GetPixel(LMI.PosS+s, LMI.PosT+t, Red, Green, Blue, Alpha);

                v=Red;   OutFile.write(&v, sizeof(v));
                v=Green; OutFile.write(&v, sizeof(v));
                v=Blue;  OutFile.write(&v, sizeof(v));
                v=Alpha; OutFile.write(&v, sizeof(v));
            }
    }

    // SHLMaps
    {
        const SHLMapInfoT&  SMI            =SHLMapInfo;
        const unsigned long NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;

        OutFile.write((char*)&SMI.SizeS, sizeof(SMI.SizeS));
        OutFile.write((char*)&SMI.SizeT, sizeof(SMI.SizeT));

        for (unsigned long t=0; t<SMI.SizeT; t++)
            for (unsigned long s=0; s<SMI.SizeS; s++)
                if (cf::SceneGraph::SHLMapManT::NrOfRepres>0)
                {
                    // Compressed, only write the indices.
                    OutFile.write((char*)&SHLMapMan.SHLMaps[SMI.SHLMapNr]->Indices[(SMI.PosT+t)*cf::SceneGraph::SHLMapManT::SIZE_S+SMI.PosS+s], sizeof(unsigned short));
                }
                else
                {
                    // Not compressed, write NR_OF_SH_COEFFS coefficients for each element.
                    for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                        OutFile.write((char*)&SHLMapMan.SHLMaps[SMI.SHLMapNr]->Coeffs[((SMI.PosT+t)*cf::SceneGraph::SHLMapManT::SIZE_S+SMI.PosS+s)*NR_OF_SH_COEFFS+CoeffNr], sizeof(float));
                }
    }

    // DrawIndices
    aux::Write(OutFile, aux::cnc_ui32(DrawIndices.Size()));
    for (unsigned long IndexNr=0; IndexNr<DrawIndices.Size(); IndexNr++)
        aux::Write(OutFile, aux::cnc_ui32(DrawIndices[IndexNr]));
}


const BoundingBox3T<double>& FaceNodeT::GetBoundingBox() const
{
    return BB;
}


bool FaceNodeT::IsOpaque() const
{
    return Material->HasDefaultBlendFunc();
}


void FaceNodeT::DrawAmbientContrib(const Vector3dT& /*ViewerPos*/) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    MatSys::Renderer->SetCurrentMaterial(RenderMat);

    if (LightMapInfo.LightMapNr<LightMapMan.Textures.Size())
    {
        MatSys::Renderer->SetCurrentLightMap   (LightMapMan.Textures [LightMapInfo.LightMapNr]);
        MatSys::Renderer->SetCurrentLightDirMap(LightMapMan.Textures2[LightMapInfo.LightMapNr]);
    }

    MatSys::Renderer->RenderMesh(Mesh);
}


void FaceNodeT::DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::STENCILSHADOW);

    // Render the silhouette quads.
    static MatSys::MeshT SilhouetteMesh(MatSys::MeshT::QuadStrip);
    SilhouetteMesh.Vertices.Overwrite();
    SilhouetteMesh.Vertices.PushBackEmpty(2*Mesh.Vertices.Size()+2);

    const VectorT  A0=VectorT(Mesh.Vertices[0].Origin[0], Mesh.Vertices[0].Origin[1], Mesh.Vertices[0].Origin[2]);
    const VectorT LA0=A0-LightPos;

    // Note that the order is reversed because we're dealing with a back-facing polygon as occluder!
    SilhouetteMesh.Vertices[0].SetOrigin(LA0.x, LA0.y, LA0.z, 0.0);
    SilhouetteMesh.Vertices[1].SetOrigin( A0.x,  A0.y,  A0.z, 1.0);

    for (unsigned long VertexNr=1; VertexNr<Mesh.Vertices.Size(); VertexNr++)
    {
        const VectorT  A=VectorT(Mesh.Vertices[VertexNr].Origin[0], Mesh.Vertices[VertexNr].Origin[1], Mesh.Vertices[VertexNr].Origin[2]);
        const VectorT LA=A-LightPos;

        // Note that the order is reversed because we're dealing with a back-facing polygon as occluder!
        SilhouetteMesh.Vertices[VertexNr*2+0].SetOrigin(LA.x, LA.y, LA.z, 0.0);
        SilhouetteMesh.Vertices[VertexNr*2+1].SetOrigin( A.x,  A.y,  A.z, 1.0);
    }

    SilhouetteMesh.Vertices[Mesh.Vertices.Size()*2+0].SetOrigin(LA0.x, LA0.y, LA0.z, 0.0);  // Close the silhouette for this occluder.
    SilhouetteMesh.Vertices[Mesh.Vertices.Size()*2+1].SetOrigin( A0.x,  A0.y,  A0.z, 1.0);

    MatSys::Renderer->RenderMesh(SilhouetteMesh);


    // Render the occluders near cap (front-facing wrt. the light source).
    // As we are considering a *back-facing* polygon as a occluder (oriented CCW when looked at from behind),
    // we have to reverse the order of its vertices in order to turn it into a CW ordered, front-facing polygon.
    static MatSys::MeshT CapMesh(MatSys::MeshT::TriangleFan);   // The default winding order is "CW".

    CapMesh.Vertices.Overwrite();
    CapMesh.Vertices.PushBackEmpty(Mesh.Vertices.Size());

    for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        CapMesh.Vertices[CapMesh.Vertices.Size()-VertexNr-1].SetOrigin(Mesh.Vertices[VertexNr].Origin[0],
                                                                       Mesh.Vertices[VertexNr].Origin[1],
                                                                       Mesh.Vertices[VertexNr].Origin[2], 1.0);

    // What a pity the order isn't the other way round. Could then simply write:  MatSys::Renderer->RenderMesh(Mesh);
    MatSys::Renderer->RenderMesh(CapMesh);


    // Render the occluders far cap (back-facing wrt. the light source).
    // As we are already dealing with a back-facing polygon, the vertex order is already as required,
    // we just have to project them to infinity as seen from the light source.
    for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        CapMesh.Vertices[VertexNr].SetOrigin(Mesh.Vertices[VertexNr].Origin[0]-LightPos.x,
                                             Mesh.Vertices[VertexNr].Origin[1]-LightPos.y,
                                             Mesh.Vertices[VertexNr].Origin[2]-LightPos.z, 0.0);

    MatSys::Renderer->RenderMesh(CapMesh);
}


void FaceNodeT::DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING);

    MatSys::Renderer->SetCurrentMaterial(RenderMat);
    MatSys::Renderer->RenderMesh(Mesh);
}


void FaceNodeT::DrawTranslucentContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    DrawAmbientContrib(ViewerPos);
}


void FaceNodeT::InitDefaultLightMaps(const float LightMapPatchSize)
{
    LightMapInfo.SizeS=0;
    LightMapInfo.SizeT=0;

    // Einige Faces wollen keine generierte Lightmap.
    if (!Material->UsesGeneratedLightMap()) return;

    // Bestimme die Größe der LightMap
    VectorT U;
    VectorT V;

    Polygon.Plane.GetSpanVectors(U, V);

    // Bestimme MinU, MinV, MaxU, MaxV
    if (Polygon.Vertices.Size()==0) return;     // Bei Fehler ordne keine Lightmap zu.

    double MinU=dot(Polygon.Vertices[0], U); double MaxU=MinU;
    double MinV=dot(Polygon.Vertices[0], V); double MaxV=MinV;

    for (unsigned long VertexNr=1; VertexNr<Polygon.Vertices.Size(); VertexNr++)
    {
        double u=dot(Polygon.Vertices[VertexNr], U);
        double v=dot(Polygon.Vertices[VertexNr], V);

        if (u<MinU) MinU=u;
        if (v<MinV) MinV=v;

        if (u>MaxU) MaxU=u;
        if (v>MaxV) MaxV=v;
    }

    // Dies ist das einzige Mal, daß die Größen der LightMaps rechnerisch bestimmt werden.
    // Fortan werden sie von allen Tools und der Engine direkt aus dem CW-File geladen,
    // um Rundungsfehler, die beim Speichern durch Zusammenfassung entstehen, zu vermeiden.
    LightMapInfo.SizeS=(unsigned short)ceil((MaxU-MinU)/LightMapPatchSize)+2;
    LightMapInfo.SizeT=(unsigned short)ceil((MaxV-MinV)/LightMapPatchSize)+2;

    // Maximum permitted size exceeded?
    if (LightMapInfo.SizeS>cf::SceneGraph::LightMapManT::SIZE_S) { printf("LMI.SizeS exceeds maximum!\n"); LightMapInfo.SizeS=0; LightMapInfo.SizeT=0; return; }
    if (LightMapInfo.SizeT>cf::SceneGraph::LightMapManT::SIZE_T) { printf("LMI.SizeT exceeds maximum!\n"); LightMapInfo.SizeS=0; LightMapInfo.SizeT=0; return; }

    // Setze die ganze LightMap auf neutrales weiß.
    unsigned long LmNr;
    unsigned int  LmPosS;
    unsigned int  LmPosT;

    if (!LightMapMan.Allocate(LightMapInfo.SizeS, LightMapInfo.SizeT, LmNr, LmPosS, LmPosT)) { printf("LightMapMan.Allocate() failed!\n"); LightMapInfo.SizeS=0; LightMapInfo.SizeT=0; return; }

    LightMapInfo.LightMapNr=(unsigned short)LmNr;
    LightMapInfo.PosS      =(unsigned short)LmPosS;
    LightMapInfo.PosT      =(unsigned short)LmPosT;

    for (unsigned int t=0; t<LightMapInfo.SizeT; t++)
        for (unsigned int s=0; s<LightMapInfo.SizeS; s++)
            LightMapMan.Bitmaps[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t, 0xFF, 0xFF, 0xFF);

    for (unsigned int t=0; t<LightMapInfo.SizeT; t++)
        for (unsigned int s=0; s<LightMapInfo.SizeS; s++)
            LightMapMan.Bitmaps2[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t, 0x80, 0x80, 0xFF, 0xFF);
}


void FaceNodeT::CreatePatchMeshes(ArrayT<cf::PatchMeshT>& PatchMeshes, ArrayT< ArrayT< ArrayT<Vector3dT> > >& SampleCoords, const float LightMapPatchSize) const
{
    if (LightMapInfo.SizeS==0) return;
    if (LightMapInfo.SizeT==0) return;
    if (Polygon.Vertices.Size()<3) return;

    // A FaceNodeT creates excatly one patch mesh - namely the one that covers it.
    PatchMeshes.PushBackEmpty();
    SampleCoords.PushBackEmpty();

    PatchMeshT& PatchMesh=PatchMeshes[PatchMeshes.Size()-1];


    // Fill-in basic data.
    PatchMesh.Width =LightMapInfo.SizeS;
    PatchMesh.Height=LightMapInfo.SizeT;
    PatchMesh.Patches.PushBackEmpty(PatchMesh.Width*PatchMesh.Height);

    PatchMesh.WrapsHorz=false;
    PatchMesh.WrapsVert=false;

    PatchMesh.Node    =this;
    PatchMesh.Material=Material;

    SampleCoords[0].PushBackEmpty(PatchMesh.Width*PatchMesh.Height);


    // Compute the details for each patch.
    // Dieser Code ist sehr ähnlich zu dem Code in CaLights PostProcessBorders() Funktion!
    // Änderungen hier könnten Änderungen in dieser Funktion erforderlich machen!

    // Bestimme die Spannvektoren.
    VectorT U;
    VectorT V;

    Polygon.Plane.GetSpanVectors(U, V);

    // Finde SmallestU und SmallestV.
    double SmallestU=dot(Polygon.Vertices[0], U);
    double SmallestV=dot(Polygon.Vertices[0], V);

    for (unsigned long VertexNr=1; VertexNr<Polygon.Vertices.Size(); VertexNr++)
    {
        double u=dot(Polygon.Vertices[VertexNr], U);
        double v=dot(Polygon.Vertices[VertexNr], V);

        if (u<SmallestU) SmallestU=u;
        if (v<SmallestV) SmallestV=v;
    }

    // Bereite folgende Schleife vor.
    const VectorT UV_Origin=scale(Polygon.Plane.Normal, Polygon.Plane.Dist);
    const VectorT Safety   =scale(Polygon.Plane.Normal, 0.1);

    Polygon3T<double> PatchPoly;
    PatchPoly.Plane=dot(Polygon.Plane.Normal, cross(U, V))<0 ? Polygon.Plane : Polygon.Plane.GetMirror();

    // Nun betrachte alle Patches.
    for (unsigned long t=0; t<PatchMesh.Height; t++)
        for (unsigned long s=0; s<PatchMesh.Width; s++)
        {
            const double PATCH_SIZE=LightMapPatchSize;
            cf::PatchT&  Patch     =PatchMesh.Patches[t*PatchMesh.Width+s];

            Patch.Coord     =VectorT(0, 0, 0);
            Patch.Normal    =Polygon.Plane.Normal;
            Patch.Area      =PATCH_SIZE*PATCH_SIZE;
            Patch.InsideFace=false;

            // Also assign an initial, non-zero "Where comes the energy from?"-direction.
            // The value (==length) has been chosen entirely arbitrary with the accumulative nature of the computations in CaLight
            // in mind, and in the hope to pick a reasonable value.
            // (However, tests seem to indicate the smaller values are better. I tried 0.3 first, then 0.05.)
            Patch.EnergyFromDir=Patch.Normal*0.02;

            const double s_=s;
            const double t_=t;

            PatchPoly.Vertices.Clear();
            PatchPoly.Vertices.PushBack(UV_Origin+scale(U, SmallestU+(s_-1.0)*PATCH_SIZE)+scale(V, SmallestV+(t_-1.0)*PATCH_SIZE));
            PatchPoly.Vertices.PushBack(UV_Origin+scale(U, SmallestU+ s_     *PATCH_SIZE)+scale(V, SmallestV+(t_-1.0)*PATCH_SIZE));
            PatchPoly.Vertices.PushBack(UV_Origin+scale(U, SmallestU+ s_     *PATCH_SIZE)+scale(V, SmallestV+ t_     *PATCH_SIZE));
            PatchPoly.Vertices.PushBack(UV_Origin+scale(U, SmallestU+(s_-1.0)*PATCH_SIZE)+scale(V, SmallestV+ t_     *PATCH_SIZE));

            assert(fabs(PatchPoly.GetArea()-Patch.Area)<1.0);   // This is just to make sure there is no logic error in area computation.

            if (!Polygon.Overlaps(PatchPoly, false, ROUND_EPSILON)) continue;

            if (!Polygon.Encloses(PatchPoly, true, ROUND_EPSILON))
            {
                ArrayT< Polygon3T<double> > NewPolygons;

                PatchPoly.GetChoppedUpAlong(Polygon, ROUND_EPSILON, NewPolygons);

                if (NewPolygons.Size()>0 && NewPolygons[NewPolygons.Size()-1].Vertices.Size()>0)
                {
                    PatchPoly=NewPolygons[NewPolygons.Size()-1];
                }
                else
                {
                    // It's not utterly right, but to keep things simple in case of problems with chopping,
                    // we just treat this patch as if it was fully outside of our polygon.
                    printf("WARNING: PolygonChopUp failed or PatchPoly.Vertices.Size()==0.\n");
                    continue;
                }
            }

            // If a patch was at least fractionally inside its face, we (re-)determine its center point now.
            for (unsigned long VertexNr=0; VertexNr<PatchPoly.Vertices.Size(); VertexNr++)
                Patch.Coord=Patch.Coord+PatchPoly.Vertices[VertexNr];

            // The "Safety" is to avoid (or at least reduce) accidental self-intersections due to rounding errors
            // during clipping computations in CaLight.
            Patch.Coord     =scale(Patch.Coord, 1.0/double(PatchPoly.Vertices.Size()))+Safety;
         // Patch.Normal    =Polygon.Plane.Normal;      // Already done above.
            Patch.Area      =PatchPoly.GetArea();
            Patch.InsideFace=true;


            // Compute several sample coordinates for each patch.
            // The caller may use them for example for computing/sampling the initial incident of sunlight.
            // Note that we add two kinds of safety margin: We move the samples a bit "above" their plane along the
            // normal vector ("Safety") as well as a bit towards the center of the patch, away from their border.
            ArrayT<Vector3dT>& ppsc=SampleCoords[0][t*PatchMesh.Width+s];

            for (unsigned long VertexNr=0; VertexNr<PatchPoly.Vertices.Size(); VertexNr++)
            {
                const Vector3dT& V1=PatchPoly.Vertices[VertexNr];
                const Vector3dT& V2=PatchPoly.Vertices[(VertexNr+1) % PatchPoly.Vertices.Size()];

                const Vector3dT SafetyTowardsCenter1=normalize(Patch.Coord-(V1+Safety), 0.0)*0.1;
                const Vector3dT SafetyTowardsCenter2=normalize(Patch.Coord-(V2+Safety), 0.0)*0.1;

                ppsc.PushBack(V1+SafetyTowardsCenter1+Safety);
                ppsc.PushBack(scale(V1+SafetyTowardsCenter1 + V2+SafetyTowardsCenter2, 0.5)+Safety);
                ppsc.PushBack(scale(V1+Safety + Patch.Coord, 0.5));
            }

            ppsc.PushBack(Patch.Coord);
        }
}


void FaceNodeT::BackToLightMap(const cf::PatchMeshT& PatchMesh, const float LightMapPatchSize)
{
    assert(PatchMesh.Width ==LightMapInfo.SizeS);
    assert(PatchMesh.Height==LightMapInfo.SizeT);
    assert(PatchMesh.Patches.Size()==PatchMesh.Width*PatchMesh.Height);

    assert(PatchMesh.WrapsHorz==false);
    assert(PatchMesh.WrapsVert==false);

    assert(PatchMesh.Node    ==this);
    assert(PatchMesh.Material==Material);


    // This duplicates the code of InitRenderMeshesAndMats() above!
    const Vector3fT FaceNormal  =Polygon.Plane.Normal.AsVectorOfFloat();
    const Vector3fT UxV         =cross(TI.U, TI.V);
    const Vector3fT FaceTangentU=normalizeOr0(TI.U+scale(UxV, -dot(FaceNormal, TI.U)/dot(FaceNormal, UxV)));
    const Vector3fT FaceTangentV=normalizeOr0(TI.V+scale(UxV, -dot(FaceNormal, TI.V)/dot(FaceNormal, UxV)));


    // Übertrage die Patches-Werte zurück in die LightMaps.
    for (unsigned long t=0; t<PatchMesh.Height; t++)
        for (unsigned long s=0; s<PatchMesh.Width; s++)
        {
            const PatchT&  Patch=PatchMesh.GetPatch(s, t);
            const VectorT& RGB  =Patch.TotalEnergy;

            LightMapMan.Bitmaps[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t, char(RGB.x+0.49), char(RGB.y+0.49), char(RGB.z+0.49));


            VectorT Dir=normalizeOr0(Patch.EnergyFromDir);

            // Contrary to Bezier Patches, where we need code as shown in the snipped below,
            // there is no need to check Dir to not exceed a maximum angle with regard to face patches,
            // because faces are always planar and thus cannot receive energy from "behind" in the first place.
         // if (dot(Dir, Patch.Normal)<CosMaxAngle)
         // {
         //     // ...
         // }

            // Compute the Implicit Orientation Factor.
            // *** Note that this factor is already (implicitly) contained in the RGB value ***
            // *** above, because CaLight naturally computes the radiosity results so!!!    ***
            // The value is directly scaled from 0..1 to 0..255 range, and converted to int.
            int iof=int(dot(Dir, Patch.Normal)*255.0+0.49);

            if (iof<  1) iof=  1;   // Avoid divisions-by-zero in the MatSys's pixel shaders.
            if (iof>255) iof=255;

            // Transform (rotate) Dir from world-space into tangent-space.
            MatrixT RotMat;

            for (unsigned long i=0; i<3; i++)
            {
                RotMat.m[0][i]=FaceTangentU[i];
                RotMat.m[1][i]=FaceTangentV[i];
                RotMat.m[2][i]=FaceNormal  [i];
            }

            Dir=RotMat.Mul0(Dir);
         // if (Dir.z<0) Dir.z=0;   // Contrary to Bezier Patches, were this can happen and even is in order (and thus must *not* be clamped to 0!), it never occurs with faces, as it is impossible for faces to be reached by energy from behind.
            Dir=normalizeOr0(Dir);

            // Color-encode Dir and scale from 0..1 to 0..255 range.
            Dir=(Dir+VectorT(1, 1, 1))*0.5*255.0;

            LightMapMan.Bitmaps2[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t,
                char(Dir.x+0.49), char(Dir.y+0.49), char(Dir.z+0.49), iof);
        }
}
