/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_UPDATE_SURFACE_HPP_INCLUDED
#define CAFU_COMMAND_UPDATE_SURFACE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
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
    EditorMaterialI*   MaterialOld;    ///< The surface's former material for undo.
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
