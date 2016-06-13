/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_OPTIONS_HPP_INCLUDED
#define CAFU_OPTIONS_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "wx/wx.h"


class GameConfigT;


class OptionsT
{
    public:

    struct GeneralT
    {
        int      UndoLevels;
        bool     LockingTextures;
        bool     NewUVsFaceAligned;     ///< Whether the texture-space axes of newly created brush faces are initialized "face aligned" or "world aligned".

        wxString EngineExe;
        wxString BSPExe;
        wxString LightExe;
        wxString PVSExe;
    };

    struct View2DT
    {
        bool DrawVertices;
        bool SelectByHandles;
        bool ShowEntityInfo;
        bool ShowEntityTargets;
        bool UseGroupColors;
    };

    struct View3DT
    {
        bool  ReverseY;                 ///< Whether to reverse the mouse's Y axis when mouse looking.
        int   BackPlane;                ///< Distance to far clipping plane in world units.
        int   ModelDistance;            ///< Distance in world units within which models render.
        bool  AnimateModels;            ///< Whether or not to animate models.
        int   MaxCameraVelocity;        ///< Max forward speed in world units per second.
        int   TimeToMaxSpeed;           ///< Time to max forward speed in milliseconds.
        float MouseSensitivity;         ///< Mouse sensitivity for the angular (rotating and orbiting) modes of 3D views navigation, in degrees (of rotation) per pixel (of mouse movement).
        int   SplitPlanesDepth;         ///< The depth up to which the split planes of the BSP tree should be rendered (for debugging/developers only).
    };

    struct GridT
    {
        int      InitialSpacing;        ///< The default spacing between grid lines, in world units (usually 4, 8 or 16).
        int      MinPixelSpacing;       ///< The minimum pixel spacing between grid lines. The grid is hidden if it gets smaller than this.
        bool     UseDottedGrid;         ///< When true, the grid is rendered with dots. Otherwise, it is rendered with lines.
        bool     ShowHighlight1;        ///< Whether every n-th world unit should be highlighted.
        int      SpacingHighlight1;     ///< The spacing number n that should be highlighted.
        bool     ShowHighlight2;        ///< Whether every m-th world unit should be highlighted.
        int      SpacingHighlight2;     ///< The spacing number m that should be highlighted.

        wxColour ColorBackground;       ///< The background color.
        wxColour ColorBaseGrid;         ///< The base grid color.
        wxColour ColorHighlight1;       ///< The color for highlighting every n-th world unit, if enabled.
        wxColour ColorHighlight2;       ///< The color for highlighting every m-th world unit, if enabled.
        wxColour ColorAxes;             ///< The color for the major axes through the origin.
    };

    struct ColorsT
    {
        wxColour Selection;             ///< The color of selected objects.
        wxColour SelectedFace;          ///< The color of a selected face.
        wxColour SelectedEdge;          ///< The color of a selected edge.
        wxColour Vertex;                ///< The color of vertices.
        wxColour ToolHandle;            ///< The color of tool handles.
        wxColour ToolSelection;         ///< The color of the selection tool.
        wxColour ToolMorph;             ///< The color of the morph tool.
        wxColour ToolDrag;              ///< The color of tool bounds while it is being dragged.
    };


    ~OptionsT();

    void Init();
    void Write() const;
    void DeleteGameConfigs();

    GeneralT             general;
    View2DT              view2d;
    View3DT              view3d;
    GridT                Grid;
    ColorsT              colors;
    ArrayT<GameConfigT*> GameConfigs;
    unsigned long        DaysSinceInstalled;
};


extern OptionsT Options;

#endif
