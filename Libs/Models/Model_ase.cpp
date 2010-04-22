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

/*****************/
/*** ase Model ***/
/*****************/

#include <stdio.h>
#include <assert.h>

#include "Model_ase.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "TextParser/TextParser.hpp"

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


void ModelAseT::ReadMaterials(TextParserT& TP)
{
    // Last seen token was "*MATERIAL_LIST".
    TP.AssertAndSkipToken("{");
    TP.AssertAndSkipToken("*MATERIAL_COUNT");

    const unsigned long ExpectedNrOfMaterials=strtoul(TP.GetNextToken().c_str(), NULL, 0);

    for (unsigned long MaterialNr=0; MaterialNr<ExpectedNrOfMaterials; MaterialNr++)
    {
        TP.AssertAndSkipToken("*MATERIAL");
        if (strtoul(TP.GetNextToken().c_str(), NULL, 0)!=MaterialNr) throw TextParserT::ParseError();
        TP.AssertAndSkipToken("{");

        // Now do just search for "*MATERIAL_NAME" in this material block.
        while (true)
        {
            std::string Token=TP.GetNextToken();

            if (Token=="*MATERIAL_NAME")
            {
                // Good, found the material name.
                MaterialNames.PushBack(TP.GetNextToken());  // Store the material name.
                TP.SkipBlock("{", "}", true);               // Skip the rest of this *MATERIAL block.
                break;                                      // Proceed with the next material.
            }
            else if (Token=="{")
            {
                // Nested block found - skip it and continue the search.
                TP.SkipBlock("{", "}", true);
            }
            else if (Token=="}")
            {
                // Bad - end of material found, but no material name.
                printf("Missing \"*MATERIAL_NAME\" in material %lu.\n", MaterialNr);
                MaterialNames.PushBack("NoMaterial");   // Store a dummy name.
                break;                                  // Proceed with the next material.
            }
            else
            {
                // Otherwise, just continue the search.
                continue;
            }
        }
    }

    TP.AssertAndSkipToken("}");
}


void ModelAseT::GeomObjectT::ReadMesh(TextParserT& TP)
{
    // Last seen token was "*MESH".
    TP.AssertAndSkipToken("{");

    if (Vertices.Size()>0 || TexCoords.Size()>0 || Triangles.Size()>0)
    {
        printf("Warning: More than one mesh per \"*GEOMOBJECT\"!? Skipping...\n");
        TP.SkipBlock("{", "}", true);
        return;
    }

    unsigned long ExpectedNrOfVertices =0;
    unsigned long ExpectedNrOfFaces    =0;
    unsigned long ExpectedNrOfTexCoords=0;

    while (true)
    {
        const std::string Token=TP.GetNextToken();

             if (Token=="*TIMEVALUE"       ) TP.GetNextToken(); // Ignore the timevalue.
        else if (Token=="*MESH_NUMVERTEX"  ) ExpectedNrOfVertices =strtoul(TP.GetNextToken().c_str(), NULL, 0);
        else if (Token=="*MESH_VERTEX_LIST")
        {
            TP.AssertAndSkipToken("{");

            for (unsigned long VertexNr=0; VertexNr<ExpectedNrOfVertices; VertexNr++)
            {
                TP.AssertAndSkipToken("*MESH_VERTEX");
                if (strtoul(TP.GetNextToken().c_str(), NULL, 0)!=VertexNr) throw TextParserT::ParseError();

                // This cannot be collapsed to VectorT(atof(...), atof(...), atof(...)), because
                // the order of the calls to atof(...) is then not necessarily from left to right!
                VectorT Vert;
                Vert.x=atof(TP.GetNextToken().c_str());
                Vert.y=atof(TP.GetNextToken().c_str());
                Vert.z=atof(TP.GetNextToken().c_str());
                Vertices.PushBack(Vert);
            }

            TP.AssertAndSkipToken("}");
        }
        else if (Token=="*MESH_NUMFACES"   ) ExpectedNrOfFaces    =strtoul(TP.GetNextToken().c_str(), NULL, 0);
        else if (Token=="*MESH_FACE_LIST"  )
        {
            TP.AssertAndSkipToken("{");

            Triangles.PushBackEmpty(ExpectedNrOfFaces-Triangles.Size());

            for (unsigned long FaceNr=0; FaceNr<ExpectedNrOfFaces; FaceNr++)
            {
                TP.AssertAndSkipToken("*MESH_FACE");
                // The colon in the token (ex.: "0:") is ignored by strtoul().
                if (strtoul(TP.GetNextToken().c_str(), NULL, 0)!=FaceNr) throw TextParserT::ParseError();

                TP.AssertAndSkipToken("A:"); Triangles[FaceNr].IndVertices[0]=strtoul(TP.GetNextToken().c_str(), NULL, 0);
                TP.AssertAndSkipToken("B:"); Triangles[FaceNr].IndVertices[1]=strtoul(TP.GetNextToken().c_str(), NULL, 0);
                TP.AssertAndSkipToken("C:"); Triangles[FaceNr].IndVertices[2]=strtoul(TP.GetNextToken().c_str(), NULL, 0);

                TP.AssertAndSkipToken("AB:"); TP.GetNextToken();    // Ignore this edge flag.
                TP.AssertAndSkipToken("BC:"); TP.GetNextToken();    // Ignore this edge flag.
                TP.AssertAndSkipToken("CA:"); TP.GetNextToken();    // Ignore this edge flag.

                TP.AssertAndSkipToken("*MESH_SMOOTHING");
                while (true)
                {
                    std::string SmoothGroupString=TP.GetNextToken();

                    // Note 1: TP has the comma among its delimiters!
                    // Note 2: If we ever change this because some faces have the "*MESH_MTLID" keyword not specified at all,
                    // remeber to not ignore the "*MATERIAL_REF" keyword any longer, and fix all faces with no "*MESH_MTLID" after loading!
                    if (SmoothGroupString=="*MESH_MTLID") break;
                    Triangles[FaceNr].SmoothGroups.PushBack(strtoul(SmoothGroupString.c_str(), NULL, 0));
                    if (Triangles[FaceNr].SmoothGroups.Size()>32) throw TextParserT::ParseError();   // Safe-Guard...
                }

                // WRONG!!
                // It seems that the number after *MESH_MTLID describes some logical group,
                // like the six sides of a box. What WE need is the number after *MATERIAL_REF.
                // Triangles[FaceNr].IndMaterial=strtoul(TP.GetNextToken().c_str(), NULL, 0);
                TP.GetNextToken();  // Right: Ignore the sub-material number.
            }

            TP.AssertAndSkipToken("}");
        }
        else if (Token=="*MESH_NUMTVERTEX" ) ExpectedNrOfTexCoords=strtoul(TP.GetNextToken().c_str(), NULL, 0);
        else if (Token=="*MESH_TVERTLIST"  )
        {
            TP.AssertAndSkipToken("{");

            for (unsigned long TexCoordNr=0; TexCoordNr<ExpectedNrOfTexCoords; TexCoordNr++)
            {
                TP.AssertAndSkipToken("*MESH_TVERT");
                if (strtoul(TP.GetNextToken().c_str(), NULL, 0)!=TexCoordNr) throw TextParserT::ParseError();

                // This cannot be collapsed to VectorT(atof(...), atof(...), atof(...)), because
                // the order of the calls to atof(...) is then not necessarily from left to right!
                VectorT TexCoord;
                TexCoord.x= atof(TP.GetNextToken().c_str());
                TexCoord.y=-atof(TP.GetNextToken().c_str());
                TexCoord.z= atof(TP.GetNextToken().c_str());
                TexCoords.PushBack(TexCoord);
            }

            TP.AssertAndSkipToken("}");
        }
        else if (Token=="*MESH_NUMTVFACES" )
        {
            unsigned long ExpectedNrOfTextureFaces=strtoul(TP.GetNextToken().c_str(), NULL, 0);

            if (ExpectedNrOfFaces==0 || ExpectedNrOfTextureFaces!=ExpectedNrOfFaces)
                printf("Warning: Expected number of faces: %lu, expected number of texture faces: %lu.\n", ExpectedNrOfFaces, ExpectedNrOfTextureFaces);
        }
        else if (Token=="*MESH_TFACELIST"  )
        {
            TP.AssertAndSkipToken("{");

            Triangles.PushBackEmpty(ExpectedNrOfFaces-Triangles.Size());

            for (unsigned long FaceNr=0; FaceNr<ExpectedNrOfFaces; FaceNr++)
            {
                TP.AssertAndSkipToken("*MESH_TFACE");
                if (strtoul(TP.GetNextToken().c_str(), NULL, 0)!=FaceNr) throw TextParserT::ParseError();

                Triangles[FaceNr].IndTexCoords[0]=strtoul(TP.GetNextToken().c_str(), NULL, 0);
                Triangles[FaceNr].IndTexCoords[1]=strtoul(TP.GetNextToken().c_str(), NULL, 0);
                Triangles[FaceNr].IndTexCoords[2]=strtoul(TP.GetNextToken().c_str(), NULL, 0);
            }

            TP.AssertAndSkipToken("}");
        }
        else if (Token=="*MESH_MAPPINGCHANNEL")
        {
            // Ignore additional sets of TexCoords. I may change my mind later, though!
            TP.GetNextToken();
            TP.SkipBlock("{", "}", false);
        }
        else if (Token=="*MESH_NUMCVERTEX") TP.GetNextToken();              // Ignore the number of color vertices.
        else if (Token=="*MESH_CVERTLIST" ) TP.SkipBlock("{", "}", false);  // Ignore the per-vertex color list.
        else if (Token=="*MESH_NUMCVFACES") TP.GetNextToken();              // Ignore the number of color faces.
        else if (Token=="*MESH_CFACELIST" ) TP.SkipBlock("{", "}", false);  // Ignore the per-face color specifications.
        else if (Token=="*MESH_NORMALS"   ) TP.SkipBlock("{", "}", false);  // Ignore the normals - we compute the entire tangent space ourselves!
        else if (Token=="}"               ) break;                          // End of mesh.
        else
        {
            // Unknown token!
            // If the next token is a block start, skip the block.
            // Otherwise it was something else that we just throw away.
            printf("Unknown token \"%s\", skipping...\n", Token.c_str());
            if (TP.GetNextToken()=="{") TP.SkipBlock("{", "}", true);
        }
    }


    // Run a few sanity checks on the just loaded mesh.
    // Doing so was motivated by models that have faces (triangles) specified, but no texture-coordinates.
    unsigned long MaxVertexIndex  =0;
    unsigned long MaxTexCoordIndex=0;

    for (unsigned long TriNr=0; TriNr<Triangles.Size(); TriNr++)
    {
        TriangleT& Tri=Triangles[TriNr];

        for (unsigned long i=0; i<3; i++)
        {
            if (Tri.IndVertices [i]>MaxVertexIndex  ) MaxVertexIndex  =Tri.IndVertices [i];
            if (Tri.IndTexCoords[i]>MaxTexCoordIndex) MaxTexCoordIndex=Tri.IndTexCoords[i];
        }
    }

    // "Fix" the model by pushing back "dummy" array elements...
    while (MaxVertexIndex  >=Vertices .Size()) Vertices .PushBackEmpty();
    while (MaxTexCoordIndex>=TexCoords.Size()) TexCoords.PushBackEmpty();
}


void ModelAseT::ReadGeometry(TextParserT& TP)
{
    // Last seen token was "*GEOMOBJECT".
    TP.AssertAndSkipToken("{");

    GeomObjects.PushBackEmpty();
    GeomObjectT& GO=GeomObjects[GeomObjects.Size()-1];

    GO.IndexMaterial=0;
    GO.CastShadows=false;

    while (true)
    {
        const std::string Token=TP.GetNextToken();

             if (Token=="*NODE_NAME"      ) TP.GetNextToken();              // Ignore the name of this object.
        else if (Token=="*NODE_TM"        ) TP.SkipBlock("{", "}", false);  // Ignore the transformation matrix specification. May change my mind later, though!
        else if (Token=="*MESH"           ) GO.ReadMesh(TP);
        else if (Token=="*PROP_MOTIONBLUR") TP.GetNextToken();              // Ignore the "motion blur" property.
        else if (Token=="*PROP_CASTSHADOW") GO.CastShadows=(TP.GetNextToken()!="0");
        else if (Token=="*PROP_RECVSHADOW") TP.GetNextToken();              // Ignore the "receive shadow" property.
        else if (Token=="*TM_ANIMATION"   ) TP.SkipBlock("{", "}", false);  // Ignore the specification of the transformation matrix for animation.
        else if (Token=="*MATERIAL_REF"   ) GO.IndexMaterial=strtoul(TP.GetNextToken().c_str(), NULL, 0);
        else if (Token=="}"               ) break;                          // End of object.
        else
        {
            // Unknown token!
            // If the next token is a block start, skip the block.
            // Otherwise it was something else that we just throw away.
            printf("Unknown token \"%s\", skipping...\n", Token.c_str());
            if (TP.GetNextToken()=="{") TP.SkipBlock("{", "}", true);
        }
    }
}


static bool CompareGOIDs(const unsigned long& Pair1, const unsigned long& Pair2)
{
    // Just compare the lower WORDs with the material index, the upper WORDs contain the GONr.
    return (Pair1 & 0xFFFF)<(Pair2 & 0xFFFF);
}


ModelAseT::ModelAseT(const std::string& FileName_) /*throw (ModelT::LoadError)*/
    : FileName(FileName_),
      Gui_GeomObjNr(-1)
{
    TextParserT TP(FileName.c_str(), ",");  // The comma is required for SmoothGroups.

    try
    {
        // A good description of the ASE file format can be found at
        // http://www.unrealwiki.com/wiki/ASE_File_Format
        while (!TP.IsAtEOF())
        {
            const std::string Token=TP.GetNextToken();

                 if (Token=="*3DSMAX_ASCIIEXPORT") TP.GetNextToken();               // Ignore the version.
            else if (Token=="*COMMENT"           ) TP.GetNextToken();               // Ignore the comment.
            else if (Token=="*SCENE"             ) TP.SkipBlock("{", "}", false);   // Ignore the scene description.
            else if (Token=="*MATERIAL_LIST"     ) ReadMaterials(TP);
            else if (Token=="*GEOMOBJECT"        ) ReadGeometry(TP);
            else if (Token=="*LIGHTOBJECT"       ) TP.SkipBlock("{", "}", false);
            else if (Token=="*CAMERAOBJECT"      ) TP.SkipBlock("{", "}", false);
            else
            {
                // Unknown token!
                // If the next token is a block start, skip the block.
                // Otherwise it was something else that we just throw away.
                printf("Unknown token \"%s\", skipping...\n", Token.c_str());
                if (TP.GetNextToken()=="{") TP.SkipBlock("{", "}", true);
            }
        }

        if (MaterialNames.Size()==0) throw TextParserT::ParseError();
        if (GeomObjects  .Size()==0) throw TextParserT::ParseError();
    }
    catch (const TextParserT::ParseError&)
    {
        throw ModelT::LoadError();
    }


    // TODO: Sort the triangles according to their material!   Profile!
    //       Can we even do this across GeomObjects? Without introducing another layer of indirection?
    //       Probably yes, by simply loading everything into one big triangle list...  hehe.
    //       The question is if this is reasonable wrt. silhouette detection... yes, I think so;
    //       one big mesh offers more opportunity for fewer silhouette edges than many small!
    // Then get one MeshT(MeshT::Triangles) per batch of triangles with common material.
    // Will never be more than there are materials (will be exactly as many as there are used materials).


    // Find the SmoothGroup with the highest ID.
    // We need this below in order to be able to obtain more unique SmoothGroup IDs.
    unsigned long NextSmoothGroupID=0;

    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
        for (unsigned long TriangleNr=0; TriangleNr<GeomObjects[GONr].Triangles.Size(); TriangleNr++)
        {
            const GeomObjectT::TriangleT& Tri=GeomObjects[GONr].Triangles[TriangleNr];

            for (unsigned long SGNr=0; SGNr<Tri.SmoothGroups.Size(); SGNr++)
                if (Tri.SmoothGroups[SGNr]>=NextSmoothGroupID)
                    NextSmoothGroupID=Tri.SmoothGroups[SGNr]+1;
        }

    // Now assign each triangle that is in NO SmoothGroup an own, UNIQUE SmoothGroup.
    // This avoids special-case treatment below, and is intentionally done ACROSS GeomObjects (rather than per GeomObject).
    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
        for (unsigned long TriangleNr=0; TriangleNr<GeomObjects[GONr].Triangles.Size(); TriangleNr++)
        {
            GeomObjectT::TriangleT& Tri=GeomObjects[GONr].Triangles[TriangleNr];

            if (Tri.SmoothGroups.Size()==0) Tri.SmoothGroups.PushBack(NextSmoothGroupID++);
        }


    // Compute the tangent space and neighbourhood relationship.
    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        GeomObjectT& GO=GeomObjects[GONr];

        // Precompute the tangent-space of each triangle. Needed below as sources for computing the averages (per-vertex).
     // ArrayT<VectorT> PerTriNormals;      // Stored with each triangle, as we need the info also for the stencil shadows.
        ArrayT<VectorT> PerTriTangents;
        ArrayT<VectorT> PerTriBiNormals;

        for (unsigned long TriangleNr=0; TriangleNr<GO.Triangles.Size(); TriangleNr++)
        {
            GeomObjectT::TriangleT& Tri=GO.Triangles[TriangleNr];

            // Understanding what's going on here is easy. The key statement is
            // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
            // First, there is a short explanation in "The Cg Tutorial", chapter 8.
            // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
            const VectorT Edge01=GO.Vertices[Tri.IndVertices[1]]-GO.Vertices[Tri.IndVertices[0]];
            const VectorT Edge02=GO.Vertices[Tri.IndVertices[2]]-GO.Vertices[Tri.IndVertices[0]];
            const VectorT uv01  =GO.TexCoords[Tri.IndTexCoords[1]]-GO.TexCoords[Tri.IndTexCoords[0]];
            const VectorT uv02  =GO.TexCoords[Tri.IndTexCoords[2]]-GO.TexCoords[Tri.IndTexCoords[0]];
            const double  f     =uv01.x*uv02.y-uv01.y*uv02.x>0.0 ? 1.0 : -1.0;

            const VectorT Normal  =cross(Edge01, Edge02);
            const VectorT Tangent =scale(Edge02, -uv01.y*f) + scale(Edge01, uv02.y*f);
            const VectorT BiNormal=scale(Edge02,  uv01.x*f) - scale(Edge01, uv02.x*f);

            const double  NormalL  =length(Normal  );
            const double  TangentL =length(Tangent );
            const double  BiNormalL=length(BiNormal);

                          Tri.Normal=NormalL  >0.000001 ? scale(Normal  , 1.0/NormalL  ) : VectorT(0, 0, 1);
            PerTriTangents .PushBack(TangentL >0.000001 ? scale(Tangent , 1.0/TangentL ) : VectorT(1, 0, 0));
            PerTriBiNormals.PushBack(BiNormalL>0.000001 ? scale(BiNormal, 1.0/BiNormalL) : VectorT(0, 1, 0));
        }

        // Precompute a table that stores for each vertex which triangles refer to it.
        // This accelerates the subsequent computations a lot.
        ArrayT< ArrayT<unsigned long> > TrianglesForVertex;

        TrianglesForVertex.PushBackEmpty(GO.Vertices.Size());

        for (unsigned long TriangleNr=0; TriangleNr<GO.Triangles.Size(); TriangleNr++)
        {
            GeomObjectT::TriangleT& Tri=GO.Triangles[TriangleNr];

            TrianglesForVertex[Tri.IndVertices[0]].PushBack(TriangleNr);
            TrianglesForVertex[Tri.IndVertices[1]].PushBack(TriangleNr);
            TrianglesForVertex[Tri.IndVertices[2]].PushBack(TriangleNr);
        }

        // Compute the tangent space for each vertex of each triangle.
        for (unsigned long TriangleNr=0; TriangleNr<GO.Triangles.Size(); TriangleNr++)
        {
            GeomObjectT::TriangleT& Tri=GO.Triangles[TriangleNr];

            for (unsigned long VNr=0; VNr<3; VNr++)
            {
                Tri.Normals  [VNr]=VectorT(0, 0, 0);
                Tri.Tangents [VNr]=VectorT(0, 0, 0);
                Tri.BiNormals[VNr]=VectorT(0, 0, 0);

                // Simply loop over those triangles that are known to refer to Tri.IndVertices[VNr].
                // This speeds up this code a lot. The old loop simply was: for (unsigned long T2Nr=0; T2Nr<GO.Triangles.Size(); T2Nr++)
                for (unsigned long RefNr=0; RefNr<TrianglesForVertex[Tri.IndVertices[VNr]].Size(); RefNr++)
                {
                    // Note that Tri==Tri2 is well possible (a normal case), thanks to the removal of "no" smoothgroups above.
                    const unsigned long           T2Nr=TrianglesForVertex[Tri.IndVertices[VNr]][RefNr];
                    const GeomObjectT::TriangleT& Tri2=GO.Triangles[T2Nr];

                    // See if Tri and Tri2 are in the same SmoothGroup (that is, if their sets of SmoothGroups intersect).
                    bool DoIntersect=false;

                    for (unsigned long SG1Nr=0; SG1Nr<Tri.SmoothGroups.Size() && !DoIntersect; SG1Nr++)
                        for (unsigned long SG2Nr=0; SG2Nr<Tri2.SmoothGroups.Size() && !DoIntersect; SG2Nr++)
                            if (Tri.SmoothGroups[SG1Nr]==Tri2.SmoothGroups[SG2Nr])
                                DoIntersect=true;

                    // Don't intersect? Then Tri and Tri2 are not in a common smoothgroup.
                    if (!DoIntersect) continue;

                    // If Tri2 doesn't share Tri.IndVertices[VNr], Tri2 does not influence the tangent space at that vertex.
                    // Update: I don't think that this condition is required any more!
                    // It made sense when we looped over *all* triangles of the GO, but as we now only loop over those that
                    // refer to Tri.IndVertices[VNr] anyway, Tri.IndVertices[VNr] is trivially shared by Tri2.
                    if (Tri.IndVertices[VNr]!=Tri2.IndVertices[0] &&
                        Tri.IndVertices[VNr]!=Tri2.IndVertices[1] &&
                        Tri.IndVertices[VNr]!=Tri2.IndVertices[2]) continue;

                    // Okay, Tri and Tri2 are in a common smoothgroup, and share the current vertex.
                    Tri.Normals  [VNr]=Tri.Normals  [VNr]+Tri2.Normal;
                    Tri.Tangents [VNr]=Tri.Tangents [VNr]+PerTriTangents [T2Nr];
                    Tri.BiNormals[VNr]=Tri.BiNormals[VNr]+PerTriBiNormals[T2Nr];
                }

                // Normalize the tangent space vectors.
                double Len;

                Len=length(Tri.Normals  [VNr]); Tri.Normals  [VNr]=(Len>0.000001) ? scale(Tri.Normals  [VNr], 1.0/Len) : Tri.Normal;
                Len=length(Tri.Tangents [VNr]); Tri.Tangents [VNr]=(Len>0.000001) ? scale(Tri.Tangents [VNr], 1.0/Len) : PerTriTangents [TriangleNr];
                Len=length(Tri.BiNormals[VNr]); Tri.BiNormals[VNr]=(Len>0.000001) ? scale(Tri.BiNormals[VNr], 1.0/Len) : PerTriBiNormals[TriangleNr];
            }
        }

        // Pre-compute all neighbourhood relationships.
        for (unsigned long TriangleNr=0; TriangleNr<GO.Triangles.Size(); TriangleNr++)
        {
            GeomObjectT::TriangleT& Tri=GO.Triangles[TriangleNr];

            // Init with "no neighbours".
            Tri.Neighbours[0]=-1;
            Tri.Neighbours[1]=-1;
            Tri.Neighbours[2]=-1;
        }

        // Important note: If three triangles share a common edge, the relevant edge of *all* three triangles must be flagged with -2
        // (have multiple neighbours at this edge, treat like it was a free edge with no neighbour).
        // However, the fact that the three triangles share a common edge IS TYPICALLY DETECTED FOR ONLY *ONE* OF THE THREE TRIANGLES,
        // namely the one that has an orientation different from the two others.
        // We therefore also have to modify other triangles except for Tri1 at iteration Tri1Nr in order to make sure that all
        // triangle-edges at a triply-shared edge are set to -2 when such a case is detected.
        for (unsigned long Tri1Nr=0; Tri1Nr<GO.Triangles.Size(); Tri1Nr++)
        {
            GeomObjectT::TriangleT& Tri1=GO.Triangles[Tri1Nr];

            for (unsigned long v1=0; v1<3; v1++)
            {
                // Note that the neighbour of edge <v1, (v1+1) % 3> is contained in the set of triangles that refer to v1.
                for (unsigned long RefNr=0; RefNr<TrianglesForVertex[Tri1.IndVertices[v1]].Size(); RefNr++)
                {
                    const unsigned long     Tri2Nr=TrianglesForVertex[Tri1.IndVertices[v1]][RefNr];
                    GeomObjectT::TriangleT& Tri2  =GO.Triangles[Tri2Nr];

                    if (Tri1Nr==Tri2Nr) continue;

                    for (unsigned long v2=0; v2<3; v2++)
                        if (Tri1.IndVertices[v1]==Tri2.IndVertices[(v2+1) % 3] && Tri1.IndVertices[(v1+1) % 3]==Tri2.IndVertices[v2])
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
                                GeomObjectT::TriangleT& Tri3=GO.Triangles[Tri1.Neighbours[v1]];
                                unsigned long           v3;

                                for (v3=0; v3<2; v3++)      // The  v3<2  instead of  v3<3  is intentional, to be safe that v3 never gets 3 (out-of-range).
                                    if (Tri1.IndVertices[v1]==Tri3.IndVertices[(v3+1) % 3] && Tri1.IndVertices[(v1+1) % 3]==Tri3.IndVertices[v3])
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
        }
    }


    // Register the render materials (but only those that are really needed by the GeomObjects).
    ArrayT<bool> IsMatNeeded;

    for (unsigned long MaterialNr=0; MaterialNr<MaterialNames.Size(); MaterialNr++) IsMatNeeded.PushBack(false);
    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        // Ase files can come with too few materials specified.
        // Make sure that there are enough of them, or else we'll crash with array-index-out-of-bounds later.
        if (GeomObjects[GONr].IndexMaterial>=MaterialNames.Size())
        {
            printf("ase model error (%s): GeomObject %lu refers to material index %lu, but there are only %lu materials total.\n", FileName.c_str(), GONr, GeomObjects[GONr].IndexMaterial, MaterialNames.Size());
            throw ModelT::LoadError();
        }

        IsMatNeeded[GeomObjects[GONr].IndexMaterial]=true;
    }

    for (unsigned long MaterialNr=0; MaterialNr<MaterialNames.Size(); MaterialNr++)
    {
        if (IsMatNeeded[MaterialNr])
        {
            MaterialT* Material=MaterialManager->GetMaterial(MaterialNames[MaterialNr]);

            if (Material==NULL)
            {
                printf("\nWARNING: Material '%s' not found!\n", MaterialNames[MaterialNr].c_str());
                Materials.PushBack(NULL);
                RenderMaterials.PushBack(NULL);
                continue;
            }

            if (!Material->LightMapComp.IsEmpty())
            {
                printf("\nWARNING: Model \"%s\" uses material \"%s\", which in turn has lightmaps defined.\n", FileName.c_str(), MaterialNames[MaterialNr].c_str());
                printf("It will work in the ModelViewer, but for other applications like Cafu itself you should use a material without lightmaps.\n");
                // It works in the ModelViewer because the ModelViewer is kind enough to provide a default lightmap...
            }

            Materials.PushBack(Material);
            RenderMaterials.PushBack(MatSys::Renderer!=NULL ? MatSys::Renderer->RegisterMaterial(Material) : NULL);
        }
        else
        {
            Materials.PushBack(NULL);
            RenderMaterials.PushBack(NULL);
        }
    }


    // Pre-store the triangle-mesh for each GeomObject.
    // This will not longer work once Multi/Sub-Objects are supported!
    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        GeomObjectT& GO=GeomObjects[GONr];

        GO.TriangleMesh.Type   =MatSys::MeshT::Triangles;
        GO.TriangleMesh.Winding=MatSys::MeshT::CCW;
        GO.TriangleMesh.Vertices.PushBackEmpty(GO.Triangles.Size()*3);

        for (unsigned long TriangleNr=0; TriangleNr<GO.Triangles.Size(); TriangleNr++)
        {
            const GeomObjectT::TriangleT& Tri=GO.Triangles[TriangleNr];

            for (unsigned long VNr=0; VNr<3; VNr++)
            {
                MatSys::MeshT::VertexT& V=GO.TriangleMesh.Vertices[3*TriangleNr+VNr];

                const VectorT& A  =GO.Vertices [Tri.IndVertices [VNr]];
                const VectorT& tcA=GO.TexCoords[Tri.IndTexCoords[VNr]];

                V.SetOrigin(A.AsVectorOfFloat());
                V.SetTextureCoord(float(tcA.x), float(tcA.y));
                V.SetNormal(Tri.Normals[VNr].AsVectorOfFloat());
                V.SetTangent(Tri.Tangents[VNr].AsVectorOfFloat());
                V.SetBiNormal(Tri.BiNormals[VNr].AsVectorOfFloat());
            }
        }
    }


    // Compute the bounding box of each GeomObject.
    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        GeomObjectT& GO=GeomObjects[GONr];

        GO.BB=BoundingBox3T<double>(GO.Vertices);
    }


    // Fill-in GOIndicesSortedByMat, the list of GeomObject-IDs, sorted by IndexMaterial.
    if (GeomObjects.Size()<=0xFFFF && MaterialNames.Size()<=0xFFFF)
    {
        // Initialize with the GONr in the upper WORD, and the respective material index in the lower WORD.
        for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
            GOIndicesSortedByMat.PushBack((GONr << 16) + GeomObjects[GONr].IndexMaterial);

        GOIndicesSortedByMat.QuickSort(CompareGOIDs);

        // Clear the contents - just leave the GONr in the DWORD.
        for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
            GOIndicesSortedByMat[GONr]=GOIndicesSortedByMat[GONr] >> 16;
    }
    else
    {
        printf("WARNING: Could not sort GeomObjects by their material index.\n");

        for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
            GOIndicesSortedByMat.PushBack(GONr);
    }


    // See if we have at least one GUI triangle, after all.
    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        const GeomObjectT& GO=GeomObjects[GONr];

        if (Materials[GO.IndexMaterial]!=NULL && Materials[GO.IndexMaterial]->Name=="Textures/meta/EntityGUI" && GO.Triangles.Size()>0)
        {
            const GeomObjectT::TriangleT& Tri0=GO.Triangles[0];

            // As directional vectors (Richtungsvektoren), just take the first tangent and the first binormal of the first triangle
            // -- they should be identical across all triangles and their vertices anyway.
            // ASSUMPTION: All tangents and binormals are *unit* vectors!
            const VectorT AxisX=Tri0.Tangents [0]; assert(length(AxisX)>0.5);
            const VectorT AxisY=Tri0.BiNormals[0]; assert(length(AxisY)>0.5);

            // As an initial, temporary origin vector (Stützvektor), just pick the first vertex of the first Triangle.
            const VectorT Origin=GO.Vertices[Tri0.IndVertices[0]];

            BoundingBox3T<double> BB;

            // In order to find the proper dimensions of the GUI panel, project all vertices (of all triangles, i.e. the entire GO)
            // into the new (temporary) plane, and build the 2D bounding rectangle of the projection.
            for (unsigned long VertexNr=0; VertexNr<GO.Vertices.Size(); VertexNr++)
            {
                const VectorT Proj=VectorT(dot(GO.Vertices[VertexNr]-Origin, AxisX),
                                           dot(GO.Vertices[VertexNr]-Origin, AxisY),
                                           0.0);

                if (VertexNr==0) BB=BoundingBox3T<double>(Proj);
                            else BB.Insert(Proj);
            }

            // Now transform the BB.Min back into model space in order to find the *proper* origin vector (Stützvektor).
            const VectorT Gui_ul=Origin+AxisX*BB.Min.x+AxisY*BB.Min.y;  // The upper left  of the GUI panel in object space.
            const VectorT Gui_lr=Origin+AxisX*BB.Max.x+AxisY*BB.Max.y;  // The lower right of the GUI panel in object space.

            Gui_Origin=Gui_ul.AsVectorOfFloat();

            // Project the "screen diagonal" (Gui_lr-Gui_ul) onto the AxisX and AxisY to find the proper lengths of the directional vectors.
            Gui_AxisX=scale(AxisX, dot(AxisX, Gui_lr-Gui_ul)).AsVectorOfFloat();
            Gui_AxisY=scale(AxisY, dot(AxisY, Gui_lr-Gui_ul)).AsVectorOfFloat();

            Gui_GeomObjNr=GONr;
            break;
        }
    }
}


ModelAseT::~ModelAseT()
{
    if (MatSys::Renderer==NULL) return;

    for (unsigned long RMNr=0; RMNr<RenderMaterials.Size(); RMNr++)
        MatSys::Renderer->FreeMaterial(RenderMaterials[RMNr]);
}


/***********************************************/
/*** Implementation of the ModelT interface. ***/
/***********************************************/

const std::string& ModelAseT::GetFileName() const
{
    return FileName;
}


void ModelAseT::Draw(int /*SequenceNr*/, float /*FrameNr*/, float /*LodDist*/, const ModelT* /*SubModel*/) const
{
    const float*                LightPos_=MatSys::Renderer->GetCurrentLightSourcePosition();
    const VectorT               LightPos(LightPos_[0], LightPos_[1], LightPos_[2]);
    const double                LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();
    const VectorT               LightBox(LightRadius, LightRadius, LightRadius);
    const BoundingBox3T<double> LightBB(LightPos+LightBox, LightPos-LightBox);


    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
        {
            for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
            {
                const GeomObjectT& GO=GeomObjects[GOIndicesSortedByMat[GONr]];

                MatSys::Renderer->SetCurrentMaterial(RenderMaterials[GO.IndexMaterial]);
                MatSys::Renderer->RenderMesh(GO.TriangleMesh);
            }
            break;
        }

        case MatSys::RendererI::LIGHTING:
        {
            for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
            {
                const GeomObjectT& GO=GeomObjects[GOIndicesSortedByMat[GONr]];

                if (!LightBB.Intersects(GO.BB)) continue;

                MatSys::Renderer->SetCurrentMaterial(RenderMaterials[GO.IndexMaterial]);
                MatSys::Renderer->RenderMesh(GO.TriangleMesh);
            }
            break;
        }

        case MatSys::RendererI::STENCILSHADOW:
        {
            for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
            {
                const GeomObjectT& GO=GeomObjects[GONr];

                if (!LightBB.Intersects(GO.BB)) continue;
                if (Materials[GO.IndexMaterial]==NULL || Materials[GO.IndexMaterial]->NoShadows) continue;

                static ArrayT<bool> TriangleIsFrontFacing;
                if (TriangleIsFrontFacing.Size()<GO.Triangles.Size()) TriangleIsFrontFacing.PushBackEmpty(GO.Triangles.Size()-TriangleIsFrontFacing.Size());

                for (unsigned long TriNr=0; TriNr<GO.Triangles.Size(); TriNr++)
                {
                    const GeomObjectT::TriangleT& Tri      =GO.Triangles[TriNr];
                    const double                  LightDist=dot(LightPos-GO.Vertices[Tri.IndVertices[0]], Tri.Normal);

                    TriangleIsFrontFacing[TriNr]=LightDist<0;   // Front-facing triangles are ordered CCW (contrary to Cafu)!
                }


                // Note that we have to cull the following polygons wrt. the *VIEWER* (not the light source)!
                static MatSys::MeshT MeshSilhouette(MatSys::MeshT::Quads);
                MeshSilhouette.Vertices.Overwrite();

                for (unsigned long TriNr=0; TriNr<GO.Triangles.Size(); TriNr++)
                {
                    if (!TriangleIsFrontFacing[TriNr]) continue;

                    // This triangle is front-facing wrt. the light source.
                    const GeomObjectT::TriangleT& Tri       =GO.Triangles[TriNr];
                    const unsigned long           VertInd[3]={ Tri.IndVertices[0], Tri.IndVertices[1], Tri.IndVertices[2] };

                    for (unsigned long EdgeNr=0; EdgeNr<3; EdgeNr++)
                    {
                        const bool IsSilhouetteEdge=(Tri.Neighbours[EdgeNr]<0) ? true : !TriangleIsFrontFacing[Tri.Neighbours[EdgeNr]];

                        if (IsSilhouetteEdge)
                        {
                            // The neighbour at edge 'EdgeNr' is back-facing (or non-existant), so we have found a possible silhouette edge.
                            const unsigned long v1=EdgeNr;
                            const unsigned long v2=(EdgeNr+1) % 3;
                            const VectorT       LA=GO.Vertices[VertInd[v1]]-LightPos;
                            const VectorT       LB=GO.Vertices[VertInd[v2]]-LightPos;

                            MeshSilhouette.Vertices.PushBackEmpty(4);

                            const unsigned long MeshSize=MeshSilhouette.Vertices.Size();

                            MeshSilhouette.Vertices[MeshSize-4].SetOrigin(GO.Vertices[VertInd[v2]].x, GO.Vertices[VertInd[v2]].y, GO.Vertices[VertInd[v2]].z);
                            MeshSilhouette.Vertices[MeshSize-3].SetOrigin(GO.Vertices[VertInd[v1]].x, GO.Vertices[VertInd[v1]].y, GO.Vertices[VertInd[v1]].z);
                            MeshSilhouette.Vertices[MeshSize-2].SetOrigin(LA.x, LA.y, LA.z, 0.0);
                            MeshSilhouette.Vertices[MeshSize-1].SetOrigin(LB.x, LB.y, LB.z, 0.0);
                        }
                    }
                }

                MatSys::Renderer->RenderMesh(MeshSilhouette);


                static MatSys::MeshT MeshCaps(MatSys::MeshT::Triangles);
                MeshCaps.Vertices.Overwrite();

                for (unsigned long TriNr=0; TriNr<GO.Triangles.Size(); TriNr++)
                {
                    if (!TriangleIsFrontFacing[TriNr]) continue;

                    // This triangle is front-facing wrt. the light source.
                    const GeomObjectT::TriangleT& Tri=GO.Triangles[TriNr];

                    MeshCaps.Vertices.PushBackEmpty(6);

                    const unsigned long MeshSize=MeshCaps.Vertices.Size();

                    // Render the occluder (front-facing wrt. the light source).
                    const VectorT& A=GO.Vertices[Tri.IndVertices[0]];
                    const VectorT& B=GO.Vertices[Tri.IndVertices[1]];
                    const VectorT& C=GO.Vertices[Tri.IndVertices[2]];

                    MeshCaps.Vertices[MeshSize-6].SetOrigin(A.x, A.y, A.z);
                    MeshCaps.Vertices[MeshSize-5].SetOrigin(B.x, B.y, B.z);
                    MeshCaps.Vertices[MeshSize-4].SetOrigin(C.x, C.y, C.z);

                    // Render the occluder (back-facing wrt. the light source).
                    const VectorT LA=A-LightPos;
                    const VectorT LB=B-LightPos;
                    const VectorT LC=C-LightPos;

                    MeshCaps.Vertices[MeshSize-3].SetOrigin(LC.x, LC.y, LC.z, 0.0);
                    MeshCaps.Vertices[MeshSize-2].SetOrigin(LB.x, LB.y, LB.z, 0.0);
                    MeshCaps.Vertices[MeshSize-1].SetOrigin(LA.x, LA.y, LA.z, 0.0);
                }

                MatSys::Renderer->RenderMesh(MeshCaps);
            }
            break;
        }
    }
}


bool ModelAseT::GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    if (Gui_GeomObjNr==-1) return false;

#if 0
    // Old, too simple code, just used for testing.
    const GeomObjectT&            GO =GeomObjects[GuiGeomObjNr];
    const GeomObjectT::TriangleT& Tri=GO.Triangles[0];

    GuiOrigin=GO.Vertices[Tri.IndVertices[0]].AsVectorOfFloat();
    GuiAxisX =GO.Vertices[Tri.IndVertices[1]].AsVectorOfFloat()-GuiOrigin;
    GuiAxisY =GO.Vertices[Tri.IndVertices[2]].AsVectorOfFloat()-GuiOrigin;
#else
    GuiOrigin=Gui_Origin;
    GuiAxisX =Gui_AxisX;
    GuiAxisY =Gui_AxisY;
#endif

    return true;
}


void ModelAseT::Print() const
{
    printf("\nThis is an ase model. FileName: \"%s\"\n", FileName.c_str());
    printf("Materials:\n");
    for (unsigned long MaterialNr=0; MaterialNr<MaterialNames.Size(); MaterialNr++)
        printf("    %2lu %s\n", MaterialNr, MaterialNames[MaterialNr].c_str());
    printf("\n");

    printf("GeomObjects:\n");
    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        const GeomObjectT& GO=GeomObjects[GONr];

        printf("    %2lu, using Mat %lu\n", GONr, GO.IndexMaterial);
    }
    printf("\n");

    printf("GeomObject indices sorted by material index:\n");
    for (unsigned long GONr=0; GONr<GOIndicesSortedByMat.Size(); GONr++)
        printf("    GO index: %2lu    MatIndex: %2lu\n", GOIndicesSortedByMat[GONr], GeomObjects[GOIndicesSortedByMat[GONr]].IndexMaterial);
    printf("\n");
}


int ModelAseT::GetNrOfSequences() const
{
    return 1;
}


const float* ModelAseT::GetSequenceBB(int /*SequenceNr*/, float /*FrameNr*/) const
{
    static float BB[6];

    BoundingBox3T<double> TotalBB(GeomObjects[0].BB);

    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        TotalBB.Insert(GeomObjects[GONr].BB.Min);
        TotalBB.Insert(GeomObjects[GONr].BB.Max);
    }

    BB[0]=float(TotalBB.Min.x);
    BB[1]=float(TotalBB.Min.y);
    BB[2]=float(TotalBB.Min.z);

    BB[3]=float(TotalBB.Max.x);
    BB[4]=float(TotalBB.Max.y);
    BB[5]=float(TotalBB.Max.z);

    return BB;
}


// float ModelAseT::GetNrOfFrames(int /*SequenceNr*/) const
// {
//     return 0.0;
// }


float ModelAseT::AdvanceFrameNr(int /*SequenceNr*/, float /*FrameNr*/, float /*DeltaTime*/, bool /*Loop*/) const
{
    return 0.0;
}
