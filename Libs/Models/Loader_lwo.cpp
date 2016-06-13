/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_lwo.hpp"
#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/Material.hpp"

extern "C"
{
    #include "lwo2.h"
}

#include <cassert>
#include <cstring>      // For strdup().
#include <map>

// For some reason, this one has not been defined in lwo2.h.
#define ID_TXUV  LWID_('T','X','U','V')


static inline const char* ChunkIdToString(unsigned long Chunk)
{
    static char String[5];

    String[0]=char(Chunk >> 24);
    String[1]=char(Chunk >> 16);
    String[2]=char(Chunk >>  8);
    String[3]=char(Chunk      );
    String[4]=0;

    return String;
}


/// Determines the UV-texture-coordinates for the given LWO polygon point.
static Vector3fT FindTexCoordForPolygonPoint(const lwLayer* Layer, const int PolyNr, const lwPolVert& PolyVert, const int VertexNr)
{
    // Find out which uv-coordinates should be used for this vertex.
    // Therefore, see if there is a VMAD of type TXUV for this vertex first.
    bool      FoundTC=false;
    Vector3fT TexCoord;

    for (int VMadNr=0; VMadNr<PolyVert.nvmaps; VMadNr++)
    {
        const lwVMap* VMad     =PolyVert.vm[VMadNr].vmap;
        const int     VMadIndex=PolyVert.vm[VMadNr].index;

        // If the VMad is not of type TXUV, skip it here.
        if (VMad->type!=ID_TXUV)
        {
            // Console->DevWarning(std::string("VMad of unsupported type ")+ChunkIdToString(VMad->type)+" ignored.\n");
            continue;
        }

        // Make sure it's really a VMAD (per-polygon mapping), not a VMAP (per-point mapping).
        if (VMad->perpoly==0)
        {
            Console->Warning("VMad is really a VMap - ignored.\n");
            continue;
        }

        // The dimension should always be 2 when we get here.
        if (VMad->dim!=2)
        {
            Console->Warning(cf::va("VMad of type TXUV has %i dimensions (2 expected, VMad ignored).\n", VMad->dim));
            continue;
        }

        // The VMAD of type TXUV should(?) only occur once.
        if (FoundTC)
        {
            Console->Warning("Multiple VMads of type TXUV per polygon point found. VMad ignored.\n");
            continue;
        }

        // Let's see if I got that right...
        assert(VMad->vindex[VMadIndex]==PolyVert.index);
        assert(VMad->pindex[VMadIndex]==PolyNr);

        FoundTC=true;
        TexCoord.x=VMad->val[VMadIndex][0];
        TexCoord.y=VMad->val[VMadIndex][1];
    }

    if (!FoundTC)
    {
        // Didn't find a uv-pair in the per-polygon VMADs, now try the VMAPs for the point.
        const lwPoint& Point=Layer->point.pt[PolyVert.index];

        for (int VMapNr=0; VMapNr<Point.nvmaps; VMapNr++)
        {
            const lwVMap* VMap     =Point.vm[VMapNr].vmap;
            const int     VMapIndex=Point.vm[VMapNr].index;

            // If the VMap is not of type TXUV, skip it here.
            if (VMap->type!=ID_TXUV)
            {
                // Console->DevWarning(std::string("VMap of unsupported type ")+ChunkIdToString(VMap->type)+" ignored.\n");
                continue;
            }

            // Make sure it's really a VMAP (per-point mapping), not a VMAD (per-polygon mapping).
            if (VMap->perpoly!=0)
            {
                Console->Warning("VMap is really a VMad - ignored.\n");
                continue;
            }

            // The dimension should always be 2 when we get here.
            if (VMap->dim!=2)
            {
                Console->Warning(cf::va("VMap of type TXUV has %i dimensions (2 expected, VMap ignored).\n", VMap->dim));
                continue;
            }

            // The VMAP of type TXUV should(?) only occur once.
            if (FoundTC)
            {
                Console->Warning("Multiple VMaps of type TXUV per point found. VMap ignored.\n");
                continue;
            }

            // Let's see if I got that right...
            assert(VMap->vindex[VMapIndex]==PolyVert.index);

            FoundTC=true;
            TexCoord.x=VMap->val[VMapIndex][0];
            TexCoord.y=VMap->val[VMapIndex][1];
        }
    }

    if (!FoundTC)
    {
        Console->Warning(cf::va("Layer %i, polygon %i: No TXUV data found for vertex %i.\n", Layer->index, PolyNr, VertexNr));
    }

    return TexCoord;
}


/* static void PrintLayerLWO(const lwLayer* Layer)
{
    // Print information about the layer itself.
    Console->Print(cf::va("Layer \"%s\", index %i, parent %i, flags %i, pivot (...), BB (...), points %i, polygons %i, num vmaps %i.\n",
        Layer->name!=NULL ? Layer->name : "NULL", Layer->index, Layer->parent, Layer->flags, Layer->point.count, Layer->polygon.count, Layer->nvmaps));


    // Print the list of polygons of the layer.
    for (int PolyNr=0; PolyNr<Layer->polygon.count; PolyNr++)
    {
        const lwPolygon& Poly=Layer->polygon.pol[PolyNr];

        Console->Print(cf::va("    Poly %i, surfname %s, part %i, smoothgrp %i, flags %i, type %s, norm (%f %f %f), verts %i.\n",
            PolyNr, Poly.surf->name, Poly.part, Poly.smoothgrp, Poly.flags, ChunkIdToString(Poly.type), Poly.norm[0], Poly.norm[1], Poly.norm[2], Poly.nverts));

        for (int VertexNr=0; VertexNr<Poly.nverts; VertexNr++)
        {
            const lwPolVert& PolyVert=Poly.v[VertexNr];

            Console->Print(cf::va("        Vertex %i: is at points[%i], norm (%f %f %f), nvmaps %i.\n", VertexNr, PolyVert.index, PolyVert.norm[0], PolyVert.norm[1], PolyVert.norm[2], PolyVert.nvmaps));
        }
    }


    // Print the list of points of the layer.
    for (int VertexNr=0; VertexNr<Layer->point.count; VertexNr++)
    {
        const lwPoint& Point=Layer->point.pt[VertexNr];

        Console->Print(cf::va("    Point %i: numpols %i, nvmaps %i%s.\n",
            VertexNr, Point.npols, Point.nvmaps, Point.nvmaps>0 ? "" : " (!!!)"));
    }


    // Print the list of VMAPs and VMADs of this layer.
    for (lwVMap* VMap=Layer->vmap; VMap!=NULL; VMap=VMap->next)
    {
        Console->Print(cf::va("VMap \"%s\", type %s, nverts*dim %i*%i, perpoly %i.\n",
            VMap->name!=NULL ? VMap->name : "NULL", ChunkIdToString(VMap->type), VMap->nverts, VMap->dim, VMap->perpoly));
    }
} */


// Note that the nomenclature in the LWO context is different from the usual (Cafu) lingo:
// - An LWO "layer" corresponds to a CafuModelT::MeshT (however, whenever multiple materials occur in an LWO layer,
//   the current code models that layer with as many CafuModelT::MeshTs as there are materials in the layer).
// - An LWO "surface" corresponds to a Cafu MatSys "material" (MaterialT).
// - An LWO "polygon" can also mean something more complex than a planar surface, there are several polygon types.
// - LWOs store their vertex positions in "points", the CafuModelT class in "weights" (CafuModelT::MeshT::WeightT).
// - LWOs store their texture-coordinates in "VMAPs" and "VMADs", the CafuModelT class in "vertices" (CafuModelT::MeshT::VertexT).
LoaderLwoT::LoaderLwoT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags | REMOVE_UNUSED_VERTICES | REMOVE_UNUSED_WEIGHTS)    // The code below relies on postprocessing removing unused vertices and weights.
{
}


void LoaderLwoT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    char*     fname=strdup(m_FileName.c_str());
    lwObject* lwo=lwGetObject(fname, NULL, NULL);

    free(fname); fname=NULL;
    if (lwo==NULL)
    {
        throw LoadErrorT("The model could not be loaded.\n");
    }


    // Create a default "identity" joint.
    // That single joint is used for (shared by) all weights of all meshes.
    Joints.PushBackEmpty();

    Joints[0].Name  ="root";
    Joints[0].Parent=-1;
 // Joints[0].Pos   =Vector3fT();
 // Joints[0].Qtr   =Vector3fT();   // Identity quaternion...
    Joints[0].Scale =Vector3fT(1.0f, 1.0f, 1.0f);

    ArrayT<unsigned long> PolygonMeshVertices;   // An auxiliary array used below.


    for (lwLayer* Layer=lwo->layer; Layer!=NULL; Layer=Layer->next)
    {
        // PrintLayerLWO(Layer);

        // For each LWO layer, create a matching MeshT in Meshes.
        // LWO layers and MeshTs match nicely wrt. the vertices, but note that MeshTs can only have a single material for all triangles,
        // whereas with LWO layers, theoretically each *polygon* can have an individual material.
        // We solve/work-around the problem by creating one MeshT for each material used in the layer,
        // initially duplicating the vertices and removing all unused ones in postprocessing (see Flags in ctor).
        std::map<lwSurface*, unsigned long> MatToMeshNr;     // Maps lwSurface pointers to indices into Meshes.

        // This loop is only to create a separate mesh for each material in the layer.
        for (int PolyNr=0; PolyNr<Layer->polygon.count; PolyNr++)
        {
            const lwPolygon& Poly=Layer->polygon.pol[PolyNr];

            if (Poly.type!=ID_FACE) continue;   // Silently ignore non-face polygons here, an excplicit warning is issued below. Note: No Meshes entry is created for surfaces that only exist on non-face polygons!
            if (MatToMeshNr.find(Poly.surf)!=MatToMeshNr.end()) continue;

            Meshes.PushBackEmpty();
            MatToMeshNr[Poly.surf]=Meshes.Size()-1;
            CafuModelT::MeshT& Mesh=Meshes[Meshes.Size()-1];

            Mesh.Name="Mesh";
            Mesh.Material=MaterialMan.GetMaterial(Poly.surf->name);

            if (!Mesh.Material)
            {
                // We don't have enough data in order to be able to reasonably reconstruct materials here.
                // Thus if there isn't an appropriately prepared .cmat file (so that MatName is found in MaterialMan),
                // go for the wire-frame substitute straight away.
                Mesh.Material=MaterialMan.RegisterMaterial(CreateDefaultMaterial(Poly.surf->name));
            }

            Mesh.Weights.PushBackEmpty(Layer->point.count);

            for (int VertexNr=0; VertexNr<Layer->point.count; VertexNr++)
            {
                const lwPoint& Point=Layer->point.pt[VertexNr];

                Mesh.Weights[VertexNr].JointIdx=0;
                Mesh.Weights[VertexNr].Weight  =1.0f;
                Mesh.Weights[VertexNr].Pos     =Vector3fT(Point.pos);
            }
        }

        if (MatToMeshNr.size()>1)
            Console->Warning(cf::va("LWO layer uses %lu materials and thus has been split into %lu meshes.\n", MatToMeshNr.size(), MatToMeshNr.size()));


        // For each LWO surface (Cafu material), there is one mesh in Meshes.
        // So loop over the meshes, and for each mesh, loop over those polygons whose material matches that of the mesh (and skip the other polygons).
        // Then loop over the polygon vertices, in order to determine their uv-coordinates and eventually build the triangles list.
        for (std::map<lwSurface*, unsigned long>::const_iterator It=MatToMeshNr.begin(); It!=MatToMeshNr.end(); ++It)
        {
            CafuModelT::MeshT& Mesh=Meshes[It->second];

            // For each weight in Mesh.Weights[], which corresponds to Layer->point.pt[], keep track of how many Mesh.Vertices[] refer to that weight.
            // This is done by keeping a Mesh.Vertices[] indices list in parallel to the Mesh.Weights[] array.
            ArrayT< ArrayT<unsigned long> > WeightVerts;
            WeightVerts.PushBackEmpty(Mesh.Weights.Size());

            for (int PolyNr=0; PolyNr<Layer->polygon.count; PolyNr++)
            {
                const lwPolygon& Poly=Layer->polygon.pol[PolyNr];

                // Only consider (loop over) polygons that have the material (surface) of the current Mesh.
                if (Poly.surf!=It->first) continue;

                if (Poly.type!=ID_FACE)
                {
                    Console->Warning(std::string("Polygon of unsupported type ")+ChunkIdToString(Poly.type)+" ignored.\n");
                    continue;
                }


                // Find or create all the Mesh.Vertices (uv-coordinates) that we need for this polygon,
                // and keep their array indices for creating the triangles below.
                PolygonMeshVertices.Overwrite();

                for (int VertexNr=0; VertexNr<Poly.nverts; VertexNr++)
                {
                    const lwPolVert& PolyVert=Poly.v[VertexNr];
                    const Vector3fT  TexCoord=FindTexCoordForPolygonPoint(Layer, PolyNr, PolyVert, VertexNr);

                    // Okay, now lets see if for point Layer->point.pt[PolyVert.index], or rather weight Mesh.Weights[PolyVert.index],
                    // we already have something in Mesh.Vertices[...] with (TexCoord.x, TexCoord.y). Create a new Mesh.Vertices entry otherwise.
                    unsigned long Nr;

                    for (Nr=0; Nr<WeightVerts[PolyVert.index].Size(); Nr++)
                    {
                        const unsigned long               MeshVertexNr=WeightVerts[PolyVert.index][Nr];
                        const CafuModelT::MeshT::VertexT& MeshVertex  =Mesh.Vertices[MeshVertexNr];

                        assert(MeshVertex.FirstWeightIdx==(unsigned int)PolyVert.index);
                        assert(MeshVertex.NumWeights==1);

                        if (MeshVertex.u==TexCoord.x && MeshVertex.v==1.0f-TexCoord.y)
                        {
                            PolygonMeshVertices.PushBack(MeshVertexNr);
                            break;
                        }
                    }

                    if (Nr<WeightVerts[PolyVert.index].Size()) continue;

                    // No vertex for Mesh.Weights[PolyVert.index] was found that has the required uv-coordinates, so add a new one.
                    Mesh.Vertices.PushBackEmpty();
                    unsigned long               MeshVertexNr=Mesh.Vertices.Size()-1;
                    CafuModelT::MeshT::VertexT& MeshVertex  =Mesh.Vertices[MeshVertexNr];

                    MeshVertex.u             =TexCoord.x;
                    MeshVertex.v             =1.0f-TexCoord.y;      // LightWave seems to assume the texture image origin in a different place than we do...
                    MeshVertex.FirstWeightIdx=PolyVert.index;
                    MeshVertex.NumWeights    =1;

                    WeightVerts[PolyVert.index].PushBack(MeshVertexNr);
                    PolygonMeshVertices.PushBack(MeshVertexNr);
                }

                assert(int(PolygonMeshVertices.Size())==Poly.nverts);


                // Create the triangle list for this mesh (assuming that Poly is always convex...).
                for (unsigned long VertexNr=0; VertexNr+2<PolygonMeshVertices.Size(); VertexNr++)
                {
                    Mesh.Triangles.PushBackEmpty();
                    unsigned long                 TriangleNr=Mesh.Triangles.Size()-1;
                    CafuModelT::MeshT::TriangleT& Triangle  =Mesh.Triangles[TriangleNr];

                    Triangle.VertexIdx[0]=PolygonMeshVertices[         0];
                    Triangle.VertexIdx[1]=PolygonMeshVertices[VertexNr+2];
                    Triangle.VertexIdx[2]=PolygonMeshVertices[VertexNr+1];

                    if (Poly.smoothgrp<0 || Poly.smoothgrp>31)
                        Console->Warning(cf::va("Polygon %i is in smoothing group %i, but should be in 0...31.\n", PolyNr, Poly.smoothgrp));

                    Triangle.SmoothGroups=uint32_t(1) << Poly.smoothgrp;
                }
            }
        }
    }

    lwFreeObject(lwo);

    // for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    //     Console->Print(cf::va("Mesh %lu: %lu weights, %lu vertices, %lu triangles.\n", MeshNr, Meshes[MeshNr].Weights.Size(), Meshes[MeshNr].Vertices.Size(), Meshes[MeshNr].Triangles.Size()));
}
