/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _ASE_MODEL_LOADER_HPP_
#define _ASE_MODEL_LOADER_HPP_

#include "Loader.hpp"


class TextParserT;


/// This class imports an ASE (ASCII Scene Exporter, 3dsmax) model file into a new Cafu model.
class LoaderAseT : public ModelLoaderT
{
    public:

    /// The constructor for importing an ASE (ASCII Scene Exporter, 3dsmax) model file into a new Cafu model.
    /// @param FileName   The name of the .ase file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderAseT(const std::string& FileName, int Flags=NONE) /*throw (ModelT::LoadError)*/;

    bool UseGivenTS() const { return true; }
    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan) { }
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures, ArrayT<CafuModelT::GuiLocT>& GuiLocs);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }


    private:

    /// A geometric object.
    struct GeomObjectT
    {
        /// A single triangle.
        struct TriangleT
        {
            /// Constructor.
            TriangleT()
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
