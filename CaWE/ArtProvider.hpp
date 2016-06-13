/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CAWE_ART_PROVIDER_HPP_INCLUDED
#define CAFU_CAWE_ART_PROVIDER_HPP_INCLUDED

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
