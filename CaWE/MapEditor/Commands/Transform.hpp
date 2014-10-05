/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_COMMAND_TRANSFORM_HPP_INCLUDED
#define CAFU_COMMAND_TRANSFORM_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/Vector3.hpp"


class MapDocumentT;
class MapElementT;
class TrafoMementoT;


/// Command to transform an object or a list of objects using a delta and a transform mode.
class CommandTransformT : public CommandT
{
    public:

    /// This enum describes the kind of transformation that is to be applied by a CommandTransformT.
    enum TransModeT
    {
        MODE_TRANSLATE,
        MODE_ROTATE,
        MODE_SCALE,
        MODE_MATRIX
    };


    /// The constructor to transform an array of map elements using a delta and a transform mode.
    CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, TransModeT Mode, const Vector3fT& RefPoint, const Vector3fT& Amount);
    CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, const MatrixT& Matrix);

    /// The destructor.
    ~CommandTransformT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    void Init();

    MapDocumentT&              m_MapDoc;        ///< The map document that is (whose elements are) modified.
    const ArrayT<MapElementT*> m_TransElems;    ///< The map elements (of m_MapDoc) that are to be transformed.
    ArrayT<TrafoMementoT*>     m_OldStates;     ///< The old states of the map elements, for the Undo() operation.
    const TransModeT           m_Mode;          ///< The kind of transformation to apply to the m_TransElems.
    const Vector3fT            m_RefPoint;      ///< Ignored for translations, the reference point for scales and rotations.
    const Vector3fT            m_Amount;        ///< The delta for translations, the scale factors for scales, the angles for rotations.
    const MatrixT              m_Matrix;        ///< The matrix for generic matrix transformations.
};

#endif
