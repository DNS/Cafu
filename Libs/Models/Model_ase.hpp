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

#ifndef _MODEL_ASE_HPP_
#define _MODEL_ASE_HPP_

#include "MaterialSystem/Mesh.hpp"
#include "Math3D/Vector3.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Templates/Array.hpp"
#include "Model.hpp"


class MaterialT;
namespace MatSys { class RenderMaterialT; }
class TextParserT;


/// This class implements an ASE (ASCII Scene Exporter, 3dsmax) model.
class ModelAseT : public ModelT
{
    public:

    /// The constructor.
    ModelAseT(const std::string& FileName_) /*throw (ModelT::LoadError)*/;

    /// The destructor.
    ~ModelAseT();

    // The ModelT interface.
    const std::string& GetFileName() const;
    void               Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel=NULL) const;
    bool               GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;
    void               Print() const;
    int                GetNrOfSequences() const;
    const float*       GetSequenceBB(int SequenceNr, float FrameNr) const;
 // float              GetNrOfFrames(int SequenceNr) const;
    float              AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop=true) const;


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
            unsigned long         IndVertices [3];      ///< Indices into the Vertices array.
            unsigned long         IndTexCoords[3];      ///< Indices into the TexCoords array.
            ArrayT<unsigned long> SmoothGroups;         ///< The SmoothGroups this triangle is in.

            // This data is computed after loading.
            VectorT               Normal;               ///< The geometric per-triangle normal.
            VectorT               Normals   [3];        ///< The smoothgroup normals   for the three vertices.
            VectorT               Tangents  [3];        ///< The smoothgroup tangents  for the three vertices.
            VectorT               BiNormals [3];        ///< The smoothgroup binormals for the three vertices.
            int                   Neighbours[3];        ///< The array indices of the three neighbouring triangles at the edges 01, 12 and 20. -1 indicates no neighbour, -2 indicates more than one neighbour.
        };

        // This data is read from the file.
        ArrayT<VectorT>   Vertices;       ///< List of vertices.
        ArrayT<VectorT>   TexCoords;      ///< List of texture coordinates.
        ArrayT<TriangleT> Triangles;      ///< List of triangles.
        unsigned long     IndexMaterial;  ///< Index of the material used for this object.
        bool              CastShadows;    ///< Whether this object casts shadows.

        // This data is computed after loading.
        MatSys::MeshT         TriangleMesh; ///< Triangle mesh representing this object.
        BoundingBox3T<double> BB;           ///< The objects bounding box.


        /// Method for initializing the mesh from (a file within) TP.
        /// @param TP TextParserT used to read the mesh in.
        void ReadMesh(TextParserT& TP);
    };

    const std::string                FileName;
    ArrayT<std::string>              MaterialNames;
    ArrayT<MaterialT*>               Materials;
    ArrayT<MatSys::RenderMaterialT*> RenderMaterials;
    ArrayT<GeomObjectT>              GeomObjects;
    ArrayT<unsigned long>            GOIndicesSortedByMat;

    int       Gui_GeomObjNr;    ///< The index into GeomObjects of the GeomObjectT that has the "Textures/meta/EntityGUI" material and at least one triangle, -1 for none.
    Vector3fT Gui_Origin;
    Vector3fT Gui_AxisX;
    Vector3fT Gui_AxisY;

    // Helper functions for the constructor.
    void ReadMaterials(TextParserT& TP);
    void ReadGeometry(TextParserT& TP);
};

#endif
