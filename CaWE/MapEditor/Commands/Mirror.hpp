/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_MIRROR_HPP_INCLUDED
#define CAFU_COMMAND_MIRROR_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MapDocumentT;
class MapElementT;
class TrafoMementoT;


/// Command to mirror map elements along a given mirror plane.
class CommandMirrorT : public CommandT
{
    public:

    /// The constructor.
    CommandMirrorT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, unsigned int NormalAxis, float Dist, bool LockTexCoords);

    /// Destructor.
    ~CommandMirrorT();

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&              m_MapDoc;
    const ArrayT<MapElementT*> m_MirrorElems;
    ArrayT<TrafoMementoT*>     m_OldStates;
    const unsigned int         m_NormalAxis;    ///< The number of the axis along which the normal vector of the mirror plane points: 0, 1 or 2 for the x-, y- or z-axis respectively.
    const float                m_Dist;          ///< The position of the mirror plane along its normal vector, where it intersects the NormalAxis.
    const bool                 m_LockTexCoords;
};

#endif
