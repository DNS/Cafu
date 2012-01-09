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

#ifndef _FILTER_SETTINGS_HPP_
#define _FILTER_SETTINGS_HPP_

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
