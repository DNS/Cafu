/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_APPLY_MATERIAL_HPP_INCLUDED
#define CAFU_COMMAND_APPLY_MATERIAL_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MapBrushT;
class MapBezierPatchT;
class MapDocumentT;
class MapElementT;
class MapTerrainT;
class EditorMaterialI;


class CommandApplyMaterialT : public CommandT
{
    public:

    /// Constructor to apply the given material to an array of objects.
    CommandApplyMaterialT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Objects, EditorMaterialI* Material);

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                      m_MapDoc;
    EditorMaterialI*                   m_Material;
    ArrayT<MapElementT*>               m_Objects;

    ArrayT<MapBrushT*>                 m_Brushes;
    ArrayT<MapBezierPatchT*>           m_BezierPatches;
    ArrayT<MapTerrainT*>               m_Terrains;

    ArrayT< ArrayT<EditorMaterialI*> > m_OldBrushMats;
    ArrayT<EditorMaterialI*>           m_OldBezierPatchMats;
    ArrayT<EditorMaterialI*>           m_OldTerrainMats;
};

#endif
