/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_ase.hpp"
#include "MaterialSystem/Material.hpp"
#include "Math3D/BoundingBox.hpp"
#include "TextParser/TextParser.hpp"

#include <assert.h>
#include <stdio.h>


void LoaderAseT::ReadMaterials(TextParserT& TP)
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
                std::string MatName=TP.GetNextToken();

                // Make sure that it is unique.
                while (m_MaterialNames.Find(MatName)>=0) MatName+=std::string("/")+char('a'+(MaterialNr % 26));

                m_MaterialNames.PushBack(MatName);      // Store the material name.
                TP.SkipBlock("{", "}", true);           // Skip the rest of this *MATERIAL block.
                break;                                  // Proceed with the next material.
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
                m_MaterialNames.PushBack("NoMaterial"); // Store a dummy name.
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


void LoaderAseT::GeomObjectT::ReadMesh(TextParserT& TP)
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

                    const unsigned long SG=strtoul(SmoothGroupString.c_str(), NULL, 0);

                    if (SG > 31)
                        printf("Mesh is in smoothing group %lu, but should be in 0...31.\n", SG);

                    Triangles[FaceNr].SmoothGroups.PushBack(SG);
                    Triangles[FaceNr].SmoothGrps=uint32_t(1) << SG;

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


void LoaderAseT::ReadGeometry(TextParserT& TP)
{
    // Last seen token was "*GEOMOBJECT".
    TP.AssertAndSkipToken("{");

    m_GeomObjects.PushBackEmpty();
    GeomObjectT& GO=m_GeomObjects[m_GeomObjects.Size()-1];

    GO.Name="Object";
    GO.IndexMaterial=0;
    GO.CastShadows=false;

    while (true)
    {
        const std::string Token=TP.GetNextToken();

             if (Token=="*NODE_NAME"      ) GO.Name=TP.GetNextToken();
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


LoaderAseT::LoaderAseT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags)
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

        if (m_MaterialNames.Size()==0) throw TextParserT::ParseError();
        if (m_GeomObjects  .Size()==0) throw TextParserT::ParseError();
    }
    catch (const TextParserT::ParseError&)
    {
        throw LoadErrorT("Could not parse the file.");
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

    for (unsigned long GONr=0; GONr<m_GeomObjects.Size(); GONr++)
        for (unsigned long TriangleNr=0; TriangleNr<m_GeomObjects[GONr].Triangles.Size(); TriangleNr++)
        {
            const GeomObjectT::TriangleT& Tri=m_GeomObjects[GONr].Triangles[TriangleNr];

            for (unsigned long SGNr=0; SGNr<Tri.SmoothGroups.Size(); SGNr++)
                if (Tri.SmoothGroups[SGNr]>=NextSmoothGroupID)
                    NextSmoothGroupID=Tri.SmoothGroups[SGNr]+1;
        }

    // Now assign each triangle that is in NO SmoothGroup an own, UNIQUE SmoothGroup.
    // This avoids special-case treatment below, and is intentionally done ACROSS GeomObjects (rather than per GeomObject).
    for (unsigned long GONr=0; GONr<m_GeomObjects.Size(); GONr++)
        for (unsigned long TriangleNr=0; TriangleNr<m_GeomObjects[GONr].Triangles.Size(); TriangleNr++)
        {
            GeomObjectT::TriangleT& Tri=m_GeomObjects[GONr].Triangles[TriangleNr];

            if (Tri.SmoothGroups.Size()==0) Tri.SmoothGroups.PushBack(NextSmoothGroupID++);
        }


    // Compute the tangent space vectors.
    for (unsigned long GONr=0; GONr<m_GeomObjects.Size(); GONr++)
    {
        GeomObjectT& GO=m_GeomObjects[GONr];

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
    }


    // Check if the material indices are all within the allowed range.
    for (unsigned long GONr=0; GONr<m_GeomObjects.Size(); GONr++)
    {
        // Ase files can come with too few materials specified.
        // Make sure that there are enough of them, or else we'll crash with array-index-out-of-bounds later.
        if (m_GeomObjects[GONr].IndexMaterial>=m_MaterialNames.Size())
        {
            printf("ase model error (%s): GeomObject %lu refers to material index %lu, but there are only %lu materials total.\n", FileName.c_str(), GONr, m_GeomObjects[GONr].IndexMaterial, m_MaterialNames.Size());
            throw LoadErrorT("A GeomObject refers to an unknown material.");
        }
    }
}


void LoaderAseT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    // Create a default "identity" joint.
    // That single joint is used for (shared by) all weights of all meshes.
    Joints.PushBackEmpty();

    Joints[0].Name  ="root";
    Joints[0].Parent=-1;
 // Joints[0].Pos   =Vector3fT();
 // Joints[0].Qtr   =Vector3fT();   // Identity quaternion...
    Joints[0].Scale =Vector3fT(1.0f, 1.0f, 1.0f);


    for (unsigned long GONr=0; GONr<m_GeomObjects.Size(); GONr++)
    {
        Meshes.PushBackEmpty();

        const GeomObjectT& GO  =m_GeomObjects[GONr];
        CafuModelT::MeshT& Mesh=Meshes[GONr];

        Mesh.Name=GO.Name;

        // Set the default tangent-space method for ASE models to HARD.
        // This helps with many older and/or simpler models that are not immediately edited in the Model Editor.
        Mesh.TSMethod=CafuModelT::MeshT::HARD;

        for (unsigned long TriNr=0; TriNr<GO.Triangles.Size(); TriNr++)
        {
            Mesh.Triangles.PushBackEmpty();

            const GeomObjectT::TriangleT& AseTri=GO.Triangles[TriNr];
            CafuModelT::MeshT::TriangleT& CafuTri=Mesh.Triangles[TriNr];

            for (unsigned long i=0; i<3; i++)
            {
                const Vector3fT AseVertexPos=GO.Vertices[AseTri.IndVertices[i]].AsVectorOfFloat();
                unsigned long   CafuVertexNr=0;

                // Try to find a suitable vertex in Mesh.Vertices[].
                for (CafuVertexNr=0; CafuVertexNr<Mesh.Vertices.Size(); CafuVertexNr++)
                {
                    const CafuModelT::MeshT::VertexT& CafuVertex=Mesh.Vertices[CafuVertexNr];

                    assert(CafuVertex.NumWeights==1);

                    if (Mesh.Weights[CafuVertex.FirstWeightIdx].Pos!=AseVertexPos) continue;
                    if (CafuVertex.u!=GO.TexCoords[AseTri.IndTexCoords[i]].AsVectorOfFloat().x) continue;
                    if (CafuVertex.v!=GO.TexCoords[AseTri.IndTexCoords[i]].AsVectorOfFloat().y) continue;

                    // This vertex meets all criteria - take it.
                    break;
                }

                // If no suitable vertex was found in Mesh.Vertices[], insert a new one.
                if (CafuVertexNr>=Mesh.Vertices.Size())
                {
                    unsigned long WeightNr=0;

                    for (WeightNr=0; WeightNr<Mesh.Weights.Size(); WeightNr++)
                        if (Mesh.Weights[WeightNr].Pos==AseVertexPos) break;

                    if (WeightNr>=Mesh.Weights.Size())
                    {
                        Mesh.Weights.PushBackEmpty();

                        Mesh.Weights[WeightNr].JointIdx=0;
                        Mesh.Weights[WeightNr].Weight  =1.0f;
                        Mesh.Weights[WeightNr].Pos     =AseVertexPos;
                    }

                    Mesh.Vertices.PushBackEmpty();
                    CafuModelT::MeshT::VertexT& CafuVertex=Mesh.Vertices[CafuVertexNr];

                    CafuVertex.u             =GO.TexCoords[AseTri.IndTexCoords[i]].AsVectorOfFloat().x;
                    CafuVertex.v             =GO.TexCoords[AseTri.IndTexCoords[i]].AsVectorOfFloat().y;
                    CafuVertex.FirstWeightIdx=WeightNr;
                    CafuVertex.NumWeights    =1;
                }

                // Triangles are ordered CW for Cafu models and CCW for ase models,
                // so we write [2-i] in order to record the vertices in the proper (reversed) order.
                CafuTri.VertexIdx[2-i]=CafuVertexNr;
            }

            CafuTri.SmoothGroups=AseTri.SmoothGrps;
        }


        Mesh.Material=MaterialMan.GetMaterial(m_MaterialNames[GO.IndexMaterial]);

        if (!Mesh.Material)
        {
            // As we haven't parsed the *MATERIAL definitions more deeply, we cannot reasonably reconstruct materials here.
            // Thus if there isn't an appropriately prepared .cmat file (so that MatName is found in MaterialMan),
            // go for the wire-frame substitute straight away.
            Mesh.Material=MaterialMan.RegisterMaterial(CreateDefaultMaterial(m_MaterialNames[GO.IndexMaterial]));
        }
    }
}


void LoaderAseT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
{
}


void LoaderAseT::Print() const
{
    printf("\nThis is an ase model. FileName: \"%s\"\n", m_FileName.c_str());
    printf("Materials:\n");
    for (unsigned long MaterialNr=0; MaterialNr<m_MaterialNames.Size(); MaterialNr++)
        printf("    %2lu %s\n", MaterialNr, m_MaterialNames[MaterialNr].c_str());
    printf("\n");

    printf("GeomObjects:\n");
    for (unsigned long GONr=0; GONr<m_GeomObjects.Size(); GONr++)
    {
        const GeomObjectT& GO=m_GeomObjects[GONr];

        printf("    %2lu, using Mat %lu\n", GONr, GO.IndexMaterial);
    }
    printf("\n");
}
