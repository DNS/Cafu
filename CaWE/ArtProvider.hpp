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

#ifndef _CAWE_ART_PROVIDER_HPP_
#define _CAWE_ART_PROVIDER_HPP_

#include "wx/artprov.h"
#include <vector>


/// This class provides our application with custom bitmaps for menus, toolbars, dialogs, etc.
/// One or several instances are plugged into the chain of wxWidgets art providers in order to
/// complement the native or built-in art providers and to provide support for user themes.
///
/// The bitmaps in this ArtProviderT are all from a common theme.
/// The implementation, the directory structure and the file names follow the freedesktop.org
/// "Icon Naming Specification" and the "Icon Theme Specification". For details, see:
/// http://tango.freedesktop.org/Standard_Icon_Naming_Specification
class ArtProviderT : public wxArtProvider
{
    public:

    ArtProviderT(const wxString& Theme);


    protected:

    wxBitmap CreateBitmap(const wxArtID& ID, const wxArtClient& Client, const wxSize& Size);

    const wxString      m_Theme;
    std::vector<wxSize> m_Sizes;
};

#endif
