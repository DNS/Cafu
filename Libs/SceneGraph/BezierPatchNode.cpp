/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "BezierPatchNode.hpp"
#include "FaceNode.hpp"     // For FaceNodeT::ROUND_EPSILON.
#include "_aux.hpp"
#include "LightMapMan.hpp"
#include "Bitmap/Bitmap.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/BezierPatch.hpp"
#include "Math3D/Matrix.hpp"
#include "Util/Util.hpp"

#include <cassert>


using namespace cf::SceneGraph;


BezierPatchNodeT::BezierPatchNodeT(LightMapManT& LMM, float MaxError)
    : SizeX(0),
      SizeY(0),
      ControlPointsXYZ(),
      ControlPointsUV(),
      SubdivsHorz(0),
      SubdivsVert(0),
      m_MaxError(MaxError),
      Material(NULL),
      LightMapInfo(),
      LightMapMan(LMM),
      RenderMaterial(NULL)
{
    Init();
}


BezierPatchNodeT::BezierPatchNodeT(LightMapManT& LMM, unsigned long SizeX_, unsigned long SizeY_, const ArrayT<float>& ControlPoints_, int SubdivsHorz_, int SubdivsVert_, MaterialT* Material_, float MaxError)
    : SizeX(SizeX_),
      SizeY(SizeY_),
      ControlPointsXYZ(),
      ControlPointsUV(),
      SubdivsHorz(SubdivsHorz_),
      SubdivsVert(SubdivsVert_),
      m_MaxError(MaxError),
      Material(Material_),
      LightMapInfo(),
      LightMapMan(LMM),
      RenderMaterial(NULL)
{
    for (unsigned long ComponentNr=0; ComponentNr<ControlPoints_.Size(); ComponentNr+=5)
    {
        ControlPointsXYZ.PushBack(Vector3fT(ControlPoints_[ComponentNr+0], ControlPoints_[ComponentNr+1], ControlPoints_[ComponentNr+2]));
        ControlPointsUV .PushBack(Vector3fT(ControlPoints_[ComponentNr+3], ControlPoints_[ComponentNr+4], 0.0f));
    }

    Init();
}


BezierPatchNodeT::BezierPatchNodeT(LightMapManT& LMM, unsigned long SizeX_, unsigned long SizeY_, const ArrayT<Vector3fT>& ControlPointsXYZ_, const ArrayT<Vector3fT>& ControlPointsUV_, int SubdivsHorz_, int SubdivsVert_, MaterialT* Material_, float MaxError)
    : SizeX(SizeX_),
      SizeY(SizeY_),
      ControlPointsXYZ(ControlPointsXYZ_),
      ControlPointsUV(ControlPointsUV_),
      SubdivsHorz(SubdivsHorz_),
      SubdivsVert(SubdivsVert_),
      m_MaxError(MaxError),
      Material(Material_),
      LightMapInfo(),
      LightMapMan(LMM),
      RenderMaterial(NULL)
{
    Init();
}


BezierPatchNodeT* BezierPatchNodeT::CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, LightMapManT& LMM, SHLMapManT& /*SMM*/)
{
    const float MaxError = aux::ReadFloat(InFile);

    BezierPatchNodeT* BP=new BezierPatchNodeT(LMM, MaxError);

    BP->SizeX=aux::ReadUInt32(InFile);
    BP->SizeY=aux::ReadUInt32(InFile);

    BP->SubdivsHorz=aux::ReadInt32(InFile);
    BP->SubdivsVert=aux::ReadInt32(InFile);

    const std::string MaterialName=Pool.ReadString(InFile);
    BP->Material=MaterialManager->GetMaterial(MaterialName);
 // BP->Material=MaterialManager->GetMaterial("wireframeTest");
    if (BP->Material==NULL)
    {
        printf("Material %s not found!\n", MaterialName.c_str());
        delete BP;
        return NULL;
    }

    for (unsigned long Coord=0; Coord<BP->SizeX*BP->SizeY; Coord++)
    {
        BP->ControlPointsXYZ.PushBack(aux::ReadVector3f(InFile));
        BP->ControlPointsUV .PushBack(aux::ReadVector3f(InFile));
    }

    // LightMaps
    {
        LightMapInfoT& LMI=BP->LightMapInfo;

        InFile.read((char*)&LMI.SizeS, sizeof(LMI.SizeS));
        InFile.read((char*)&LMI.SizeT, sizeof(LMI.SizeT));

        unsigned long LmNr;
        unsigned int  LmPosS;
        unsigned int  LmPosT;

        if (!BP->LightMapMan.Allocate(LMI.SizeS, LMI.SizeT, LmNr, LmPosS, LmPosT))
        {
            printf("Could not allocate LightMap.\n");
            delete BP;
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

                BP->LightMapMan.Bitmaps[LMI.LightMapNr]->SetPixel(LMI.PosS+s, LMI.PosT+t, Red, Green, Blue);
            }

        for (unsigned int t=0; t<LMI.SizeT; t++)
            for (unsigned int s=0; s<LMI.SizeS; s++)
            {
                char Red, Green, Blue, Alpha;

                InFile.read(&Red  , sizeof(Red  ));
                InFile.read(&Green, sizeof(Green));
                InFile.read(&Blue , sizeof(Blue ));
                InFile.read(&Alpha, sizeof(Alpha));

                BP->LightMapMan.Bitmaps2[LMI.LightMapNr]->SetPixel(LMI.PosS+s, LMI.PosT+t, Red, Green, Blue, Alpha);
            }
    }

    BP->Init();
    return BP;
}


BezierPatchNodeT::~BezierPatchNodeT()
{
    Clean();
}


void BezierPatchNodeT::WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const
{
    aux::Write(OutFile, "BP");

    aux::Write(OutFile, m_MaxError);
    aux::Write(OutFile, aux::cnc_ui32(SizeX));
    aux::Write(OutFile, aux::cnc_ui32(SizeY));

    aux::Write(OutFile, (int32_t)SubdivsHorz);
    aux::Write(OutFile, (int32_t)SubdivsVert);

    Pool.Write(OutFile, Material->Name);
    for (unsigned long c=0; c<ControlPointsXYZ.Size(); c++)
    {
        aux::Write(OutFile, ControlPointsXYZ[c]);
        aux::Write(OutFile, ControlPointsUV [c]);
    }

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
}


const BoundingBox3T<double>& BezierPatchNodeT::GetBoundingBox() const
{
    return BB;
}


bool BezierPatchNodeT::IsOpaque() const
{
    return Material->HasDefaultBlendFunc();
}


void BezierPatchNodeT::DrawAmbientContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    MatSys::Renderer->SetCurrentMaterial(RenderMaterial);

    if (LightMapInfo.LightMapNr<LightMapMan.Textures.Size())
    {
        MatSys::Renderer->SetCurrentLightMap   (LightMapMan.Textures [LightMapInfo.LightMapNr]);
        MatSys::Renderer->SetCurrentLightDirMap(LightMapMan.Textures2[LightMapInfo.LightMapNr]);
    }

    // TODO:
    // cLOD Überlegungen sind denkbar - siehe "r_lodCurveError" convar im Q3 Quellcode.
    // Siehe http://xreal.sourceforge.net/xrealwiki/XMapExplanations
    // Aber: Sind wg. Tangent-Space viel aufwendiger als bei Q3... Performance?
    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
        MatSys::Renderer->RenderMesh(*Meshes[MeshNr]);

#if 0
    // Render the tangent-space axes (for debugging).
    const float   AxesLength=200.0f;
    MatSys::MeshT Axes(MatSys::MeshT::Lines);

    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
        for (unsigned long VertexNr=0; VertexNr<Meshes[MeshNr]->Vertices.Size(); VertexNr+=(MeshNr+1<Meshes.Size()) ? 2 : 1)
        {
            // Note that we only consider every second vertex in the (triangle-strip) mesh.
            const MatSys::MeshT::VertexT& PatchVertex=Meshes[MeshNr]->Vertices[VertexNr];
            const Vector3fT               Orig       =Vector3fT(PatchVertex.Origin);

            Axes.Vertices.PushBackEmpty(6);
            const unsigned long vSize=Axes.Vertices.Size();

            Axes.Vertices[vSize-6].SetOrigin(Orig);
            Axes.Vertices[vSize-6].SetColor(1.0f, 0.0f, 0.0f);
            Axes.Vertices[vSize-5].SetOrigin(Orig+Vector3fT(PatchVertex.Normal)*AxesLength);
            Axes.Vertices[vSize-5].SetColor(1.0f, 0.0f, 0.0f);
            Axes.Vertices[vSize-4].SetOrigin(Orig);
            Axes.Vertices[vSize-4].SetColor(0.0f, 1.0f, 0.0f);
            Axes.Vertices[vSize-3].SetOrigin(Orig+Vector3fT(PatchVertex.Tangent)*AxesLength);
            Axes.Vertices[vSize-3].SetColor(0.0f, 1.0f, 0.0f);
            Axes.Vertices[vSize-2].SetOrigin(Orig);
            Axes.Vertices[vSize-2].SetColor(0.0f, 0.0f, 1.0f);
            Axes.Vertices[vSize-1].SetOrigin(Orig+Vector3fT(PatchVertex.BiNormal)*AxesLength);
            Axes.Vertices[vSize-1].SetColor(0.0f, 0.0f, 1.0f);
        }

    MatSys::Renderer->RenderMesh(Axes);
#endif
}


void BezierPatchNodeT::DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::STENCILSHADOW);
}


void BezierPatchNodeT::DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING);

    MatSys::Renderer->SetCurrentMaterial(RenderMaterial);

    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
        MatSys::Renderer->RenderMesh(*Meshes[MeshNr]);
}


void BezierPatchNodeT::DrawTranslucentContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    DrawAmbientContrib(ViewerPos);
}


void BezierPatchNodeT::UpdateMeshColor(const float red, const float green, const float blue, const float alpha)
{
    for (unsigned int i=0; i<Meshes.Size(); i++)
    {
        MatSys::MeshT* currentMesh=Meshes[i];
        for(unsigned int j=0; j<currentMesh->Vertices.Size(); j++)
        {
            currentMesh->Vertices[j].Color[0]=red;
            currentMesh->Vertices[j].Color[1]=green;
            currentMesh->Vertices[j].Color[2]=blue;
            currentMesh->Vertices[j].Color[3]=alpha;
        }
    }
}


void BezierPatchNodeT::InitDefaultLightMaps(const float LightMapPatchSize)
{
    LightMapInfo.SizeS=0;
    LightMapInfo.SizeT=0;

    // Einige BPs wollen keine generierte Lightmap.
    if (!Material->UsesGeneratedLightMap()) return;

    cf::math::BezierPatchT<float> LightMapMesh;
    GenerateLightMapMesh(LightMapMesh, LightMapPatchSize, false);

    // Note that contrary to faces, we need no "border" of 1 pixel width around our lightmap due to texture filtering,
    // because the philosophy for bezier patches is a bit different than that for faces:
    // For faces, the square lightmap elements (lightmap patches) are aligned so that their top-left borders align with
    // the top-left corner of a face. For Bezier patches however, the vertices of the patch mesh correspond to the
    // *center* of their related light-map element. This makes a "border" unnecessary.
    // You may want to refer to the lightmap-coordinates computation of the draw mesh for more details.
    unsigned long LmNr;
    unsigned int  LmPosS;
    unsigned int  LmPosT;

    if (!LightMapMan.Allocate(LightMapMesh.Width, LightMapMesh.Height, LmNr, LmPosS, LmPosT))
    {
        printf("LightMapMan.Allocate() failed!\n");
        return;
    }

    LightMapInfo.SizeS     =(unsigned short)LightMapMesh.Width;
    LightMapInfo.SizeT     =(unsigned short)LightMapMesh.Height;
    LightMapInfo.LightMapNr=(unsigned short)LmNr;
    LightMapInfo.PosS      =(unsigned short)LmPosS;
    LightMapInfo.PosT      =(unsigned short)LmPosT;

    // Setze die ganze LightMap auf neutrales weiß.
    for (unsigned int t=0; t<LightMapInfo.SizeT; t++)
        for (unsigned int s=0; s<LightMapInfo.SizeS; s++)
        {
#ifdef DEBUG
            // Color the "border" green and the inside red.
            const unsigned char r=(s==0 || s+1==LightMapInfo.SizeS || t==0 || t+1==LightMapInfo.SizeT) ? 0 : 0xFF;
            const unsigned char g=0xFF-r;
            const unsigned char b=0;
#else
            const unsigned char r=0xFF;
            const unsigned char g=0xFF;
            const unsigned char b=0xFF;
#endif
            LightMapMan.Bitmaps[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t, r, g, b);
        }

    for (unsigned int t=0; t<LightMapInfo.SizeT; t++)
        for (unsigned int s=0; s<LightMapInfo.SizeS; s++)
            LightMapMan.Bitmaps2[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t, 0x80, 0x80, 0xFF, 0xFF);
}


void BezierPatchNodeT::CreatePatchMeshes(ArrayT<cf::PatchMeshT>& PatchMeshes, ArrayT< ArrayT< ArrayT<Vector3dT> > >& SampleCoords, const float LightMapPatchSize) const
{
    if (LightMapInfo.SizeS==0) return;
    if (LightMapInfo.SizeT==0) return;

    cf::math::BezierPatchT<float> LightMapMesh;
    GenerateLightMapMesh(LightMapMesh, LightMapPatchSize, true);

    if (LightMapMesh.Width !=LightMapInfo.SizeS) { printf("WARNING: LightMapMesh.Width!=LightMapInfo.SizeS\n");  return; }
    if (LightMapMesh.Height!=LightMapInfo.SizeT) { printf("WARNING: LightMapMesh.Height!=LightMapInfo.SizeT\n"); return; }


    // A BezierPatchNodeT creates excatly one patch mesh - namely the one that covers it.
    // TODO... this also has implications on the computations with OutwardRay below...!
    //   (In fact it's even worse: Two-sided implies we need two separate lightmaps...!)
    if (Material->TwoSided) printf("WARNING: Two-sided material %s on bezier patch only gets one-sided patch mesh!\n", Material->Name.c_str());

    PatchMeshes.PushBackEmpty();
    SampleCoords.PushBackEmpty();

    PatchMeshT& PatchMesh=PatchMeshes[PatchMeshes.Size()-1];


    // Fill-in basic data.
    PatchMesh.WrapsHorz=LightMapMesh.WrapsHorz(); assert(!PatchMesh.WrapsHorz || LightMapInfo.SizeS>1);
    PatchMesh.WrapsVert=LightMapMesh.WrapsVert(); assert(!PatchMesh.WrapsVert || LightMapInfo.SizeT>1);

    PatchMesh.Width =PatchMesh.WrapsHorz ? LightMapInfo.SizeS-1 : LightMapInfo.SizeS;
    PatchMesh.Height=PatchMesh.WrapsVert ? LightMapInfo.SizeT-1 : LightMapInfo.SizeT;
    PatchMesh.Patches.PushBackEmpty(PatchMesh.Width*PatchMesh.Height);

    PatchMesh.Node    =this;
    PatchMesh.Material=Material;

    SampleCoords[0].PushBackEmpty(PatchMesh.Width*PatchMesh.Height);


    // Prepare a collision model for use below.
    cf::ClipSys::CollisionModelStaticT* CollModel=NULL;

    // Material can be NULL when BezierPatchNodeTs are created with "dummy materials" in CaWE.
    // FIXME: Is that still true? I think we can now (since revision 749) only ever get here
    //        from CaLight code, where all Material pointer should/must be valid.
    if (Material && (Material->ClipFlags & MaterialT::Clip_Radiance)>0)
    {
        // ###
        // ### We need to create CollisionBezierPatch instead of reusing CurveMesh here because
        // ### the mesh must be exactly identical to that in the collision world!
        // ### (Right now the situation is very awkward: We have a CollModel here against which TraceRay()
        // ###  calls are shot in CreatePatchMeshes(), but the actual tracing of rays in CaLight is done
        // ###  in the fully separate collision world!)
        // ###
        ArrayT<Vector3dT> CtrlPoints;

        for (unsigned long PointNr=0; PointNr<ControlPointsXYZ.Size(); PointNr++)
            CtrlPoints.PushBack(ControlPointsXYZ[PointNr].AsVectorOfDouble());

        cf::math::BezierPatchT<double> CollisionBezierPatch(SizeX, SizeY, CtrlPoints);

        if (SubdivsHorz>0 && SubdivsVert>0)
        {
            // The mapper may have provided an explicit number of subdivisions in order to avoid gaps between adjacent bezier patches.
            // The casts to unsigned long are needed in order to resolve ambiguity of the overloaded Subdivide() method.
            CollisionBezierPatch.Subdivide((unsigned long)SubdivsHorz, (unsigned long)SubdivsVert);
        }
        else
        {
            // ###
            // ### For now, these consts MUST be kept in sync with those in ClipSys/CollisionModel_static.cpp / LoadWorld.cpp !!!
            // ###
            const double COLLISION_MODEL_MAX_CURVE_ERROR  = 24.0;
            const double COLLISION_MODEL_MAX_CURVE_LENGTH = -1.0;

            CollisionBezierPatch.Subdivide(COLLISION_MODEL_MAX_CURVE_ERROR, COLLISION_MODEL_MAX_CURVE_LENGTH);
        }

        // Get a collision model from the curve mesh.
        ArrayT<Vector3dT> CoordsOnly;

        for (unsigned long VertexNr=0; VertexNr<CollisionBezierPatch.Mesh.Size(); VertexNr++)
            CoordsOnly.PushBack(CollisionBezierPatch.Mesh[VertexNr].Coord);

        const double COLLISION_MODEL_MIN_NODE_SIZE = 40.0;

        CollModel = new cf::ClipSys::CollisionModelStaticT(CollisionBezierPatch.Width, CollisionBezierPatch.Height, CoordsOnly, Material, COLLISION_MODEL_MIN_NODE_SIZE);
    }


    // Nun betrachte alle Patches.
    const double BBDiagonalLength=length(GetBoundingBox().Max-GetBoundingBox().Min);

    // Find the center of this mesh.
    Vector3dT MeshCenter=LightMapMesh.Mesh[0].Coord.AsVectorOfDouble();

    for (unsigned long VertexNr=1; VertexNr<LightMapMesh.Mesh.Size(); VertexNr++)
        MeshCenter+=LightMapMesh.Mesh[VertexNr].Coord.AsVectorOfDouble();

    MeshCenter*=(1.0/LightMapMesh.Mesh.Size());


    for (unsigned long t=0; t<PatchMesh.Height; t++)
        for (unsigned long s=0; s<PatchMesh.Width; s++)
        {
            const cf::math::BezierPatchT<float>::VertexT& Vertex=LightMapMesh.GetVertex(s, t);
            cf::PatchT&                                   Patch =PatchMesh.Patches[t*PatchMesh.Width+s];

            // Determine the coordinate and the normal vector for the patch.
            Patch.Coord =Vertex.Coord.AsVectorOfDouble();
            Patch.Normal=Vertex.Normal.AsVectorOfDouble();

            // Move the Patch.Coord a bit towards the center of the mesh.
            // This a) moves the point out of walls that the mesh might be adjacent to,
            // and  b) makes sure that the TraceRay() call below doesn't "miss" the patch near its border by a tiny amount.
            // (Oh, I just realize that b) is wrong: a miss can only happen if the sample ray could "overshoot", which it cannot,
            //  it never goes farther inside the patch than Patch.Coord.)
            //
            // Note that for the benefit of non-border patches, the code has been restricted to border patches only.
            // This is okay, except for certain degenerate patches that have "inner" (control-)points that are on the border, too.
            // However, I think that the BezierPatchT::Subdivide() methods remove such redundant vertices from the mesh.
            // Even if they didn't, the CaLight border postprocessing code probably fixes any problems that might result from this.
            // In the very rare (impossible?) case that that didn't help either, the user should change the definition of the patch in the map.
            if ((!PatchMesh.WrapsHorz && (s==0 || s==PatchMesh.Width-1)) || (!PatchMesh.WrapsVert && (t==0 || t==PatchMesh.Height-1)))
            {
                const Vector3dT DirToCenter=MeshCenter-Patch.Coord;
                const Vector3dT ProjDir    =DirToCenter-Patch.Normal*dot(Patch.Normal, DirToCenter);

                // ProjDir is the projection of DirToCenter into the plane defined by Patch.Normal.
                // Use it to move the Patch.Coord by FaceNodeT::ROUND_EPSILON to the center.
                // Note that doing so might move the point "into" (or "under") the patch,
                // which however is addressed/fixed by the code with the OutwardRay below.
                Patch.Coord+=normalizeOr0(ProjDir)*FaceNodeT::ROUND_EPSILON;
            }

            // Patch.Coord must be on the *clip* hull of this bezier patch - from which the mesh that we generated above may differ!
            // Therefore, move Patch.Coord out along its normal, then try to move back as much as possible, until the clip hull is hit.
            //
            // The problem with a too long OutwardRay is that is does not work with patches that are "more than 180 degrees concave",
            // because then another portion of the bezier patch might get into the way and be hit first, ruining the computation.
            // There are three alternatives to address the problem:
            // a) Just ignore that the case of "more than 180 degrees concave" patches can occur, and/or let the user break them down into
            //    smaller peaces if this actually ever occurs. Besides that, a very simple and robust solution.
            // b) Use a reasonable, small constant instead of BBDiagonalLength. Requires knowledge about the maximum deviation from the
            //    clip hull to be safe. Note that in fact, we happen to have that knowledge: At the time of this writing, the clip hull
            //    and the LightMapMesh happen to be geometrically identical!
            // c) First trace the ray outwards, and then from the possible hit point at another point of the patch back to the start.
            // Although I think that c) is best, I currently have a) implememented, which works very well, too, and is tested.
            // BEWARE: It is assumed that the patches have one-sided materials, and that rays from back through to front are *not* clipped!
            //         If two-sided materials are used however, all this needs thorough revision...! For example, with one-sided materials
            //         method a) works even with tubes that we see from the inside, but when two-sided materials are used, it stops to work!
            if (CollModel)
            {
                const VectorT OutwardRay  =Patch.Normal*BBDiagonalLength;
                const VectorT OutwardPoint=Patch.Coord+OutwardRay;

                const static cf::ClipSys::TracePointT Point;
                cf::ClipSys::TraceResultT Result(1.0);

                CollModel->TraceConvexSolid(Point, OutwardPoint, -OutwardRay, MaterialT::Clip_Radiance, Result);

                // The Patch.Normal*FaceNodeT::ROUND_EPSILON is "safety" in order to avoid (or at least reduce) accidental
                // self-intersections due to rounding errors during clipping computations in CaLight.
                //
                // The *10.0 has been added in order to increase the "safety" margin even more,
                // in order to obtain a "perfect" result throughout TestPatches.cmap.
                // TODO: I still don't quite understand why such a relatively large safety is necessary at all, everywhere
                //       else the factor is much less than the FaceNodeT::ROUND_EPSILON*10.0 required here - investigate!
                //       This *might* be related to float vs. double in the current implementation (revision 747)...
                //       Update: doubles are used everywhere now (since r749), but I still leave the *10.0 in, because it
                //       provides *slightly* better results.
                Patch.Coord=OutwardPoint-OutwardRay*Result.Fraction + Patch.Normal*(FaceNodeT::ROUND_EPSILON*10.0);
            }

            // Determine the area for the Patch.
            Patch.Area      =LightMapMesh.GetSurfaceAreaAtVertex(s, t);
            Patch.InsideFace=(Patch.Area>1.0);

            // Also assign an initial, non-zero "Where comes the energy from?"-direction.
            // The value (==length) has been chosen entirely arbitrary with the accumulative nature of the computations in CaLight
            // in mind, and in the hope to pick a reasonable value.
            // (However, tests seem to indicate the smaller values are better. I tried 0.3 first, then 0.05.)
            Patch.EnergyFromDir=Patch.Normal*0.02;

            // Return one sample coordinate for each patch.
            // The caller may use it for example for computing/sampling the initial incident of sunlight.
            SampleCoords[0][t*PatchMesh.Width+s].PushBack(Patch.Coord);
        }

    delete CollModel;
}


void BezierPatchNodeT::BackToLightMap(const cf::PatchMeshT& PatchMesh, const float LightMapPatchSize)
{
    assert(PatchMesh.Width ==(PatchMesh.WrapsHorz ? LightMapInfo.SizeS-1ul : LightMapInfo.SizeS));
    assert(PatchMesh.Height==(PatchMesh.WrapsVert ? LightMapInfo.SizeT-1ul : LightMapInfo.SizeT));
    assert(PatchMesh.Patches.Size()==PatchMesh.Width*PatchMesh.Height);

    assert(PatchMesh.Node    ==this);
    assert(PatchMesh.Material==Material);


    cf::math::BezierPatchT<float> LightMapMesh;
    GenerateLightMapMesh(LightMapMesh, LightMapPatchSize, true);

    assert(LightMapMesh.Width ==LightMapInfo.SizeS);
    assert(LightMapMesh.Height==LightMapInfo.SizeT);


    // Übertrage die Patches-Werte zurück in die LightMaps.
    for (unsigned long t=0; t<LightMapInfo.SizeT; t++)
        for (unsigned long s=0; s<LightMapInfo.SizeS; s++)
        {
            const unsigned long s_=s % PatchMesh.Width;     // Meshes that wrap horizontally and/or vertically get their last lightmap column and/or row repeated.
            const unsigned long t_=t % PatchMesh.Height;

            const cf::math::BezierPatchT<float>::VertexT& Vertex=LightMapMesh.GetVertex(s_, t_);
            const cf::PatchT&                             Patch =PatchMesh.GetPatch(s_, t_);
            const VectorT&                                RGB   =Patch.TotalEnergy;

            LightMapMan.Bitmaps[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t, char(RGB.x+0.49), char(RGB.y+0.49), char(RGB.z+0.49));


            VectorT      Dir        =normalizeOr0(Patch.EnergyFromDir);
            const double MaxAngle   =80.0/180.0*3.141592654;
            const double CosMaxAngle=cos(MaxAngle);

            if (dot(Dir, Patch.Normal)<CosMaxAngle)
            {
                // The angle between Dir and Patch.Normal is greater than MaxAngle.
                const double CosIsAngle=dot(Dir, Patch.Normal);
                const double IsAngle   =acos(CosIsAngle);

                // Debug output.
             // printf("IS  angle: DEG %f, RAD %f, cos %f\n", IsAngle *180.0/3.141592654, IsAngle,  CosIsAngle );
             // printf("MAX angle: DEG %f, RAD %f, cos %f\n", MaxAngle*180.0/3.141592654, MaxAngle, CosMaxAngle);

                // The factor l is easily computed by employing the Sinussatz, using the insight that
                // one angle in the triangle is IsAngle-MaxAngle and another is MaxAngle, and that
                // the length of side Dir is 1.
                const double l=sin(IsAngle-MaxAngle)/sin(MaxAngle);

                // Finally fix Dir to not exceed the MaxAngle any longer.
                Dir=normalizeOr0(Dir+Patch.Normal*l);

                // Assert (even in release builds) that the new Dir is correct.
                if (fabs(dot(Dir, Patch.Normal)-CosMaxAngle)>0.0001) printf("WARNING: Corrected Dir still incorrect!\n");
            }

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
                RotMat.m[0][i]=Vertex.TangentS[i];
                RotMat.m[1][i]=Vertex.TangentT[i];
                RotMat.m[2][i]=Vertex.Normal  [i];
            }

            Dir=RotMat.Mul0(Dir);
         // if (Dir.z<0) Dir.z=0;    // Do *NOT* uncomment this line - negative z values are in order here! (Although they should never occur due to the above MaxAngle correction code anyway.)
            Dir=normalizeOr0(Dir);

            // Color-encode Dir and scale from 0..1 to 0..255 range.
            Dir=(Dir+VectorT(1, 1, 1))*0.5*255.0;

            LightMapMan.Bitmaps2[LightMapInfo.LightMapNr]->SetPixel(LightMapInfo.PosS+s, LightMapInfo.PosT+t,
                char(Dir.x+0.49), char(Dir.y+0.49), char(Dir.z+0.49), iof);
        }
}


static void AssignMatSysFromBezierPatchVertex(MatSys::MeshT::VertexT& msV, const cf::math::BezierPatchT<float>::VertexT& bpV)
{
    msV.SetOrigin       (bpV.Coord);
    msV.SetColor        (0.0, 1.0, 1.0);    // For debugging materials that use "useMeshColors".
    msV.SetTextureCoord (bpV.TexCoord.x, bpV.TexCoord.y);
 // msV.SetLightMapCoord(0, 0);
 // msV.SetSHLMapCoord  (0, 0);
    msV.SetNormal       (bpV.Normal);
    msV.SetTangent      (bpV.TangentS);
    msV.SetBiNormal     (bpV.TangentT);
}


void BezierPatchNodeT::Init()
{
    Clean();

    if (SizeX==0) return;
    if (SizeY==0) return;


    // Create a BB over all control points.
    // This works because the bezier patch is always contained in the convex hull of the control points.
    for (unsigned long y=0; y<SizeY; y++)
        for (unsigned long x=0; x<SizeX; x++)
        {
            const Vector3dT CP=ControlPointsXYZ[y*SizeX+x].AsVectorOfDouble();

            if (x==0 && y==0) BB=BoundingBox3T<double>(CP);
                         else BB.Insert(CP);
        }


#if 1
#if 1
    cf::math::BezierPatchT<float> CurveMesh(SizeX, SizeY, ControlPointsXYZ, ControlPointsUV);

    CurveMesh.ComputeTangentSpace();

    // The casts to unsigned long are needed in order to resolve ambiguity of the overloaded Subdivide() method.
    if (SubdivsHorz>0 && SubdivsVert>0) CurveMesh.Subdivide((unsigned long)SubdivsHorz, (unsigned long)SubdivsVert);
                                   else CurveMesh.Subdivide(m_MaxError, -1.0f);
#else
    // Use this for debugging the lightmap mesh code...
    cf::math::BezierPatchT<float> CurveMesh;
    GenerateLightMapMesh(CurveMesh, true);
#endif

    // Copy the curve mesh into MatSys meshes.
    for (unsigned long ColumnNr=0; ColumnNr+1<CurveMesh.Width; ColumnNr++)
    {
        // There is one triangle strip for each column of the curve mesh.
        Meshes.PushBack(new MatSys::MeshT(MatSys::MeshT::TriangleStrip));
        Meshes[ColumnNr]->Vertices.PushBackEmpty(2*CurveMesh.Height);

        for (unsigned long RowNr=0; RowNr<CurveMesh.Height; RowNr++)
        {
            MatSys::MeshT::VertexT& Left =Meshes[ColumnNr]->Vertices[RowNr*2+0];

            AssignMatSysFromBezierPatchVertex(Left, CurveMesh.GetVertex(ColumnNr, RowNr));

            const float LeftLmS=(float(LightMapInfo.PosS)+0.5f + (float(ColumnNr)/(CurveMesh.Width -1))*(LightMapInfo.SizeS-1)) / cf::SceneGraph::LightMapManT::SIZE_S;
            const float LeftLmT=(float(LightMapInfo.PosT)+0.5f + (float(RowNr   )/(CurveMesh.Height-1))*(LightMapInfo.SizeT-1)) / cf::SceneGraph::LightMapManT::SIZE_T;

            Left.SetLightMapCoord(LeftLmS, LeftLmT);
            Left.SetSHLMapCoord  (LeftLmS, LeftLmT);


            MatSys::MeshT::VertexT& Right=Meshes[ColumnNr]->Vertices[RowNr*2+1];

            AssignMatSysFromBezierPatchVertex(Right, CurveMesh.GetVertex(ColumnNr+1, RowNr));

            const float RightLmS=(float(LightMapInfo.PosS)+0.5f + (float(ColumnNr+1)/(CurveMesh.Width -1))*(LightMapInfo.SizeS-1)) / cf::SceneGraph::LightMapManT::SIZE_S;
            const float RightLmT=LeftLmT;

            Right.SetLightMapCoord(RightLmS, RightLmT);
            Right.SetSHLMapCoord  (RightLmS, RightLmT);
        }
    }
#else
    /*** OLD METHOD ***/

    // Normally, we preferred the subdivision approach over stepping from t==0 to t==1 in small intervalls, because
    // a) the triangles are easier to compute (simple subdivision),
    // b) the LoD considerations are much simpler,
    // c) adjacent patches join better,
    // d) texture coords are easier determined, and
    // e) normal vectors are easier determined.
    // BUT this holds more for the "dynamic" case, not for pre-computing the triangles.
    // For LoD, we can also simply skip triangles later, and subdivision does not permit arbitrary many triangles.
    // Thus, we stay with the "straight-forward" approach.
    const unsigned long TessSizeX_3x3=(SubdivsHorz>0) ? SubdivsHorz : GetAutoSubdivsHorz();
    const unsigned long TessSizeY_3x3=(SubdivsVert>0) ? SubdivsVert : GetAutoSubdivsVert();

    const unsigned long TotalNrOfTriangleColumns=TessSizeX_3x3*(SizeX-1)/2;  // Number of triangle columns for the entire patch.
    const unsigned long TotalNrOfTriangleRows   =TessSizeY_3x3*(SizeY-1)/2;  // Number of triangle rows    for the entire patch.


    for (unsigned long ColumnNr=0; ColumnNr<TotalNrOfTriangleColumns; ColumnNr++)
    {
        // There is one triangle strip for each column of the patch.
        Meshes.PushBack(new MatSys::MeshT(MatSys::MeshT::TriangleStrip));
        Meshes[ColumnNr]->Vertices.PushBackEmpty((TotalNrOfTriangleRows+1)*2);
    }

    // Loop over all 3x3 sub-patches.
    for (unsigned long SubPatchY=0; SubPatchY+2<SizeY; SubPatchY+=2)
    {
        for (unsigned long SubPatchX=0; SubPatchX+2<SizeX; SubPatchX+=2)
        {
            // Copy the control points of the 3x3 sub-patch into arrays that are easier to access.
            Vector3fT CP3x3_xyz[3][3];
            Vector3fT CP3x3_uv [3][3];

            for (unsigned long y=0; y<3; y++)
                for (unsigned long x=0; x<3; x++)
                {
                    CP3x3_xyz[x][y]=ControlPointsXYZ[(SubPatchY+y)*SizeX+(SubPatchX+x)];
                    CP3x3_uv [x][y]=ControlPointsUV [(SubPatchY+y)*SizeX+(SubPatchX+x)];
                }

            for (unsigned long y=0; y<=TessSizeY_3x3; y++)
                for (unsigned long x=0; x<=TessSizeX_3x3; x++)
                {
                    const float s=float(x)/float(TessSizeX_3x3);
                    const float t=float(y)/float(TessSizeY_3x3);

                    const float B_s [3]={ (1.0f-s)*(1.0f-s), 2.0f*s*(1.0f-s), s*s };
                    const float B_t [3]={ (1.0f-t)*(1.0f-t), 2.0f*t*(1.0f-t), t*t };

                    const float B_s_[3]={ 2.0f*(s-1.0f), 2.0f-4.0f*s, 2.0f*s };     // Derivation along s.
                    const float B_t_[3]={ 2.0f*(t-1.0f), 2.0f-4.0f*t, 2.0f*t };     // Derivation along t.

                    // If we stored all the patch points not in triangle-strips, but rather in a simple rectangular
                    // array of vertices "Verts", the array had size (TotalNrOfTriangleColumns+1)*(TotalNrOfTriangleRows+1)
                    // and the current point was at Verts[AbsIndexX][AbsIndexY].
                    // The /2 is in order to correct the count from the index number.
                    const unsigned long AbsIndexX=SubPatchX/2*TessSizeX_3x3+x;
                    const unsigned long AbsIndexY=SubPatchY/2*TessSizeY_3x3+y;


                    Vector3fT Origin;
                    Vector3fT TexCoord;
                    Vector3fT Normal;
                    Vector3fT Tangent;
                    Vector3fT BiNormal;

                    Vector3fT TangentS;     // Tangents of the spatial patch surface.
                    Vector3fT TangentT;

                    Vector3fT TexTangentS;  // Tangents of the texture image in 2D texture space.
                    Vector3fT TexTangentT;

                    for (unsigned long i=0; i<3; i++)
                        for (unsigned long j=0; j<3; j++)
                        {
                            Origin     =Origin     +scale(CP3x3_xyz[i][j], B_s [i]*B_t [j]);
                            TexCoord   =TexCoord   +scale(CP3x3_uv [i][j], B_s [i]*B_t [j]);

                            TangentS   =TangentS   +scale(CP3x3_xyz[i][j], B_s_[i]*B_t [j]);
                            TangentT   =TangentT   +scale(CP3x3_xyz[i][j], B_s [i]*B_t_[j]);

                            TexTangentS=TexTangentS+scale(CP3x3_uv [i][j], B_s_[i]*B_t [j]);
                            TexTangentT=TexTangentT+scale(CP3x3_uv [i][j], B_s [i]*B_t_[j]);
                        }

                    try
                    {
                        // This is documented in my Tech Archive, see "Computing the tangent space basis vectors".
                        // Note that the *sign* of d is really important (less its magnitude).
                        const float d=TexTangentS.x*TexTangentT.y-TexTangentS.y*TexTangentT.x;

                        if (fabs(d)<0.00000001f) throw DivisionByZeroE();

                        Normal  =normalize(cross(TangentS, TangentT), 0.0f);
                        Tangent =normalize(scale(scale(TangentT, -TexTangentS.y) + scale(TangentS, TexTangentT.y), 1.0f/d), 0.0f);
                        BiNormal=normalize(scale(scale(TangentT,  TexTangentS.x) - scale(TangentS, TexTangentT.x), 1.0f/d), 0.0f);
                    }
                    catch (const DivisionByZeroE& /*E*/)
                    {
                        // Don't announce this - it is perfectly legal and occurs whenever several control points fall in the same place.
                        // EnqueueString("Warning: Bad tangent space for Bezier patch %lu.", BPNr);
                    }


                    // The point at coordinate (AbsIndexX, AbsIndexY) is relevant for the triangle-strips of both columns
                    // AbsIndexX-1 and AbsIndexX -- except if it is in the leftmost or rightmost column of vertices.
                    if (AbsIndexX>0)
                    {
                        MatSys::MeshT::VertexT& V=Meshes[AbsIndexX-1]->Vertices[AbsIndexY*2+1];

                        V.SetOrigin       (Origin.x, Origin.y, Origin.z);
                        V.SetTextureCoord (TexCoord.x, TexCoord.y);
                        V.SetLightMapCoord(float(AbsIndexX)/float(TotalNrOfTriangleColumns), float(AbsIndexY)/float(TotalNrOfTriangleRows)); // (s, t) is for the 3x3 sub-patch only, this is across the entire patch.
                        V.SetSHLMapCoord  (float(AbsIndexX)/float(TotalNrOfTriangleColumns), float(AbsIndexY)/float(TotalNrOfTriangleRows)); // (s, t) is for the 3x3 sub-patch only, this is across the entire patch.
                        V.SetNormal       (-Normal);
                        V.SetTangent      ( Tangent);
                        V.SetBiNormal     ( BiNormal);
                    }

                    if (AbsIndexX<TotalNrOfTriangleColumns)
                    {
                        MatSys::MeshT::VertexT& V=Meshes[AbsIndexX]->Vertices[AbsIndexY*2];

                        V.SetOrigin       (Origin.x, Origin.y, Origin.z);
                        V.SetTextureCoord (TexCoord.x, TexCoord.y);
                        V.SetLightMapCoord(float(AbsIndexX)/float(TotalNrOfTriangleColumns), float(AbsIndexY)/float(TotalNrOfTriangleRows)); // (s, t) is for the 3x3 sub-patch only, this is across the entire patch.
                        V.SetSHLMapCoord  (float(AbsIndexX)/float(TotalNrOfTriangleColumns), float(AbsIndexY)/float(TotalNrOfTriangleRows)); // (s, t) is for the 3x3 sub-patch only, this is across the entire patch.
                        V.SetNormal       (-Normal);
                        V.SetTangent      ( Tangent);
                        V.SetBiNormal     ( BiNormal);
                    }
                }
        }
    }
#endif


    if (MatSys::Renderer)
    {
        RenderMaterial=MatSys::Renderer->RegisterMaterial(Material);
    }
}


void BezierPatchNodeT::Clean()
{
    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++) delete Meshes[MeshNr];
    Meshes.Clear();

    if (RenderMaterial!=NULL)
    {
        // Note that also the MatSys::Renderer pointer can be NULL, but then the RenderMaterial must be NULL, too.
        assert(MatSys::Renderer!=NULL);

        MatSys::Renderer->FreeMaterial(RenderMaterial);
        RenderMaterial=NULL;
    }
}


unsigned long BezierPatchNodeT::GetAutoSubdivsHorz() const
{
    // How many line segments do we need for a curve? Or: How far do we need to tesselate?
    // Consider a point triple A, B, C, where A and B are the curve endpoints and C is the control point.
    // Let L=(A+B)/2, R=(B+C)/2, N=(L+R)/2, H=(A+B)/2. Then HN==NC==HC/2==(2C-A-B)/4.
    // Thus, take HN (the "error" in world units) as a measure for tesselation,
    // as is cares both for the world size of the patch (ie. is the diameter of a pipe very big or very small),
    // as well as the severity of the deformation (almost flat curves get less triangles than very tight ones).

    // Auto-determine the size of the tesselation in x-direction (the number of columns per 3x3 sub-patch).
    // This is done by considering the horizontal 3x1 sub-rows of the entire patch (all 3x3 sub-patches get tesselated equally).
    float xDistMax=0.0;

    for (unsigned long y=0; y<SizeY; y++)
        for (unsigned long x=0; x+2<SizeX; x+=2)
        {
            const float xDist=length(ControlPointsXYZ[y*SizeX+(x+1)]*2.0f - ControlPointsXYZ[y*SizeX+x] - ControlPointsXYZ[y*SizeX+(x+2)])/4.0f;

            if (xDist>xDistMax) xDistMax=xDist;
        }

    return (unsigned long)(1+sqrt(xDistMax/10.0f));   // Number of triangle columns for each 3x3 sub-patch.
}


unsigned long BezierPatchNodeT::GetAutoSubdivsVert() const
{
    // Auto-determine the size of the tesselation in y-direction (the number of rows per 3x3 sub-patch).
    // This is done by considering the vertical 1x3 sub-columns of the entire patch (all 3x3 sub-patches get tesselated equally).
    float yDistMax=0.0;

    for (unsigned long y=0; y+2<SizeY; y+=2)
        for (unsigned long x=0; x<SizeX; x++)
        {
            const float yDist=length(ControlPointsXYZ[(y+1)*SizeX+x]*2.0f - ControlPointsXYZ[y*SizeX+x] - ControlPointsXYZ[(y+2)*SizeX+x])/4.0f;

            if (yDist>yDistMax) yDistMax=yDist;
        }

    return (unsigned long)(1+sqrt(yDistMax/10.0));   // Number of triangle rows    for each 3x3 sub-patch.
}


void BezierPatchNodeT::GenerateLightMapMesh(cf::math::BezierPatchT<float>& LightMapMesh, const float LightMapPatchSize, const bool ComputeTS) const
{
    float MaxLength = LightMapPatchSize * 2.0f;

    for (unsigned long RetryCount=0; RetryCount<10; RetryCount++)
    {
        LightMapMesh=cf::math::BezierPatchT<float>(SizeX, SizeY, ControlPointsXYZ, ControlPointsUV);

        if (ComputeTS) LightMapMesh.ComputeTangentSpace();

#if 1
        // This is the correct way to obtain the LightMapMesh - see the docs of BezierPatchT::ForceLinearMaxLength() for rationale.
        //
        // The casts to unsigned long are needed in order to resolve ambiguity of the overloaded Subdivide() method.
        if (SubdivsHorz>0 && SubdivsVert>0) LightMapMesh.Subdivide((unsigned long)SubdivsHorz, (unsigned long)SubdivsVert);
                                       else LightMapMesh.Subdivide(m_MaxError, /*MaxLength*/ -1.0f);

        LightMapMesh.ForceLinearMaxLength(MaxLength);
#else
        // This doesn't work as desired - see the docs of BezierPatchT::ForceLinearMaxLength() for rationale.
        //
        // No matter what the user provided via the SubdivsHorz and SubdivsVert parameters, we subdivide on automatic metrics here.
        // The m_MaxError is essentially an arbitrary number - we rely on the MaxLength parameter to get the mesh properly tesselated.
        // The MaxLength could be set to PS*4/3 because the average of PS*4/3 and PS*4/3*1/2 is PS, but the resulting meshes look
        // really fine-grained to me, so I increase from *4/3 to *2.
        LightMapMesh.Subdivide(m_MaxError, MaxLength, false /* Do not optimize "flat" rows and columns! */);
#endif

        // Maximum permitted size not exceeded? If so, we're done.
        if (LightMapMesh.Width <=cf::SceneGraph::LightMapManT::SIZE_S &&
            LightMapMesh.Height<=cf::SceneGraph::LightMapManT::SIZE_T) return;

        // Okay, this patch mesh is larger than the limits.
        // Double the error metrics and try again.
        printf("Patch %p: lightmap exceeds limits. I'll double the error metrics, then try again.\n", this);

        MaxLength*=2.0f;
    }

    // Maximum retry count exceeded?
    printf("Patch %p: Failed to assign initial lightmap.\n", this);
}
