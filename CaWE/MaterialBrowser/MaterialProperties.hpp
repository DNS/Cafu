/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATERIAL_PROPERTIES_HPP_INCLUDED
#define CAFU_MATERIAL_PROPERTIES_HPP_INCLUDED

#include "wx/wx.h"
#include "wx/propgrid/propgrid.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"


class EditorMaterialI;


namespace MaterialBrowser
{
    class DialogT;


    class MaterialPropertiesT : public wxPropertyGridManager
    {
        public:

        MaterialPropertiesT(DialogT* Parent);

        void ShowMaterial(EditorMaterialI* EditorMaterial);


        private:

        void OnValueChanging(wxPropertyGridEvent& Event);

        DECLARE_EVENT_TABLE()
    };
}

#endif
