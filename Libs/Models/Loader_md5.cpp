/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_md5.hpp"
#include "MaterialSystem/Material.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Math3D/Quaternion.hpp"
#include "TextParser/TextParser.hpp"
#include "String.hpp"

#include <stdio.h>


LoaderMd5T::LoaderMd5T(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags)
{
}


void LoaderMd5T::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    ArrayT<std::string> ComponentFiles;

    if (cf::String::EndsWith(m_FileName, "md5"))
    {
        TextParserT TP(m_FileName.c_str());

        while (!TP.IsAtEOF()) ComponentFiles.PushBack(TP.GetNextToken());
        if (ComponentFiles.Size()<1) throw LoadErrorT("No component files found in md5 file.");

        size_t PrefixLength=m_FileName.length();
        while (PrefixLength>0 && m_FileName[PrefixLength-1]!='/' && m_FileName[PrefixLength-1]!='\\') PrefixLength--;
        const std::string Prefix=std::string(m_FileName, 0, PrefixLength);

        for (unsigned long FileNr=0; FileNr<ComponentFiles.Size(); FileNr++)
        {
            // The ComponentFiles are specified relative to the parent file name,
            // thus extract the path portion from m_FileName and prepend it to the ComponentFiles.
            // If however the second character of a ComponentFiles entry is a ":", treat this as
            // a drive letter specification and thus as the special case of an absolute path.
            if (ComponentFiles[FileNr].length()>=2 && ComponentFiles[FileNr][1]==':') continue;

            ComponentFiles[FileNr]=Prefix+ComponentFiles[FileNr];
        }
    }
    else if (cf::String::EndsWith(m_FileName, "md5mesh"))
    {
        ComponentFiles.PushBack(m_FileName);
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
            const std::string Tok=TP.GetNextToken();

                 if (Tok=="MD5Version" ) { if (TP.GetNextToken()!="10") throw LoadErrorT("MD5Version is not 10."); }
            else if (Tok=="commandline") TP.GetNextToken();       // Ignore the command line.
            else if (Tok=="numJoints"  ) TP.GetNextToken();       // Ignore the given number of joints - we just load as many as we find.
            else if (Tok=="numMeshes"  ) TP.GetNextToken();       // Ignore the given number of meshes - we just load as many as we find.
            else if (Tok=="joints")
            {
                TP.AssertAndSkipToken("{");
                ArrayT<MatrixT> ParentGlobalInverse;

                while (true)
                {
                    CafuModelT::JointT Joint;

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
                    if (Joint.Parent>=int(Joints.Size()))
                    {
                        printf("WARNING: Bad bone order!  %lu bones read so far, and the next (name \"%s\") is referring to parent %i.\n", Joints.Size(), Joint.Name.c_str(), Joint.Parent);
                        throw LoadErrorT("The bones are not properly ordered.");  // TODO: Fix this by re-sorting the joints rather than by abortion!
                    }

                    const MatrixT GlobalMat(Joint.Pos, cf::math::QuaternionfT::FromXYZ(Joint.Qtr));
                    const MatrixT LocalMat=(Joint.Parent==-1) ? GlobalMat : ParentGlobalInverse[Joint.Parent] * GlobalMat;
                    cf::math::Matrix3x3fT Mat3x3;

                    for (unsigned long i=0; i<3; i++)
                        for (unsigned long j=0; j<3; j++)
                            Mat3x3[i][j]=LocalMat[i][j];

                    Joint.Pos   =Vector3fT(LocalMat[0][3], LocalMat[1][3], LocalMat[2][3]);
                    Joint.Qtr   =cf::math::QuaternionfT(Mat3x3).GetXYZ();
                    Joint.Scale =Vector3fT(1.0f, 1.0f, 1.0f);

                    ParentGlobalInverse.PushBack(GlobalMat.GetInverse());
                    Joints.PushBack(Joint);
                }
            }
            else if (Tok=="mesh")
            {
                Meshes.PushBackEmpty();
                CafuModelT::MeshT& Mesh=Meshes[Meshes.Size()-1];

                TP.AssertAndSkipToken("{");

                Mesh.Name="Mesh";

                while (true)
                {
                    const std::string Token=TP.GetNextToken();

                         if (Token=="}"         ) break;
                    else if (Token=="numtris"   ) { if (Mesh.Triangles.Size()>0) throw LoadErrorT("Mesh.Triangles.Size()>0"); Mesh.Triangles.PushBackEmpty(TP.GetNextTokenAsInt()); }
                    else if (Token=="numverts"  ) { if (Mesh.Vertices .Size()>0) throw LoadErrorT("Mesh.Vertices .Size()>0"); Mesh.Vertices .PushBackEmpty(TP.GetNextTokenAsInt()); }
                    else if (Token=="numweights") { if (Mesh.Weights  .Size()>0) throw LoadErrorT("Mesh.Weights  .Size()>0"); Mesh.Weights  .PushBackEmpty(TP.GetNextTokenAsInt()); }
                    else if (Token=="shader")
                    {
                        const std::string MatName=TP.GetNextToken();

                        Mesh.Material=MaterialMan.GetMaterial(MatName);

                        if (!Mesh.Material)
                        {
                            // Short of parsing D3/Q4 material scripts, we cannot reasonably reconstruct materials here.
                            // Thus if there isn't an appropriately prepared .cmat file (so that MatName is found in MaterialMan),
                            // go for the wire-frame substitute straight away.
                            Mesh.Material=MaterialMan.RegisterMaterial(CreateDefaultMaterial(MatName));
                        }
                    }
                    else if (Token=="tri")
                    {
                        const unsigned long TriIdx=TP.GetNextTokenAsInt();

                        if (TriIdx>=Mesh.Triangles.Size()) throw LoadErrorT("TriIdx >= Mesh.Triangles.Size()");

                        Mesh.Triangles[TriIdx].VertexIdx[0]=TP.GetNextTokenAsInt();
                        Mesh.Triangles[TriIdx].VertexIdx[1]=TP.GetNextTokenAsInt();
                        Mesh.Triangles[TriIdx].VertexIdx[2]=TP.GetNextTokenAsInt();

                        // All triangles are in a common smoothing group.
                        Mesh.Triangles[TriIdx].SmoothGroups=0x01;
                    }
                    else if (Token=="vert")
                    {
                        const unsigned long VertIdx=TP.GetNextTokenAsInt();

                        if (VertIdx>=Mesh.Vertices.Size()) throw LoadErrorT("VertIdx >= Mesh.Vertices.Size()");

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

                        if (WeightIdx>=Mesh.Weights.Size()) throw LoadErrorT("WeightIdx >= Mesh.Weights.Size()");

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
                printf("Unknown token \"%s\", skipping...\n", Tok.c_str());
                if (TP.GetNextToken()=="{") TP.SkipBlock("{", "}", true);
            }
        }
    }
    catch (const TextParserT::ParseError&)
    {
        throw LoadErrorT("Could not parse the file.");
    }


    // Read the individual animation sequence files.
    for (unsigned long FileNr=1; FileNr<ComponentFiles.Size(); FileNr++)
    {
        try
        {
            ImporterMd5AnimT Importer(ComponentFiles[FileNr]);

            Anims.PushBack(Importer.Import(Joints, Meshes));
        }
        catch (const LoadErrorT& LE)
        {
            // Loading this animation sequence failed, but as the base mesh (the md5mesh file)
            // loaded properly, that is not reason enough to abort loading the entire model.
            CafuModelT::AnimT InvalidAnim;

            InvalidAnim.FPS=-1.0f;          // Use a negative FPS to flag this animation as invalid.
            Anims.PushBack(InvalidAnim);    // Note that InvalidAnim.Frames.Size()==0, too.

            printf("WARNING: Loading animation sequence file %s failed:\n", ComponentFiles[FileNr].c_str());
            printf("%s\n", LE.what());
        }
    }
}


ImporterMd5AnimT::ImporterMd5AnimT(const std::string& FileName)
    : AnimImporterT(FileName)
{
}


ArrayT<CafuModelT::AnimT> ImporterMd5AnimT::Import(const ArrayT<CafuModelT::JointT>& Joints, const ArrayT<CafuModelT::MeshT>& Meshes)
{
    TextParserT TP(m_FileName.c_str());

    try
    {
        CafuModelT::AnimT Anim;

        Anim.Name=cf::String::StripExt(m_FileName);
        Anim.FPS =24.0f;
        Anim.Next=-1;

        const size_t PathLength=cf::String::GetPath(Anim.Name).length() + 1;

        if (PathLength < Anim.Name.length())
            Anim.Name=std::string(Anim.Name, PathLength);

        bool GotHierarchy=false;
        bool GotBounds   =false;
        bool GotBaseframe=false;
        ArrayT<bool> GotFrames;

        while (!TP.IsAtEOF())
        {
            const std::string Token=TP.GetNextToken();

                 if (Token=="MD5Version" ) { if (TP.GetNextToken()!="10") throw ModelLoaderT::LoadErrorT("MD5Version is not 10."); }
            else if (Token=="commandline") TP.GetNextToken();       // Ignore the command line.
            else if (Token=="frameRate"  ) Anim.FPS=TP.GetNextTokenAsFloat();
            else if (Token=="numFrames"  )
            {
                // Be stricter with numFrames than with some num* variables in the md5mesh file.
                // This is required because for example there must be as many bounding boxes as frames.
                if (Anim.Frames.Size()>0) throw ModelLoaderT::LoadErrorT("Anim.Frames.Size() > 0");

                Anim.Frames.PushBackEmpty(TP.GetNextTokenAsInt());

                for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                    GotFrames.PushBack(false);
            }
            else if (Token=="numJoints")
            {
                // The numJoints here MUST match the numJoints in the md5mesh file!
                const unsigned long numJoints=TP.GetNextTokenAsInt();

                if (numJoints!=Joints.Size()) { printf("%lu joints in md5anim file, %lu joints in the model.\n", numJoints, Joints.Size()); throw ModelLoaderT::LoadErrorT("The number of joints in the md5anim file does not match number of joints in the model."); }
                if (Anim.AnimJoints.Size()>0) { printf("Anim.AnimJoints.Size()==%lu\n", Anim.AnimJoints.Size()); throw ModelLoaderT::LoadErrorT("Anim.AnimJoints.Size() > 0"); }

                Anim.AnimJoints.PushBackEmpty(numJoints);
            }
            else if (Token=="numAnimatedComponents")
            {
                const unsigned long numAnimatedComponents=TP.GetNextTokenAsInt();

                // This is the number of components that is animated in the frames (and thus so many values are stored with each frame).
                // Therefore, the number of frames must have been specified already, so we can allocate all the memory here.
                if (Anim.Frames.Size()==0) throw ModelLoaderT::LoadErrorT("Anim.Frames.Size() == 0");

                for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                    Anim.Frames[FrameNr].AnimData.PushBackEmpty(numAnimatedComponents);
            }
            else if (Token=="hierarchy")
            {
                TP.AssertAndSkipToken("{");

                for (unsigned long JointNr=0; JointNr<Anim.AnimJoints.Size(); JointNr++)
                {
                    // Make sure that the name and parent are identical with the joint from the md5mesh file.
                    if (Joints[JointNr].Name  !=TP.GetNextToken()) throw ModelLoaderT::LoadErrorT("Mismatching joint name.");
                    if (Joints[JointNr].Parent!=TP.GetNextTokenAsInt()) throw ModelLoaderT::LoadErrorT("Mismatching joint parent.");

                    Anim.AnimJoints[JointNr].Flags       =TP.GetNextTokenAsInt();
                    Anim.AnimJoints[JointNr].FirstDataIdx=TP.GetNextTokenAsInt();
                }

                TP.AssertAndSkipToken("}");
                GotHierarchy=true;
            }
            else if (Token=="bounds")
            {
                TP.AssertAndSkipToken("{");

                for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                {
                    TP.AssertAndSkipToken("(");
                    Anim.Frames[FrameNr].BB.Min.x=TP.GetNextTokenAsFloat();
                    Anim.Frames[FrameNr].BB.Min.y=TP.GetNextTokenAsFloat();
                    Anim.Frames[FrameNr].BB.Min.z=TP.GetNextTokenAsFloat();
                    TP.AssertAndSkipToken(")");
                    TP.AssertAndSkipToken("(");
                    Anim.Frames[FrameNr].BB.Max.x=TP.GetNextTokenAsFloat();
                    Anim.Frames[FrameNr].BB.Max.y=TP.GetNextTokenAsFloat();
                    Anim.Frames[FrameNr].BB.Max.z=TP.GetNextTokenAsFloat();
                    TP.AssertAndSkipToken(")");
                }

                TP.AssertAndSkipToken("}");
                GotBounds=true;
            }
            else if (Token=="baseframe")
            {
                TP.AssertAndSkipToken("{");

                for (unsigned long JointNr=0; JointNr<Anim.AnimJoints.Size(); JointNr++)
                {
                    TP.AssertAndSkipToken("(");
                    Anim.AnimJoints[JointNr].DefaultPos.x=TP.GetNextTokenAsFloat();
                    Anim.AnimJoints[JointNr].DefaultPos.y=TP.GetNextTokenAsFloat();
                    Anim.AnimJoints[JointNr].DefaultPos.z=TP.GetNextTokenAsFloat();
                    TP.AssertAndSkipToken(")");
                    TP.AssertAndSkipToken("(");
                    Anim.AnimJoints[JointNr].DefaultQtr.x=TP.GetNextTokenAsFloat();
                    Anim.AnimJoints[JointNr].DefaultQtr.y=TP.GetNextTokenAsFloat();
                    Anim.AnimJoints[JointNr].DefaultQtr.z=TP.GetNextTokenAsFloat();
                    TP.AssertAndSkipToken(")");
                    Anim.AnimJoints[JointNr].DefaultScale=Vector3fT(1.0f, 1.0f, 1.0f);
                }

                TP.AssertAndSkipToken("}");
                GotBaseframe=true;
            }
            else if (Token=="frame")
            {
                const unsigned long FrameNr=TP.GetNextTokenAsInt();

                TP.AssertAndSkipToken("{");

                for (unsigned long ComponentNr=0; ComponentNr<Anim.Frames[FrameNr].AnimData.Size(); ComponentNr++)
                    Anim.Frames[FrameNr].AnimData[ComponentNr]=TP.GetNextTokenAsFloat();

                TP.AssertAndSkipToken("}");
                GotFrames[FrameNr]=true;
            }
            else
            {
                // Unknown token!
                // If the next token is a block start, skip the block.
                // Otherwise it was something else that we just throw away.
                printf("Unknown token \"%s\", skipping...\n", Token.c_str());
                if (TP.GetNextToken()=="{") TP.SkipBlock("{", "}", true);
            }
        }


        // Make sure that the imported animation sequence is valid.
        if (Anim.AnimJoints.Size()!=Joints.Size()) throw ModelLoaderT::LoadErrorT("The number of joints in the md5anim file does not match number of joints in the model.");
        if (!GotHierarchy) throw ModelLoaderT::LoadErrorT("There seems to be no joints hierarchy defined in this file.");
        if (!GotBounds) throw ModelLoaderT::LoadErrorT("There seem to be no bounds in this file.");
        if (!GotBaseframe) throw ModelLoaderT::LoadErrorT("There seems to be no baseframe data in this file.");
        if (Anim.Frames.Size()==0) throw ModelLoaderT::LoadErrorT("The animation sequence in this file has no frames.");

        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
            if (!GotFrames[FrameNr])
                throw ModelLoaderT::LoadErrorT("Some frames in the sequence are not defined.");


        // The animation seems ok, return it.
        ArrayT<CafuModelT::AnimT> Anims;
        Anims.PushBack(Anim);
        return Anims;
    }
    catch (const TextParserT::ParseError&)
    {
        printf("WARNING: Loading animation sequence file %s failed just before input byte %lu!\n", m_FileName.c_str(), TP.GetReadPosByte());

        throw ModelLoaderT::LoadErrorT("Could not parse the file.");
    }
}
