/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_CREATE_ARCH_HPP_INCLUDED
#define CAFU_DIALOG_CREATE_ARCH_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "Math3D/BoundingBox.hpp"
#include "wx/wx.h"


class AxesInfoT;
class EditorMaterialI;
class MapPrimitiveT;
class wxSpinCtrl;
class wxSpinEvent;


class ArchDialogT : public wxDialog
{
    public:

    ArchDialogT(const BoundingBox3fT& BB, const AxesInfoT& AxesInfo, wxWindow* Parent=NULL);

    ArrayT<MapPrimitiveT*> GetArch(EditorMaterialI* Material) const;


    private:

    class PreviewWinT;

    void OnSpinEvent(wxSpinEvent& event);

    const BoundingBox3fT m_BB;
    int                  m_WallWidth;
    int                  m_NrOfSegments;
    int                  m_Arc;
    int                  m_StartAngle;
    int                  m_AddHeight;
    PreviewWinT*         m_PreviewWin;

    DECLARE_EVENT_TABLE()
};

#endif
