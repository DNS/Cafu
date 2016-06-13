/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
    CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, TransModeT Mode, const Vector3fT& RefPoint, const Vector3fT& Amount, bool LockTexCoords);
    CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, const MatrixT& Matrix, bool LockTexCoords);

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
    const bool                 m_LockTexCoords; ///< Transform the texture-space along with the geometry?
};

#endif
