/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FILTER_SETTINGS_HPP_INCLUDED
#define CAFU_FILTER_SETTINGS_HPP_INCLUDED

#include "wx/wx.h"


namespace MaterialBrowser
{
    class DialogT;


    class FilterSettingsT : public wxPanel
    {
        public:

        FilterSettingsT(DialogT* Parent);

        wxString GetNameFilterValue() const;
        void     SetNameFilterValue(const wxString& s);

        bool OnlyShowUsed() const       { return m_OnlyShowUsedCheckbox && m_OnlyShowUsedCheckbox->IsChecked(); }
        bool OnlyShowEditorMats() const { return m_OnlyShowEditorMaterials && m_OnlyShowEditorMaterials->IsChecked(); }


        private:

        DialogT*    m_Parent;
        wxComboBox* m_NameFilterCombobox;
        wxCheckBox* m_OnlyShowUsedCheckbox;
        wxCheckBox* m_OnlyShowEditorMaterials;
    };
}

#endif
