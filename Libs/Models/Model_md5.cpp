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

/***********************/
/*** Doom3 md5 Model ***/
/***********************/

#include "Model_md5.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/BoundingBox.hpp"
#include "TextParser/TextParser.hpp"
#include "String.hpp"

#include <cstdio>
#include <iostream>

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


// Skips a { ... } block in TP. The block can contain nested blocks.
// It can (and must) be stated if the caller has already read the opening brace.
static void SkipBlock(TextParserT& TP, bool CallerAlreadyReadOpeningBrace)
{
    if (!CallerAlreadyReadOpeningBrace) TP.GetNextToken();

    unsigned long NestedLevel=1;

    while (true)
    {
        const std::string Token=TP.GetNextToken();

        if (Token=="{") NestedLevel++;

        if (Token=="}")
        {
            NestedLevel--;
            if (NestedLevel==0) break;
        }
    }
}


bool ModelMd5T::MeshT::AreGeoDups(int Vertex1Nr, int Vertex2Nr) const
{
    // A vertex is a geodup of itself!
    // (This explicit check is somewhat redundant, but I leave it in for clarity.)
    if (Vertex1Nr==Vertex2Nr) return true;

    // If the number of weights differs, the vertices cannot be geodups.
    if (Vertices[Vertex1Nr].NumWeights!=Vertices[Vertex2Nr].NumWeights) return false;

    // The number of weights matches. If the FirstWeightIdx is identical, too, the vertices trivially are a geodup.
    if (Vertices[Vertex1Nr].FirstWeightIdx==Vertices[Vertex2Nr].FirstWeightIdx) return true;

    // The number of weights matches, but the FirstWeightIdx does not.
    // Now compare all the weights manually. If all their contents match, they are geodups.
    for (int WeightNr=0; WeightNr<Vertices[Vertex1Nr].NumWeights; WeightNr++)
    {
        const WeightT& Weight1=Weights[Vertices[Vertex1Nr].FirstWeightIdx+WeightNr];
        const WeightT& Weight2=Weights[Vertices[Vertex2Nr].FirstWeightIdx+WeightNr];

        if (Weight1.JointIdx!=Weight2.JointIdx) return false;
        if (Weight1.Weight  !=Weight2.Weight  ) return false;   // Bitwise float compare...
        if (Weight1.Pos     !=Weight2.Pos     ) return false;   // Bitwise float compare...
    }

    // All weights were equal - the vertices are geodups of each other!
    return true;
}


ModelMd5T::ModelMd5T(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : m_FileName(FileName),
      m_UseGivenTangentSpace(false),            // The tangent space is not given by the model data from file. Recompute it from spatial and texture coordinates.
      m_Draw_CachedDataAtSequNr(-1234),         // Just a random number that is unlikely to occur normally.
      m_Draw_CachedDataAtFrameNr(-3.1415926f)   // Just a random number that is unlikely to occur normally.
{
    ArrayT<std::string> ComponentFiles;

    if (cf::String::EndsWith(FileName, "md5"))
    {
        TextParserT TP(FileName.c_str());

        while (!TP.IsAtEOF()) ComponentFiles.PushBack(TP.GetNextToken());
        if (ComponentFiles.Size()<1) throw ModelT::LoadError();

        int PrefixLength=FileName.length()-1;
        while (PrefixLength>=0 && FileName[PrefixLength]!='/' && FileName[PrefixLength]!='\\') PrefixLength--;
        const std::string Prefix=std::string(FileName, 0, PrefixLength+1);

        for (unsigned long FileNr=0; FileNr<ComponentFiles.Size(); FileNr++)
        {
            // The ComponentFiles are specified relative to the parent file name,
            // thus extract the path portion from FileName and prepend it to the ComponentFiles.
            // If however the second character of a ComponentFiles entry is a ":", treat this as
            // a drive letter specification and thus as the special case of an absolute path.
            if (ComponentFiles[FileNr].length()>=2 && ComponentFiles[FileNr][1]==':') continue;

            ComponentFiles[FileNr]=Prefix+ComponentFiles[FileNr];
        }
    }
    else if (cf::String::EndsWith(FileName, "md5mesh"))
    {
        ComponentFiles.PushBack(FileName);
    }


    try
    {
        // Read the base mesh.
        TextParserT TP(ComponentFiles[0].c_str(), "{()}");

        // A good description of the md5 file format can be found at
        // http://www.doom3world.org/phpbb2/viewtopic.php?t=2884 (initial messages also printed out in my TechArchive),
        // and at http://wiki.doom3reference.com/wiki/Models_(folder).
        while (!TP.IsAtEOF())
        {
            const std::string Token=TP.GetNextToken();

                 if (Token=="MD5Version" ) { if (TP.GetNextToken()!="10") throw ModelT::LoadError(); }
            else if (Token=="commandline") TP.GetNextToken();       // Ignore the command line.
            else if (Token=="numJoints"  ) TP.GetNextToken();       // Ignore the given number of joints - we just load as many as we find.
            else if (Token=="numMeshes"  ) TP.GetNextToken();       // Ignore the given number of meshes - we just load as many as we find.
            else if (Token=="joints")
            {
                TP.AssertAndSkipToken("{");

                while (true)
                {
                    JointT Joint;

                    Joint.Name  =TP.GetNextToken(); if (Joint.Name=="}") break;
                    Joint.Parent=TP.GetNextTokenAsInt();
                    TP.AssertAndSkipToken("(");
                    Joint.Pos.x=TP.GetNextTokenAsFloat();
                    Joint.Pos.y=TP.GetNextTokenAsFloat();
                    Joint.Pos.z=TP.GetNextTokenAsFloat();
                    TP.AssertAndSkipToken(")");
                    TP.AssertAndSkipToken("(");
                    Joint.Qtr.x=TP.GetNextTokenAsFloat();
                    Joint.Qtr.y=TP.GetNextTokenAsFloat();
                    Joint.Qtr.z=TP.GetNextTokenAsFloat();
                    TP.AssertAndSkipToken(")");

                    // Make sure that all joints (bones) are declared in a proper hierarchical order.
                    // That is, if we traverse the m_Joints in increasing order 0, 1, 2, ..., then we are never faced with a
                    // parent that has not been seen earlier in the traversal sequence. (The -1 parent at index 0 is an exception.)
                    if (Joint.Parent>=int(m_Joints.Size()))
                    {
                        printf("WARNING: Bad bone order!  %lu bones read so far, and the next (name \"%s\") is referring to parent %i.\n", m_Joints.Size(), Joint.Name.c_str(), Joint.Parent);
                        throw ModelT::LoadError();  // TODO: Fix this by re-sorting the joints rather than by abortion!
                    }

                    m_Joints.PushBack(Joint);
                }
            }
            else if (Token=="mesh")
            {
                m_Meshes.PushBackEmpty();
                MeshT& Mesh=m_Meshes[m_Meshes.Size()-1];

                TP.AssertAndSkipToken("{");

                while (true)
                {
                    const std::string Token=TP.GetNextToken();

                         if (Token=="}"         ) break;
                    else if (Token=="numtris"   ) { if (Mesh.Triangles.Size()>0) throw ModelT::LoadError(); Mesh.Triangles.PushBackEmpty(TP.GetNextTokenAsInt()); }
                    else if (Token=="numverts"  ) { if (Mesh.Vertices .Size()>0) throw ModelT::LoadError(); Mesh.Vertices .PushBackEmpty(TP.GetNextTokenAsInt()); }
                    else if (Token=="numweights") { if (Mesh.Weights  .Size()>0) throw ModelT::LoadError(); Mesh.Weights  .PushBackEmpty(TP.GetNextTokenAsInt()); }
                    else if (Token=="shader")
                    {
                        Mesh.Material      =GetMaterialByName(TP.GetNextToken());
                        Mesh.RenderMaterial=MatSys::Renderer!=NULL ? MatSys::Renderer->RegisterMaterial(Mesh.Material) : NULL;
                    }
                    else if (Token=="tri")
                    {
                        const unsigned long TriIdx=TP.GetNextTokenAsInt();

                        if (TriIdx>=Mesh.Triangles.Size()) throw ModelT::LoadError();

                        Mesh.Triangles[TriIdx].VertexIdx[0]=TP.GetNextTokenAsInt();
                        Mesh.Triangles[TriIdx].VertexIdx[1]=TP.GetNextTokenAsInt();
                        Mesh.Triangles[TriIdx].VertexIdx[2]=TP.GetNextTokenAsInt();
                    }
                    else if (Token=="vert")
                    {
                        const unsigned long VertIdx=TP.GetNextTokenAsInt();

                        if (VertIdx>=Mesh.Vertices.Size()) throw ModelT::LoadError();

                        TP.AssertAndSkipToken("(");
                        Mesh.Vertices[VertIdx].u=TP.GetNextTokenAsFloat();
                        Mesh.Vertices[VertIdx].v=TP.GetNextTokenAsFloat();
                        TP.AssertAndSkipToken(")");
                        Mesh.Vertices[VertIdx].FirstWeightIdx=TP.GetNextTokenAsInt();
                        Mesh.Vertices[VertIdx].NumWeights    =TP.GetNextTokenAsInt();
                    }
                    else if (Token=="weight")
                    {
                        const unsigned long WeightIdx=TP.GetNextTokenAsInt();

                        if (WeightIdx>=Mesh.Weights.Size()) throw ModelT::LoadError();

                        Mesh.Weights[WeightIdx].JointIdx=TP.GetNextTokenAsInt();
                        Mesh.Weights[WeightIdx].Weight  =TP.GetNextTokenAsFloat();
                        TP.AssertAndSkipToken("(");
                        Mesh.Weights[WeightIdx].Pos.x=TP.GetNextTokenAsFloat();
                        Mesh.Weights[WeightIdx].Pos.y=TP.GetNextTokenAsFloat();
                        Mesh.Weights[WeightIdx].Pos.z=TP.GetNextTokenAsFloat();
                        TP.AssertAndSkipToken(")");
                    }
                    else printf("Unknown token \"%s\", skipping...\n", Token.c_str());
                }
            }
            else
            {
                // Unknown token!
                // If the next token is a block start, skip the block.
                // Otherwise it was something else that we just throw away.
                printf("Unknown token \"%s\", skipping...\n", Token.c_str());
                if (TP.GetNextToken()=="{") SkipBlock(TP, true);
            }
        }

        if (m_Joints.Size()==0) throw ModelT::LoadError();
        if (m_Meshes.Size()==0) throw ModelT::LoadError();
    }
    catch (const TextParserT::ParseError&)
    {
        throw ModelT::LoadError();
    }


    InitMeshes();


    // Read the individual animation sequence files.
    for (unsigned long FileNr=1; FileNr<ComponentFiles.Size(); FileNr++)
    {
        TextParserT TP(ComponentFiles[FileNr].c_str());

        try
        {
            AnimT Anim;

            Anim.FPS=24.0f;

            while (!TP.IsAtEOF())
            {
                const std::string Token=TP.GetNextToken();

                     if (Token=="MD5Version" ) { if (TP.GetNextToken()!="10") throw ModelT::LoadError(); }
                else if (Token=="commandline") TP.GetNextToken();       // Ignore the command line.
                else if (Token=="frameRate"  ) Anim.FPS=TP.GetNextTokenAsFloat();
                else if (Token=="numFrames"  )
                {
                    // Be stricter with numFrames than with some num* variables in the md5mesh file.
                    // This is required because for example there must be as many bounding boxes as frames.
                    if (Anim.Frames.Size()>0) throw ModelT::LoadError();

                    Anim.Frames.PushBackEmpty(TP.GetNextTokenAsInt());
                }
                else if (Token=="numJoints")
                {
                    // The numJoints here MUST match the numJoints in the md5mesh file!
                    const unsigned long numJoints=TP.GetNextTokenAsInt();

                    if (numJoints!=m_Joints.Size()) { printf("%lu joints in md5anim file, %lu joints in md5mesh.\n", numJoints, m_Joints.Size()); throw ModelT::LoadError(); }
                    if (Anim.AnimJoints.Size()>0) { printf("Anim.AnimJoints.Size()==%lu\n", Anim.AnimJoints.Size()); throw ModelT::LoadError(); }

                    Anim.AnimJoints.PushBackEmpty(numJoints);
                }
                else if (Token=="numAnimatedComponents")
                {
                    const unsigned long numAnimatedComponents=TP.GetNextTokenAsInt();

                    // This is the number of components that is animated in the frames (and thus so many values are stored with each frame).
                    // Therefore, the number of frames must have been specified already, so we can allocate all the memory here.
                    if (Anim.Frames.Size()==0) throw ModelT::LoadError();

                    for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                        Anim.Frames[FrameNr].AnimData.PushBackEmpty(numAnimatedComponents);
                }
                else if (Token=="hierarchy")
                {
                    TP.AssertAndSkipToken("{");

                    for (unsigned long JointNr=0; JointNr<Anim.AnimJoints.Size(); JointNr++)
                    {
                        // Make sure that the name and parent are identical with the joint from the md5mesh file.
                        if (m_Joints[JointNr].Name  !=TP.GetNextToken()) throw ModelT::LoadError();
                        if (m_Joints[JointNr].Parent!=TP.GetNextTokenAsInt()) throw ModelT::LoadError();

                        Anim.AnimJoints[JointNr].Flags       =TP.GetNextTokenAsInt();
                        Anim.AnimJoints[JointNr].FirstDataIdx=TP.GetNextTokenAsInt();
                    }

                    TP.AssertAndSkipToken("}");
                }
                else if (Token=="bounds")
                {
                    TP.AssertAndSkipToken("{");

                    for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                    {
                        TP.AssertAndSkipToken("(");
                        Anim.Frames[FrameNr].BB[0]=TP.GetNextTokenAsFloat();
                        Anim.Frames[FrameNr].BB[1]=TP.GetNextTokenAsFloat();
                        Anim.Frames[FrameNr].BB[2]=TP.GetNextTokenAsFloat();
                        TP.AssertAndSkipToken(")");
                        TP.AssertAndSkipToken("(");
                        Anim.Frames[FrameNr].BB[3]=TP.GetNextTokenAsFloat();
                        Anim.Frames[FrameNr].BB[4]=TP.GetNextTokenAsFloat();
                        Anim.Frames[FrameNr].BB[5]=TP.GetNextTokenAsFloat();
                        TP.AssertAndSkipToken(")");
                    }

                    TP.AssertAndSkipToken("}");
                }
                else if (Token=="baseframe")
                {
                    TP.AssertAndSkipToken("{");

                    for (unsigned long JointNr=0; JointNr<Anim.AnimJoints.Size(); JointNr++)
                    {
                        TP.AssertAndSkipToken("(");
                        Anim.AnimJoints[JointNr].BaseValues[0]=TP.GetNextTokenAsFloat();
                        Anim.AnimJoints[JointNr].BaseValues[1]=TP.GetNextTokenAsFloat();
                        Anim.AnimJoints[JointNr].BaseValues[2]=TP.GetNextTokenAsFloat();
                        TP.AssertAndSkipToken(")");
                        TP.AssertAndSkipToken("(");
                        Anim.AnimJoints[JointNr].BaseValues[3]=TP.GetNextTokenAsFloat();
                        Anim.AnimJoints[JointNr].BaseValues[4]=TP.GetNextTokenAsFloat();
                        Anim.AnimJoints[JointNr].BaseValues[5]=TP.GetNextTokenAsFloat();
                        TP.AssertAndSkipToken(")");
                    }

                    TP.AssertAndSkipToken("}");
                }
                else if (Token=="frame")
                {
                    const unsigned long FrameNr=TP.GetNextTokenAsInt();

                    TP.AssertAndSkipToken("{");

                    for (unsigned long ComponentNr=0; ComponentNr<Anim.Frames[FrameNr].AnimData.Size(); ComponentNr++)
                        Anim.Frames[FrameNr].AnimData[ComponentNr]=TP.GetNextTokenAsFloat();

                    TP.AssertAndSkipToken("}");
                }
                else
                {
                    // Unknown token!
                    // If the next token is a block start, skip the block.
                    // Otherwise it was something else that we just throw away.
                    printf("Unknown token \"%s\", skipping...\n", Token.c_str());
                    if (TP.GetNextToken()=="{") SkipBlock(TP, true);
                }
            }

            m_Anims.PushBack(Anim);
        }
        catch (const TextParserT::ParseError&)
        {
            // Loading this animation sequence failed, but as the base mesh (the md5mesh file)
            // loaded properly, that is not reason enough to abort loading the entire model.
            AnimT InvalidAnim;

            InvalidAnim.FPS=-1.0f;          // Use a negative FPS to flags this animation as invalid.
            m_Anims.PushBack(InvalidAnim);  // Note that InvalidAnim.Frames.Size()==0, too.

            printf("WARNING: Loading animation sequence file %s failed just before input byte %lu!\n", ComponentFiles[FileNr].c_str(), TP.GetReadPosByte());
        }
        catch (const ModelT::LoadError&)
        {
            // Loading this animation sequence failed, but as the base mesh (the md5mesh file)
            // loaded properly, that is not reason enough to abort loading the entire model.
            AnimT InvalidAnim;

            InvalidAnim.FPS=-1.0f;          // Use a negative FPS to flags this animation as invalid.
            m_Anims.PushBack(InvalidAnim);  // Note that InvalidAnim.Frames.Size()==0, too.

            printf("WARNING: Loading animation sequence file %s failed just before input byte %lu!\n", ComponentFiles[FileNr].c_str(), TP.GetReadPosByte());
        }
    }


    // Allocate the cache space that is needed for drawing.
    m_Draw_JointMatrices.PushBackEmpty(m_Joints.Size());
    m_Draw_Meshes.PushBackEmpty(m_Meshes.Size());

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        m_Draw_Meshes[MeshNr].Type   =MatSys::MeshT::Triangles;
     // m_Draw_Meshes[MeshNr].Winding=MatSys::MeshT::CW;    // CW is the default.
        m_Draw_Meshes[MeshNr].Vertices.PushBackEmpty(m_Meshes[MeshNr].Triangles.Size()*3);
    }
}


ModelMd5T::ModelMd5T(const std::string& FileName, bool UseGivenTangentSpace)
    : m_FileName(FileName),
      m_UseGivenTangentSpace(UseGivenTangentSpace), // Some (static, non-animated) model file formats may bring all their tangent space data with them, then there is no need to recompute it here. The if or if not depends on the details of the derived class(es).
      m_Draw_CachedDataAtSequNr(-1234),             // Just a random number that is unlikely to occur normally.
      m_Draw_CachedDataAtFrameNr(-3.1415926f)       // Just a random number that is unlikely to occur normally.
{
    // This is a constructor for use by the derived classes.
    // It intentionally does nothing but the most basic initialization.
}


MaterialT* ModelMd5T::GetMaterialByName(const std::string& MaterialName) const
{
    MaterialT* Material=MaterialManager->GetMaterial(MaterialName);

    if (Material==NULL)
    {
        static ConVarT ModelReplacementMaterial("modelReplaceMat", "meta/model_replacement", 0, "Replacement for unknown materials of models.", NULL);

        Console->Warning("Model \""+m_FileName+"\" refers to unknown material \""+MaterialName+"\". "
            "Replacing the material with \""+ModelReplacementMaterial.GetValueString()+"\".\n");

        Material=MaterialManager->GetMaterial(ModelReplacementMaterial.GetValueString());
    }

    if (Material==NULL)
    {
        Console->Warning("The replacement material is also unknown - the model will NOT render!\n");
    }

    if (Material!=NULL && !Material->LightMapComp.IsEmpty())
    {
        Console->Warning("Model \""+m_FileName+"\" uses material \""+MaterialName+"\", which in turn has lightmaps defined.\n"
            "It will work in the ModelViewer, but for other applications like Cafu itself you should use a material without lightmaps.\n");
            // It works in the ModelViewer because the ModelViewer is kind enough to provide a default lightmap...
    }

    return Material;
}


void ModelMd5T::InitMeshes()
{
    // Compute the bounding box for the model in the md5mesh file (stored in m_BaseBB), just in case this model has no animations.
    {
        ArrayT<MatrixT> JointMatrices;
        JointMatrices.PushBackEmpty(m_Joints.Size());

        for (unsigned long JointNr=0; JointNr<m_Joints.Size(); JointNr++)
        {
            const JointT& J=m_Joints[JointNr];
            const float   t=1.0f - J.Qtr.x*J.Qtr.x - J.Qtr.y*J.Qtr.y - J.Qtr.z*J.Qtr.z;
            const float   w=(t<0.0f) ? 0.0f : -sqrt(t);
            const float   Q[4]={ J.Qtr.x, J.Qtr.y, J.Qtr.z, w };

            JointMatrices[JointNr]=MatrixT
            (
                1.0f-2.0f*Q[1]*Q[1]-2.0f*Q[2]*Q[2],      2.0f*Q[0]*Q[1]-2.0f*Q[3]*Q[2],      2.0f*Q[0]*Q[2]+2.0f*Q[3]*Q[1], J.Pos.x,
                     2.0f*Q[0]*Q[1]+2.0f*Q[3]*Q[2], 1.0f-2.0f*Q[0]*Q[0]-2.0f*Q[2]*Q[2],      2.0f*Q[1]*Q[2]-2.0f*Q[3]*Q[0], J.Pos.y,
                     2.0f*Q[0]*Q[2]-2.0f*Q[3]*Q[1],      2.0f*Q[1]*Q[2]+2.0f*Q[3]*Q[0], 1.0f-2.0f*Q[0]*Q[0]-2.0f*Q[1]*Q[1], J.Pos.z,
                                              0.0f,                               0.0f,                               0.0f,    1.0f
            );
        }

        for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
        {
            const MeshT& Mesh=m_Meshes[MeshNr];

            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];
                Vector3fT             OutVert;

                if (Vertex.NumWeights==1)
                {
                    const MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx];

                    OutVert=JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos);
                }
                else
                {
                    for (int WeightNr=0; WeightNr<Vertex.NumWeights; WeightNr++)
                    {
                        const MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx+WeightNr];

                        OutVert+=JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos) * Weight.Weight;
                    }
                }

                if (MeshNr==0 && VertexNr==0)
                {
                    m_BaseBB[0]=OutVert.x;
                    m_BaseBB[1]=OutVert.y;
                    m_BaseBB[2]=OutVert.z;

                    m_BaseBB[3]=OutVert.x;
                    m_BaseBB[4]=OutVert.y;
                    m_BaseBB[5]=OutVert.z;
                }
                else
                {
                    if (OutVert.x<m_BaseBB[0]) m_BaseBB[0]=OutVert.x;
                    if (OutVert.y<m_BaseBB[1]) m_BaseBB[1]=OutVert.y;
                    if (OutVert.z<m_BaseBB[2]) m_BaseBB[2]=OutVert.z;

                    if (OutVert.x>m_BaseBB[3]) m_BaseBB[3]=OutVert.x;
                    if (OutVert.y>m_BaseBB[4]) m_BaseBB[4]=OutVert.y;
                    if (OutVert.z>m_BaseBB[5]) m_BaseBB[5]=OutVert.z;
                }
            }
        }
    }


    // Compute auxiliary data for each mesh.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];


        // ********************************************************************************
        //  Find the GeoDups of each vertex. See MeshT::AreGeoDups() for more information.
        // ********************************************************************************

        for (unsigned long Vertex1Nr=0; Vertex1Nr<Mesh.Vertices.Size(); Vertex1Nr++)
            for (unsigned long Vertex2Nr=0; Vertex2Nr<Mesh.Vertices.Size(); Vertex2Nr++)
                if (Vertex1Nr!=Vertex2Nr && Mesh.AreGeoDups(Vertex1Nr, Vertex2Nr))
                {
                    // Vertex 2 is a "pure" geometrical duplicate of vertex 1 (Vertex1Nr!=Vertex2Nr), so record its index at vertex 1.
                    // Note that the outer loops were written to guarantee that each GeoDups array contains indices in increasing order.
                    Mesh.Vertices[Vertex1Nr].GeoDups.PushBack(Vertex2Nr);
                }

#ifdef DEBUG
        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            // Assert that the index of Vertex does not occur in Vertex.GeoDups, i.e. that Vertex.GeoDups not contains self (the own vertex index).
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                assert(Vertex.GeoDups[DupNr]!=int(VertexNr));

            // Assert that the Vertex.GeoDups elements are indeed stored in increasing order.
            for (unsigned long DupNr=1; DupNr<Vertex.GeoDups.Size(); DupNr++)
                assert(Vertex.GeoDups[DupNr-1]<Vertex.GeoDups[DupNr]);
        }
#endif


        // ************************************************************************************
        //  For each triangle, determine its polarity. (Is the texture on it mirrored or not?)
        // ************************************************************************************

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            // This code is analogous to the code that determines the per-triangle tangent-space vectors.
            const MeshT::VertexT& V_0=Mesh.Vertices[Tri.VertexIdx[0]];
            const MeshT::VertexT& V_1=Mesh.Vertices[Tri.VertexIdx[1]];
            const MeshT::VertexT& V_2=Mesh.Vertices[Tri.VertexIdx[2]];

            const Vector3fT uv01=Vector3fT(V_1.u, V_1.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
            const Vector3fT uv02=Vector3fT(V_2.u, V_2.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);

            // This is analogous to cross(uv01, uv02).z > 0.0.
            Tri.Polarity=(uv01.x*uv02.y-uv01.y*uv02.x) > 0.0;
        }


        // *******************************************************************************
        //  Precompute the tables that store for each vertex which triangles refer to it.
        //  This helps with quickly determining the neighbours of a triangle below.
        // *******************************************************************************

        // For each vertex, this is the list of triangles that use (refer to) this vertex,
        // *without*   the trianges that refer to geometrically identical vertices (i.e. the vertices in GeoDups).
        ArrayT< ArrayT<int> > Vertices_RefTris;
        Vertices_RefTris.PushBackEmpty(Mesh.Vertices.Size());

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            Vertices_RefTris[Tri.VertexIdx[0]].PushBack(TriangleNr);
            Vertices_RefTris[Tri.VertexIdx[1]].PushBack(TriangleNr);
            Vertices_RefTris[Tri.VertexIdx[2]].PushBack(TriangleNr);
        }


        // For each vertex, this is the list of triangles that use (refer to) this vertex,
        // *inclusive* the trianges that refer to geometrically identical vertices (i.e. the vertices in GeoDups).
        // That means that vertices that are dups of each other have identical RefTrisInclDups arrays (barring their elements order!).
        ArrayT< ArrayT<int> > Vertices_RefTrisInclDups;
        Vertices_RefTrisInclDups.PushBackEmpty(Mesh.Vertices.Size());

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

         // Vertices_RefTrisInclDups[VertexNr]=Vertices_RefTris[VertexNr] + the RefTris of all duplicate vertices.
            Vertices_RefTrisInclDups[VertexNr]=Vertices_RefTris[VertexNr];

            // Concatenate the RefTris arrays of all geometrically identical vertices.
            // Note that the resulting Vertices_RefTrisInclDups[VertexNr] array will *not* contain any element more than once, no duplicate indices occur!
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                Vertices_RefTrisInclDups[VertexNr].PushBack(Vertices_RefTris[Vertex.GeoDups[DupNr]]);
        }

#ifdef DEBUG
        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            // Assert that the elements of Vertices_RefTrisInclDups[VertexNr] are all unique (no triangle is mentioned twice).
            for (unsigned long r1=0; r1+1<Vertices_RefTrisInclDups[VertexNr].Size(); r1++)
                for (unsigned long r2=r1+1; r2<Vertices_RefTrisInclDups[VertexNr].Size(); r2++)
                    assert(Vertices_RefTrisInclDups[VertexNr][r1]!=Vertices_RefTrisInclDups[VertexNr][r2]);

            // Assert that the RefTrisInclDups of all vertices in Vertex.GeoDups are identical to Vertices_RefTrisInclDups[VertexNr], barring the order.
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                for (unsigned long TriNr=0; TriNr<Vertices_RefTrisInclDups[VertexNr].Size(); TriNr++)
                    assert(Vertices_RefTrisInclDups[Vertex.GeoDups[DupNr]].Find(Vertices_RefTrisInclDups[VertexNr][TriNr])!=-1);
        }
#endif


        // ********************************************************************************
        //  Pre-compute all triangle neighbourhood relationships.
        //  This information is required for the stencil shadows silhouette determination.
        // ********************************************************************************

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            // Init with "no neighbours".
            Tri.NeighbIdx[0]=-1;
            Tri.NeighbIdx[1]=-1;
            Tri.NeighbIdx[2]=-1;
        }

        // Important note: If three triangles share a common edge, the relevant edge of *all* three triangles must be flagged with -2
        // (have multiple neighbours at this edge, treat like it was a free edge with no neighbour).
        // However, the fact that the three triangles share a common edge IS TYPICALLY DETECTED FOR ONLY *ONE* OF THE THREE TRIANGLES,
        // namely the one that has an orientation different from the two others.
        // We therefore also have to modify other triangles except for Tri1 at iteration Tri1Nr in order to make sure that all
        // triangle-edges at a triply-shared edge are set to -2 when such a case is detected.
        for (unsigned long Tri1Nr=0; Tri1Nr<Mesh.Triangles.Size(); Tri1Nr++)
        {
            MeshT::TriangleT& Tri1=Mesh.Triangles[Tri1Nr];

            for (unsigned long v1=0; v1<3; v1++)
            {
                // Note that the neighbour of edge <v1, (v1+1) % 3> is contained in the set of triangles that refer to v1.
                // Note that the Vertices_RefTrisInclDups array *includes* triangles that refer to geometrical duplicates of Mesh.Vertices[Tri1.VertexIdx[v1]].
                for (unsigned long RefNr=0; RefNr<Vertices_RefTrisInclDups[Tri1.VertexIdx[v1]].Size(); RefNr++)
                {
                    const unsigned long Tri2Nr=Vertices_RefTrisInclDups[Tri1.VertexIdx[v1]][RefNr];
                    MeshT::TriangleT&   Tri2  =Mesh.Triangles[Tri2Nr];

                    if (Tri1Nr==Tri2Nr) continue;

                    for (unsigned long v2=0; v2<3; v2++)
                    {
                        // The condition used to be as in the following line, which however does
                        // not take into account that vertices may be geometrical duplicates.
                     // if (Tri1.VertexIdx[v1]==Tri2.VertexIdx[(v2+1) % 3] && Tri1.VertexIdx[(v1+1) % 3]==Tri2.VertexIdx[v2])
                        if (Mesh.AreGeoDups(Tri1.VertexIdx[v1], Tri2.VertexIdx[(v2+1) % 3]) && Mesh.AreGeoDups(Tri1.VertexIdx[(v1+1) % 3], Tri2.VertexIdx[v2]))
                        {
                            // Tri1 and Tri2 are neighbours!
                            if (Tri1.NeighbIdx[v1]==-1)
                            {
                                // Tri1 had no neighbour at this edge before, set it now.
                                // This is the normal case.
                                Tri1.NeighbIdx[v1]=Tri2Nr;
                            }
                            else if (Tri1.NeighbIdx[v1]>=0)
                            {
                                // Tri1 had a single valid neighbour at this edge before, but we just found a second.
                                // That means that three triangles share a common edge!
                                // printf("WARNING: Triangle %lu has two neighbours at edge %lu: triangles %lu and %lu.\n", Tri1Nr, v1, Tri1.NeighbIdx[v1], Tri2Nr);

                                // Re-find the matching edge in the old neighbour.
                                MeshT::TriangleT& Tri3=Mesh.Triangles[Tri1.NeighbIdx[v1]];
                                unsigned long     v3;

                                for (v3=0; v3<2; v3++)      // The  v3<2  instead of  v3<3  is intentional, to be safe that v3 never gets 3 (out-of-range).
                                    if (Mesh.AreGeoDups(Tri1.VertexIdx[v1], Tri3.VertexIdx[(v3+1) % 3]) && Mesh.AreGeoDups(Tri1.VertexIdx[(v1+1) % 3], Tri3.VertexIdx[v3])) break;

                                // Set the shared edge of ALL THREE triangles to -2 in order to indicate that this edge leads to more than one neighbour.
                                Tri3.NeighbIdx[v3]=-2;
                                Tri2.NeighbIdx[v2]=-2;
                                Tri1.NeighbIdx[v1]=-2;
                            }
                            else /* (Tri1.NeighbIdx[v1]==-2) */
                            {
                                // This edge of Tri1 was either determined to be a triply-shared edge by some an earlier neighbour triangle,
                                // or there are even more than two neighbours at this edge...
                                // In any case, be sure to properly flag the relevant edge at the neighbour!
                                Tri2.NeighbIdx[v2]=-2;
                            }
                            break;
                        }
                    }
                }
            }
        }


        // *******************************************************************************************************
        //  For each vertex, determine the polarity of the triangle(s) is belongs to.
        //  If a vertex belongs to two or more triangles with different polarity, the vertex is on a mirror seam.
        //  In this case, the mesh is "split" by adding adding a new GeoDup for that vertex.
        // *******************************************************************************************************

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            // Init each the polarity of each vertex with *some* meaningful value (overwrites with different value possible here)!
            for (unsigned long i=0; i<3; i++)
                Mesh.Vertices[Tri.VertexIdx[i]].Polarity=Tri.Polarity;
        }

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            for (unsigned long i=0; i<3; i++)
                if (Mesh.Vertices[Tri.VertexIdx[i]].Polarity!=Tri.Polarity)
                {
                    // We found a vertex whose index is mentioned in a triangle with positive and in a triangle with negative polarity.
                    // Fix the situation by duplicating the vertex (it will become a GeoDup of the originial vertex).
                    const int OldVertexNr=Tri.VertexIdx[i];
                    const int NewVertexNr=Mesh.Vertices.Size();

                    Mesh.Vertices.PushBack(Mesh.Vertices[OldVertexNr]);

                    Mesh.Vertices[OldVertexNr].Polarity=true;
                    Mesh.Vertices[NewVertexNr].Polarity=false;


                    // Fix the GeoDups of all related vertices.
                    ArrayT<int> AllOldGeoDups=Mesh.Vertices[OldVertexNr].GeoDups;

                    unsigned long InsertPos;

                    for (InsertPos=0; InsertPos<AllOldGeoDups.Size(); InsertPos++)
                        if (AllOldGeoDups[InsertPos]>OldVertexNr)
                            break;

                    AllOldGeoDups.InsertAt(InsertPos, OldVertexNr);

                    for (unsigned long DupNr=0; DupNr<AllOldGeoDups.Size(); DupNr++)
                        Mesh.Vertices[AllOldGeoDups[DupNr]].GeoDups.PushBack(NewVertexNr);

                    Mesh.Vertices[NewVertexNr].GeoDups=AllOldGeoDups;


                    // Fix all the triangles that refer to OldVertexNr.
                    for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
                    {
                        MeshT::TriangleT& FixTri=Mesh.Triangles[TriangleNr];

                        for (unsigned long VNr=0; VNr<3; VNr++)
                            if (FixTri.VertexIdx[VNr]==OldVertexNr)
                            {
                                // Re-assign the vertex indices so that the polarity of the triangle matches the polarity of the vertex.
                             // if (FixTri.Polarity==Mesh.Vertices[OldVertexNr].Polarity) FixTri.VertexIdx[VNr]=OldVertexNr;    // Already have OldVertexNr here.
                                if (FixTri.Polarity==Mesh.Vertices[NewVertexNr].Polarity) FixTri.VertexIdx[VNr]=NewVertexNr;
                            }
                    }
                }
        }

#ifdef DEBUG
        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            for (unsigned long i=0; i<3; i++)
                assert(Mesh.Vertices[Tri.VertexIdx[i]].Polarity==Tri.Polarity);
        }
#endif
    }
}


ModelMd5T::~ModelMd5T()
{
    if (MatSys::Renderer==NULL) return;

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
        MatSys::Renderer->FreeMaterial(m_Meshes[MeshNr].RenderMaterial);
}


/***********************************************/
/*** Implementation of the ModelT interface. ***/
/***********************************************/

const std::string& ModelMd5T::GetFileName() const
{
    return m_FileName;
}


static void QuaternionSlerp(const float p[4], float q[4], float t, float qt[4])
{
    float a=0;
    float b=0;

    // Decide if one of the quaternions is backwards.
    for (int i=0; i<4; i++)
    {
        a+=(p[i]-q[i])*(p[i]-q[i]);
        b+=(p[i]+q[i])*(p[i]+q[i]);
    }

    if (a>b)
        for (int i=0; i<4; i++) q[i]=-q[i];

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

        for (int i=0; i<4; i++) qt[i]=sclp*p[i]+sclq*q[i];
    }
    else
    {
        // This code seems to handle the case that the quaternions are on opposite (180 degrees) points on the unit sphere,
        // e.g. the north and south pole. The problem is apparently solved by finding/constructing an arbitrary point on the
        // equator (I believe that the next four lines do that). The interpolation is then done "through" that point(?).
        qt[0]=-p[1];
        qt[1]= p[0];
        qt[2]=-p[3];
        qt[3]= p[2];

        // This code was originally copied from Model_mdl.cpp, but I believe it was wrong.
        // Irrlicht 0.12.0 has something that corresponds more to my pole/equator comment above,
        // and therefore I fix this code accordingly. See Model_mdl.cpp for the original (presumably wrong) code.
        const float sclp=sin((0.5f-t)*3.14159265358979323846f);
        const float sclq=sin(      t *3.14159265358979323846f);

        for (int i=0; i<4; i++) qt[i]=sclp*p[i]+sclq*qt[i];
    }
}


static Vector3fT myNormalize(const Vector3fT& A)
{
    const float Length=length(A);

    return (Length>0.000001f) ? A.GetScaled(1.0f/Length) : Vector3fT();
}


void ModelMd5T::UpdateCachedDrawData(int SequenceNr, float FrameNr) const
{
    // **************************************************************************************************************
    //  Obtain a joints (bone) hierarchy for the desired frame FrameNr of the desired animation sequence SequenceNr.
    //  The result will be a transformation matrix for each joint (bone).
    // **************************************************************************************************************

    if (SequenceNr==-1)
    {
        // Don't do animation, just use the pose defined in the md5mesh file.
        for (unsigned long JointNr=0; JointNr<m_Joints.Size(); JointNr++)
        {
            const JointT& J=m_Joints[JointNr];
            const float   t=1.0f - J.Qtr.x*J.Qtr.x - J.Qtr.y*J.Qtr.y - J.Qtr.z*J.Qtr.z;
            const float   w=(t<0.0f) ? 0.0f : -sqrt(t);
            const float   Q[4]={ J.Qtr.x, J.Qtr.y, J.Qtr.z, w };

            m_Draw_JointMatrices[JointNr]=MatrixT
            (
                1.0f-2.0f*Q[1]*Q[1]-2.0f*Q[2]*Q[2],      2.0f*Q[0]*Q[1]-2.0f*Q[3]*Q[2],      2.0f*Q[0]*Q[2]+2.0f*Q[3]*Q[1], J.Pos.x,
                     2.0f*Q[0]*Q[1]+2.0f*Q[3]*Q[2], 1.0f-2.0f*Q[0]*Q[0]-2.0f*Q[2]*Q[2],      2.0f*Q[1]*Q[2]-2.0f*Q[3]*Q[0], J.Pos.y,
                     2.0f*Q[0]*Q[2]-2.0f*Q[3]*Q[1],      2.0f*Q[1]*Q[2]+2.0f*Q[3]*Q[0], 1.0f-2.0f*Q[0]*Q[0]-2.0f*Q[1]*Q[1], J.Pos.z,
                                              0.0f,                               0.0f,                               0.0f,    1.0f
            );
        }
    }
    else
    {
        // SequenceNr is a valid index into m_Anims, so use that.
        const AnimT& Anim=m_Anims[SequenceNr];
        const int    Frame_0=int(FrameNr);                                          // If FrameNr == 17.83, then Frame_0 == 17
        const float  Frame_f=FrameNr-Frame_0;                                       //                           Frame_f ==  0.83
        const int    Frame_1=(Frame_0+1>=int(Anim.Frames.Size())) ? 0 : Frame_0+1;  //                           Frame_1 == 18

        for (unsigned long JointNr=0; JointNr<m_Joints.Size(); JointNr++)
        {
            const AnimT::AnimJointT& AJ=Anim.AnimJoints[JointNr];


            // Determine the position and quaternion for Frame_0.
            float Data_0[7];

            int FlagCount=0;
            for (int i=0; i<6; i++)
            {
                const bool FlagIsSet=((AJ.Flags >> i) & 1)!=0;

                Data_0[i]=FlagIsSet ? Anim.Frames[Frame_0].AnimData[AJ.FirstDataIdx+FlagCount] : AJ.BaseValues[i];

                if (FlagIsSet) FlagCount++;
            }

            const float t_0=1.0f - Data_0[3]*Data_0[3] - Data_0[4]*Data_0[4] - Data_0[5]*Data_0[5];
            Data_0[6]=(t_0<0.0f) ? 0.0f : -sqrt(t_0);


            // Determine the position and quaternion for Frame_1.
            float Data_1[7];

            FlagCount=0;
            for (int i=0; i<6; i++)
            {
                const bool FlagIsSet=((AJ.Flags >> i) & 1)!=0;

                Data_1[i]=FlagIsSet ? Anim.Frames[Frame_1].AnimData[AJ.FirstDataIdx+FlagCount] : AJ.BaseValues[i];

                if (FlagIsSet) FlagCount++;
            }

            const float t_1=1.0f - Data_1[3]*Data_1[3] - Data_1[4]*Data_1[4] - Data_1[5]*Data_1[5];
            Data_1[6]=(t_1<0.0f) ? 0.0f : -sqrt(t_1);


            // Interpolate the position and quaternion according to the fraction Frame_f.
            float Pos[3];

            for (int i=0; i<3; i++)
                Pos[i]=(1.0f-Frame_f)*Data_0[i] + Frame_f*Data_1[i];

            float Q[4];

            QuaternionSlerp(&Data_0[3], &Data_1[3], Frame_f, Q);      // Warning: This methods also modifies Data_1 (as scratch space)!


            // Compute the matrix, which is relative to the parent bone.
            const MatrixT RelMatrix
            (
                1.0f-2.0f*Q[1]*Q[1]-2.0f*Q[2]*Q[2],      2.0f*Q[0]*Q[1]-2.0f*Q[3]*Q[2],      2.0f*Q[0]*Q[2]+2.0f*Q[3]*Q[1], Pos[0],
                     2.0f*Q[0]*Q[1]+2.0f*Q[3]*Q[2], 1.0f-2.0f*Q[0]*Q[0]-2.0f*Q[2]*Q[2],      2.0f*Q[1]*Q[2]-2.0f*Q[3]*Q[0], Pos[1],
                     2.0f*Q[0]*Q[2]-2.0f*Q[3]*Q[1],      2.0f*Q[1]*Q[2]+2.0f*Q[3]*Q[0], 1.0f-2.0f*Q[0]*Q[0]-2.0f*Q[1]*Q[1], Pos[2],
                                              0.0f,                               0.0f,                               0.0f,   1.0f
            );


            // And finally obtain the absolute matrix for that bone!
            const JointT& J=m_Joints[JointNr];

            m_Draw_JointMatrices[JointNr]=(J.Parent==-1) ? RelMatrix : m_Draw_JointMatrices[J.Parent]*RelMatrix;
        }
    }


    // *******************************************************************************************************************
    //  The JointMatrices represent now the pose of the model at the desired frame number of the desired sequence number.
    //  For all meshes do now compute the vertices according to their weights.
    // *******************************************************************************************************************

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            if (Vertex.GeoDups.Size()>0 && Vertex.GeoDups[0]<int(VertexNr))
            {
                // This vertex has a geometrically identical duplicate that has already been computed.
                // Therefore, don't bother to recompute the same position again, just copy it from the duplicate.
                Vertex.Draw_Pos=Mesh.Vertices[Vertex.GeoDups[0]].Draw_Pos;
                continue;
            }

            if (Vertex.NumWeights==1)
            {
                const MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx];

                Vertex.Draw_Pos=m_Draw_JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos);
            }
            else
            {
                Vertex.Draw_Pos=Vector3fT(0.0f, 0.0f, 0.0f);

                for (int WeightNr=0; WeightNr<Vertex.NumWeights; WeightNr++)
                {
                    const MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx+WeightNr];

                    Vertex.Draw_Pos+=m_Draw_JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos) * Weight.Weight;
                }
            }
        }
    }


    // *******************************************************************************************
    //  Compute the tangent-space basis vectors for all triangles and all vertices.
    //  This is done by first computing the per-triangle axes and then having them enter
    //  the relevant per-vertex averages as required (taking mirror corrections into account).
    //  The per-triangle normal vectors are also kept for stencil shadow silhoutte determination.
    // *******************************************************************************************

    if (m_UseGivenTangentSpace)
    {
        assert(m_Anims.Size()==0);  // It doesn't make sense to have statically given tangent-space axes with *animated* geometry...
        goto DoneComputingTS;
    }

    // For all vertices, zero the tangent-space vectors for the subsequent average accumulation.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            Vertex.Draw_Normal  =Vector3fT(0, 0, 0);
            Vertex.Draw_Tangent =Vector3fT(0, 0, 0);
            Vertex.Draw_BiNormal=Vector3fT(0, 0, 0);
        }
    }

    // Compute the per-triangle tangent-space axes and distribute them over the relevant vertices appropriately.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            MeshT::TriangleT&     Tri   =Mesh.Triangles[TriangleNr];
            const MeshT::VertexT& V_0   =Mesh.Vertices[Tri.VertexIdx[0]];
            const MeshT::VertexT& V_1   =Mesh.Vertices[Tri.VertexIdx[1]];
            const MeshT::VertexT& V_2   =Mesh.Vertices[Tri.VertexIdx[2]];
            const Vector3fT       Edge01=V_1.Draw_Pos-V_0.Draw_Pos;
            const Vector3fT       Edge02=V_2.Draw_Pos-V_0.Draw_Pos;

            // Triangles are ordered CW for md5 models and CCW for ase models, so we write
            // Normal=VectorCross(Edge02, Edge01) for md5 models and Normal=VectorCross(Edge01, Edge02) for ase models.
            Tri.Draw_Normal=myNormalize(Edge02.cross(Edge01));

            // Understanding what's going on here is easy. The key statement is
            // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
            // First, there is a short explanation in "The Cg Tutorial", chapter 8.
            // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
            const Vector3fT uv01=Vector3fT(V_1.u, V_1.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
            const Vector3fT uv02=Vector3fT(V_2.u, V_2.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
            const float     f   =uv01.x*uv02.y-uv01.y*uv02.x>0.0 ? 1.0f : -1.0f;

            const Vector3fT Tri_Draw_Tangent =myNormalize(Edge02.GetScaled(-uv01.y*f) + Edge01.GetScaled(uv02.y*f));
            const Vector3fT Tri_Draw_BiNormal=myNormalize(Edge02.GetScaled( uv01.x*f) - Edge01.GetScaled(uv02.x*f));


            // Distribute the per-triangle tangent-space over the affected vertices.
#if 1
            const float Pi=3.14159265358979323846f;

            const float c0=dot(myNormalize(Edge01), myNormalize(Edge02));
            const float c1=dot(myNormalize(Edge01), myNormalize(V_1.Draw_Pos-V_2.Draw_Pos));

            const float w0=(c0>=1.0f) ? 0.0f : ( (c0<=-1.0f) ? Pi : acos(c0) );
            const float w1=(c1>=1.0f) ? 0.0f : ( (c1<=-1.0f) ? Pi : acos(c1) );

            const float TriWeight[3]={ w0, w1, Pi-TriWeight[0]-TriWeight[1] };
#else
            const float TriWeight[3]={ 1.0f, 1.0f, 1.0f };
#endif

            for (int i=0; i<3; i++)
            {
                MeshT::VertexT& Vertex=Mesh.Vertices[Tri.VertexIdx[i]];

                assert(Tri.Polarity==Vertex.Polarity);

                Vertex.Draw_Normal  +=Tri.Draw_Normal*TriWeight[i];
                Vertex.Draw_Tangent +=Tri_Draw_Tangent*TriWeight[i];
                Vertex.Draw_BiNormal+=Tri_Draw_BiNormal*TriWeight[i];

                for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                {
                    MeshT::VertexT& DupVertex=Mesh.Vertices[Vertex.GeoDups[DupNr]];

                    DupVertex.Draw_Normal  +=Tri.Draw_Normal*TriWeight[i];
                    DupVertex.Draw_Tangent +=Tri_Draw_Tangent*(Tri.Polarity==DupVertex.Polarity ? TriWeight[i] : -TriWeight[i]);
                    DupVertex.Draw_BiNormal+=Tri_Draw_BiNormal*TriWeight[i];
                }
            }
        }
    }

    // Finally normalize the per-vertex tangent-space axes; this is quasi the "division" in the average computations.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            // Normalize the tangent-space axes.
            Vertex.Draw_Normal  =myNormalize(Vertex.Draw_Normal  );
            Vertex.Draw_Tangent =myNormalize(Vertex.Draw_Tangent );
            Vertex.Draw_BiNormal=myNormalize(Vertex.Draw_BiNormal);
        }
    }

    DoneComputingTS:


    // ***************************************************************************************************************
    //  Construct explicit MatSys::MeshT meshes now.
    //  Note that this is very inefficient - we REALLY should work with index arrays! (and/or vertex buffer objects!)
    // ***************************************************************************************************************

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const MeshT& Mesh=m_Meshes[MeshNr];

        for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
        {
            for (unsigned long i=0; i<3; i++)
            {
                unsigned long VertexIdx=Mesh.Triangles[TriNr].VertexIdx[i];

                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetOrigin(Mesh.Vertices[VertexIdx].Draw_Pos.x, Mesh.Vertices[VertexIdx].Draw_Pos.y, Mesh.Vertices[VertexIdx].Draw_Pos.z);
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetTextureCoord(Mesh.Vertices[VertexIdx].u, Mesh.Vertices[VertexIdx].v);
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetNormal  (Mesh.Vertices[VertexIdx].Draw_Normal.x,   Mesh.Vertices[VertexIdx].Draw_Normal.y,   Mesh.Vertices[VertexIdx].Draw_Normal.z  );
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetTangent (Mesh.Vertices[VertexIdx].Draw_Tangent.x,  Mesh.Vertices[VertexIdx].Draw_Tangent.y,  Mesh.Vertices[VertexIdx].Draw_Tangent.z );
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetBiNormal(Mesh.Vertices[VertexIdx].Draw_BiNormal.x, Mesh.Vertices[VertexIdx].Draw_BiNormal.y, Mesh.Vertices[VertexIdx].Draw_BiNormal.z);
            }
        }
    }
}


void ModelMd5T::Draw(int SequenceNr, float FrameNr, float /*LodDist*/, const ModelT* /*SubModel*/) const
{
    // SequenceNr==-1 means "use the base pose from the md5mesh file only (no md5anim)".
    if (SequenceNr>=int(m_Anims.Size())) SequenceNr=-1;
    if (SequenceNr!=-1 && (m_Anims[SequenceNr].FPS<0.0 || m_Anims[SequenceNr].Frames.Size()==0)) SequenceNr=-1;
    if (SequenceNr==-1) FrameNr=0.0;


    // Do an early check whether the light and the sequence BBs intersect.
    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
            break;

        case MatSys::RendererI::LIGHTING:
        case MatSys::RendererI::STENCILSHADOW:
        {
            const float                LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();
            const Vector3T<float>      LightBox(LightRadius, LightRadius, LightRadius);
            const Vector3T<float>      LightPosOLD(MatSys::Renderer->GetCurrentLightSourcePosition());
            const BoundingBox3T<float> LightBB(LightPosOLD+LightBox, LightPosOLD-LightBox);
            const float*               SequenceBB=GetSequenceBB(SequenceNr, FrameNr);

            if (!LightBB.Intersects(BoundingBox3T<float>(Vector3fT(&SequenceBB[0]), Vector3fT(&SequenceBB[3])))) return;
            break;
        }
    }


    // The caching mechanism is really simple and works as follows:
    // All data that does not depend on a frame or sequence number is precomputed and prepared once in the constructor.
    // All data that is specific to a certain frame and sequence number is computed in UpdateCachedDrawData()
    // and stored in member variables (those whose name begins with m_Draw_ or Draw_).
    // If we are called again with the same frame and sequence numbers for which we have cached data in the last call,
    // the data is simply re-used for drawing, skipping the expensive recomputations entirely.
    // If the numbers are different, the cache data is recomputed and the reference frame and sequence numbers are updated.
    //
    // 1. To understand why this works well, consider first a scene with only a single animated model:
    // First, we assume that the model will be drawn in the ambient pass plus for each light source in the stencil shadow and lighting pass.
    // That is, assuming there are L light sources, this method is called 1+2*L times per frame with the same frame and sequence numbers.
    // It's obvious that the first call for the ambient pass will come with different sequence or frame numbers than the
    // previous call, as our model is animated. In this call the cache is recomputed, and it is obvious that for the 2*L subsequent
    // calls for the stencil and lighting passes the frame and sequence numbers will be identical so that we can rely on the cached data.
    // Thus, the computational effort with the cache is only 1/(1+2*L) of the amount without the cache.
    //
    // Note that we can ***NOT*** reasonably do any better by a more complex cache management (as is done in the ModelMdlT class).
    // For example one might think that we might win something if we delay the computations of the tangent space in the ambient
    // pass until a lighting pass occurs that actually needs the tangent space.
    // First of all, this makes the cache management terribly complicated, see the ModelMdlT class for a scaring example.
    // Even better, we may reasonably assume that we will need *all* cache data in each frame eventually (letztendlich) anyway,
    // so that it is reasonable to compute it fully whenever the frame or sequence number changes.
    // The only case where the assumption does not hold (and we could have saved computing something) is when no lightsource at all
    // lights our model, and when also the ambient pass does not need the information (e.g. normal vectors for environment reflection mapping).
    // As this is hopefully a rare case, we're doing pretty good.
    // Border cases like outdoor terrains where animated player models are drawn only in the ambient pass should be addressed by dlod models.
    //
    // 2. Now consider a scene with multiple animated models that are all rendered by this single instance.
    // Unfortunately, in this case our caching mechanism fails entirely: everything is recomputed on every call.
    // In this case, several instances of the model should be created (if speed is important and space is dispensable).
    // The fact that the Cafu Material System automatically shares rendering resources makes having multiple instances viable.
    //
    // 3. Finally, consider a scene with arbitrarily many non-animated models.
    // As in this case the frame and sequence numbers are always identical for each call, the caching mechanism has maximum efficiency.
    // All computations occur only once and then never again. This is the optimum cache utilization case.
    if (m_Draw_CachedDataAtSequNr!=SequenceNr || m_Draw_CachedDataAtFrameNr!=FrameNr)
    {
        m_Draw_CachedDataAtSequNr =SequenceNr;
        m_Draw_CachedDataAtFrameNr=FrameNr;

        UpdateCachedDrawData(SequenceNr, FrameNr);
    }


    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
        {
            for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
            {
                MatSys::Renderer->SetCurrentMaterial(m_Meshes[MeshNr].RenderMaterial);
                MatSys::Renderer->RenderMesh(m_Draw_Meshes[MeshNr]);

#if 0
                // Render the tangent space axes for each vertex.
                static MaterialT SolidColorMaterial;
                SolidColorMaterial.UseMeshColors=true;

                static MatSys::MeshT TangentSpaceAxes(MatSys::MeshT::Lines);
                TangentSpaceAxes.Vertices.Overwrite();

                for (unsigned long VertexNr=0; VertexNr<m_Draw_Meshes[MeshNr].Vertices.Size(); VertexNr++)
                {
                    const float         scale=1.0f;
                    const Vector3fT     Orig =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].Origin);
                    const Vector3fT     S_   =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].Tangent);
                    const Vector3fT     T_   =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].BiNormal);
                    const Vector3fT     N_   =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].Normal);
                    const float         col_ =m_Meshes[MeshNr].Triangles[VertexNr / 3].Polarity ? 0.5f : 0.0f;
                    const unsigned long Ofs  =TangentSpaceAxes.Vertices.Size();

                    TangentSpaceAxes.Vertices.PushBackEmpty(6);

                    TangentSpaceAxes.Vertices[Ofs+0].SetOrigin(Orig);
                    TangentSpaceAxes.Vertices[Ofs+0].SetColor(1, col_, col_);
                    TangentSpaceAxes.Vertices[Ofs+1].SetOrigin(Orig+S_*scale);
                    TangentSpaceAxes.Vertices[Ofs+1].SetColor(1, col_, col_);

                    TangentSpaceAxes.Vertices[Ofs+2].SetOrigin(Orig);
                    TangentSpaceAxes.Vertices[Ofs+2].SetColor(col_, 1, col_);
                    TangentSpaceAxes.Vertices[Ofs+3].SetOrigin(Orig+T_*scale);
                    TangentSpaceAxes.Vertices[Ofs+3].SetColor(col_, 1, col_);

                    TangentSpaceAxes.Vertices[Ofs+4].SetOrigin(Orig);
                    TangentSpaceAxes.Vertices[Ofs+4].SetColor(col_, col_, 1);
                    TangentSpaceAxes.Vertices[Ofs+5].SetOrigin(Orig+N_*scale);
                    TangentSpaceAxes.Vertices[Ofs+5].SetColor(col_, col_, 1);
                }

                MatSys::RenderMaterialT* SolidColorRenderMat=MatSys::Renderer->RegisterMaterial(&SolidColorMaterial);

                MatSys::Renderer->SetCurrentMaterial(SolidColorRenderMat);
                MatSys::Renderer->RenderMesh(TangentSpaceAxes);

                MatSys::Renderer->FreeMaterial(SolidColorRenderMat);

                // FIXME! Rendering the stencil shadows uses the same material as the ambient pass does!
                // (The call to FreeMaterial() above implies that no material is being set, and thus without this line,
                //  no stencil shadows get rendered!)
                MatSys::Renderer->SetCurrentMaterial(m_Meshes[MeshNr].RenderMaterial);
#endif
            }
            break;
        }

        case MatSys::RendererI::LIGHTING:
        {
            for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
            {
                MatSys::Renderer->SetCurrentMaterial(m_Meshes[MeshNr].RenderMaterial);
                MatSys::Renderer->RenderMesh(m_Draw_Meshes[MeshNr]);
            }
            break;
        }

        case MatSys::RendererI::STENCILSHADOW:
        {
            const Vector3fT LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());

            for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
            {
                const MeshT& Mesh=m_Meshes[MeshNr];

                if (Mesh.Material==NULL || Mesh.Material->NoShadows) continue;

                static ArrayT<bool> TriangleIsFrontFacing;
                if (TriangleIsFrontFacing.Size()<Mesh.Triangles.Size())
                    TriangleIsFrontFacing.PushBackEmpty(Mesh.Triangles.Size()-TriangleIsFrontFacing.Size());

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    const MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];
                    const float             Dot=(LightPos-Mesh.Vertices[Tri.VertexIdx[0]].Draw_Pos).dot(Tri.Draw_Normal);

                    TriangleIsFrontFacing[TriNr]=Dot>0;
                }


                // Note that we have to cull the following polygons wrt. the *VIEWER* (not the light source)!
                static MatSys::MeshT MeshSilhouette(MatSys::MeshT::Quads);  // The default winding order is "CW".
                MeshSilhouette.Vertices.Overwrite();

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    if (!TriangleIsFrontFacing[TriNr]) continue;

                    // This triangle is front-facing wrt. the light source.
                    const MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];

                    for (unsigned long EdgeNr=0; EdgeNr<3; EdgeNr++)
                    {
                        // If an edge has no (-1) or more than one (-2) neighbours, make it a silhouette edge.
                        const bool IsSilhouetteEdge=(Tri.NeighbIdx[EdgeNr]<0) ? true : !TriangleIsFrontFacing[Tri.NeighbIdx[EdgeNr]];

                        if (IsSilhouetteEdge)
                        {
                            // The neighbour at edge 'EdgeNr' is back-facing (or non-existant), so we have found a possible silhouette edge.
                            const unsigned long v1=EdgeNr;
                            const unsigned long v2=(EdgeNr+1) % 3;
                            const Vector3fT     LA=Mesh.Vertices[Tri.VertexIdx[v1]].Draw_Pos-LightPos;
                            const Vector3fT     LB=Mesh.Vertices[Tri.VertexIdx[v2]].Draw_Pos-LightPos;

                            MeshSilhouette.Vertices.PushBackEmpty(4);

                            const unsigned long MeshSize=MeshSilhouette.Vertices.Size();

                            MeshSilhouette.Vertices[MeshSize-4].SetOrigin(Mesh.Vertices[Tri.VertexIdx[v2]].Draw_Pos);
                            MeshSilhouette.Vertices[MeshSize-3].SetOrigin(Mesh.Vertices[Tri.VertexIdx[v1]].Draw_Pos);
                            MeshSilhouette.Vertices[MeshSize-2].SetOrigin(LA, 0.0);
                            MeshSilhouette.Vertices[MeshSize-1].SetOrigin(LB, 0.0);
                        }
                    }
                }

                MatSys::Renderer->RenderMesh(MeshSilhouette);


                static MatSys::MeshT MeshCaps(MatSys::MeshT::Triangles);    // The default winding order is "CW".
                MeshCaps.Vertices.Overwrite();

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    if (!TriangleIsFrontFacing[TriNr]) continue;

                    // This triangle is front-facing wrt. the light source.
                    const MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];

                    MeshCaps.Vertices.PushBackEmpty(6);

                    const unsigned long MeshSize=MeshCaps.Vertices.Size();

                    // Render the occluder (front-facing wrt. the light source).
                    const Vector3fT& A=Mesh.Vertices[Tri.VertexIdx[0]].Draw_Pos;
                    const Vector3fT& B=Mesh.Vertices[Tri.VertexIdx[1]].Draw_Pos;
                    const Vector3fT& C=Mesh.Vertices[Tri.VertexIdx[2]].Draw_Pos;

                    MeshCaps.Vertices[MeshSize-6].SetOrigin(A);
                    MeshCaps.Vertices[MeshSize-5].SetOrigin(B);
                    MeshCaps.Vertices[MeshSize-4].SetOrigin(C);

                    // Render the occluder (back-facing wrt. the light source).
                    const Vector3fT LA=A-LightPos;
                    const Vector3fT LB=B-LightPos;
                    const Vector3fT LC=C-LightPos;

                    MeshCaps.Vertices[MeshSize-3].SetOrigin(LC, 0.0);
                    MeshCaps.Vertices[MeshSize-2].SetOrigin(LB, 0.0);
                    MeshCaps.Vertices[MeshSize-1].SetOrigin(LA, 0.0);
                }

                MatSys::Renderer->RenderMesh(MeshCaps);
            }

            break;
        }
    }
}


bool ModelMd5T::GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    // To be implemented...
    return false;
}


void ModelMd5T::Print() const
{
    printf("\nThis is an md5 model. FileName: \"%s\"\n", m_FileName.c_str());

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const MeshT& Mesh=m_Meshes[MeshNr];

        printf("\n### Mesh %lu ####\n", MeshNr);

        printf("Triangles:\n");
        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            std::cout << TriangleNr << ": ";
            std::cout << "Vertices ("   << Tri.VertexIdx[0] << " " << Tri.VertexIdx[1] << " " << Tri.VertexIdx[2] << "), ";
            std::cout << "Neighbours (" << Tri.NeighbIdx[0] << " " << Tri.NeighbIdx[1] << " " << Tri.NeighbIdx[2] << "), ";
            std::cout << "Polarity " << (Tri.Polarity ? '+' : '-');
            std::cout << "\n";
        }

        printf("Vertices:\n");
        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            std::cout << VertexNr << ": ";
            std::cout << "uv (" << Vertex.u << ", " << Vertex.v << "), ";
            std::cout << "wgt: " << Vertex.FirstWeightIdx << " " << Vertex.NumWeights << ", ";
            std::cout << "geodups [";
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                std::cout << " " << Vertex.GeoDups[DupNr];
            std::cout << "], ";
            std::cout << "Polarity " << (Vertex.Polarity ? '+' : '-');
            std::cout << "\n";
        }
    }
}


int ModelMd5T::GetNrOfSequences() const
{
    return m_Anims.Size();
}


const float* ModelMd5T::GetSequenceBB(int SequenceNr, float FrameNr) const
{
    if (SequenceNr==-1 || SequenceNr>=int(m_Anims.Size()) || m_Anims[SequenceNr].Frames.Size()==0 || m_Anims[SequenceNr].FPS<0.0)
        return m_BaseBB;

    // Should we interpolate the bounding box between frames as we interpolate the bones?
    const int FNr=(int(FrameNr+0.5f)) % m_Anims[SequenceNr].Frames.Size();

    return m_Anims[SequenceNr].Frames[FNr].BB;
}


// float ModelMd5T::GetNrOfFrames(int /*SequenceNr*/) const
// {
//     return 0.0;
// }


float ModelMd5T::AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop) const
{
    if (SequenceNr<0 || SequenceNr>=int(m_Anims.Size())) return 0.0f;
    const int NumFrames=m_Anims[SequenceNr].Frames.Size();
    if (NumFrames<=1) return 0.0f;

    float NewFrameNr=FrameNr+DeltaTime*m_Anims[SequenceNr].FPS;

    if (Loop)
    {
        // Wrap the sequence (it's a looping (repeating) sequence, like idle, walk, ...).
        NewFrameNr-=(int)(NewFrameNr/NumFrames)*NumFrames;
    }
    else
    {
        // Clamp the sequence (it's a play-once (non-repeating) sequence, like dying).
        // On clamping, stop the sequence 1/100th sec before the end of the last frame.
        if (NewFrameNr>=float(NumFrames-1)) NewFrameNr=float(NumFrames-1)-0.01f;
    }

    return NewFrameNr;
}
