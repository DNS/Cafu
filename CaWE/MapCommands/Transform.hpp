/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _COMMAND_TRANSFORM_HPP_
#define _COMMAND_TRANSFORM_HPP_

#include "../CommandPattern.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/Vector3.hpp"


class CommandSelectT;
class MapDocumentT;
class MapElementT;


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
    CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, TransModeT Mode, const Vector3fT& RefPoint, const Vector3fT& Amount, bool DoClone);
    CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, const MatrixT& Matrix, bool DoClone);

    /// Returns the array of cloned elements.
    /// This method is used and called by the ToolSelectionT class.
    const ArrayT<MapElementT*>& GetClones() const { return m_ClonedElems; }

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
    ArrayT<MapElementT*>       m_OldStates;     ///< The old states of the map elements, for the Undo() operation.
    ArrayT<MapElementT*>       m_ClonedElems;   ///< Local references (pointers) to the cloned elements.
    const TransModeT           m_Mode;          ///< The kind of transformation to apply to the m_TransElems.
    const Vector3fT            m_RefPoint;      ///< Ignored for translations, the reference point for scales and rotations.
    const Vector3fT            m_Amount;        ///< The delta for translations, the scale factors for scales, the angles for rotations.
    const MatrixT              m_Matrix;        ///< The matrix for generic matrix transformations.
    const bool                 m_DoClone;       ///< When false, the transformation is applied directly to the m_TransElems. When true, the m_TransElems are cloned and the transformation is applied to the clones.
    CommandSelectT*            m_CommandSelect; ///< Subcommand used for changing the selection (after map elements have been cloned).
};

#endif
