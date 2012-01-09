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

#ifndef _COMMAND_UPDATE_SURFACE_HPP_
#define _COMMAND_UPDATE_SURFACE_HPP_

#include "../CommandPattern.hpp"
#include "../SurfaceInfo.hpp"


class MapBrushT;
class MapBezierPatchT;
class MapTerrainT;
class MapDocumentT;
class EditorMaterialI;


class CommandUpdateSurfaceT : public CommandT
{
    public:

    CommandUpdateSurfaceT(MapDocumentT& MapDoc_,
                          const SurfaceInfoT& SurfaceInfoNew_, const SurfaceInfoT& SurfaceInfoOld_,
                          EditorMaterialI* MaterialNew_, EditorMaterialI* MaterialOld_);


    protected:

    MapDocumentT&      MapDoc;
    const SurfaceInfoT SurfaceInfoNew; ///< The surface info that is applied to the surface.
    const SurfaceInfoT SurfaceInfoOld; ///< The surfaces former surface info for undo.
    EditorMaterialI*   MaterialNew;    ///< The material that is applied to the surface.
    EditorMaterialI*   MaterialOld;    ///< The surfaces former material for undo.
};


class CommandUpdateSurfaceFaceT : public CommandUpdateSurfaceT
{
    public:

    CommandUpdateSurfaceFaceT(MapDocumentT& MapDoc_, MapBrushT* Brush_, unsigned long FaceIndex_, const SurfaceInfoT& SurfaceInfoNew_, EditorMaterialI* MaterialNew_);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapBrushT*          Brush;
    const unsigned long FaceIndex;
};


class CommandUpdateSurfaceBezierPatchT : public CommandUpdateSurfaceT
{
    public:

    CommandUpdateSurfaceBezierPatchT(MapDocumentT& MapDoc_, MapBezierPatchT* BezierPatch_, const SurfaceInfoT& SurfaceInfoNew_, EditorMaterialI* MaterialNew_);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapBezierPatchT* BezierPatch;
};


class CommandUpdateSurfaceTerrainT : public CommandUpdateSurfaceT
{
    public:

    CommandUpdateSurfaceTerrainT(MapDocumentT& MapDoc_, MapTerrainT* Terrain_, EditorMaterialI* MaterialNew_);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapTerrainT* Terrain;
};

#endif
