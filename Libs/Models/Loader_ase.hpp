/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_ASE_MODEL_LOADER_HPP_INCLUDED
#define CAFU_ASE_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


class TextParserT;


/// This class imports an ASE (ASCII Scene Exporter, 3dsmax) model file into a new Cafu model.
class LoaderAseT : public ModelLoaderT
{
    public:

    /// The constructor for importing an ASE (ASCII Scene Exporter, 3dsmax) model file into a new Cafu model.
    /// @param FileName   The name of the .ase file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderAseT(const std::string& FileName, int Flags=NONE);

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan) { }
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }


    private:

    /// A geometric object.
    struct GeomObjectT
    {
        /// A single triangle.
        struct TriangleT
        {
            /// Constructor.
            TriangleT()
                : SmoothGrps(0)
            {
                for (unsigned long i=0; i<3; i++)
                {
                    IndVertices [i]=0;
                    IndTexCoords[i]=0;
                }
            }

            // This data is read from the file.
            unsigned long         IndVertices [3];  ///< Indices into the Vertices array.
            unsigned long         IndTexCoords[3];  ///< Indices into the TexCoords array.
            ArrayT<unsigned long> SmoothGroups;     ///< The SmoothGroups this triangle is in.
            uint32_t              SmoothGrps;       ///< The smoothing groups that this triangle is in: If bit \c i is set, the triangle is in smoothing group \c i.

            // This data is computed after loading.
            VectorT               Normal;           ///< The geometric per-triangle normal.
            VectorT               Normals[3];       ///< The smoothgroup normals   for the three vertices.
            VectorT               Tangents[3];      ///< The smoothgroup tangents  for the three vertices.
            VectorT               BiNormals[3];     ///< The smoothgroup binormals for the three vertices.
        };


        /// Method for initializing the mesh from (a file within) TP.
        /// @param TP TextParserT used to read the mesh in.
        void ReadMesh(TextParserT& TP);

        // This data is read from the file.
        std::string       Name;           ///< Name of the object.
        ArrayT<VectorT>   Vertices;       ///< List of vertices.
        ArrayT<VectorT>   TexCoords;      ///< List of texture coordinates.
        ArrayT<TriangleT> Triangles;      ///< List of triangles.
        unsigned long     IndexMaterial;  ///< Index of the material used for this object.
        bool              CastShadows;    ///< Whether this object casts shadows.
    };


    void ReadMaterials(TextParserT& TP);
    void ReadGeometry(TextParserT& TP);
    void Print() const;

    ArrayT<std::string> m_MaterialNames;
    ArrayT<GeomObjectT> m_GeomObjects;
};

#endif
