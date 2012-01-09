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

#include "ArtProvider.hpp"
#include "wx/dir.h"
#include "wx/image.h"
#include "wx/log.h"

#include <algorithm>


namespace
{
    // This functor is used for sorting wxSizes by distance to the reference size.
    class CompareSizesT
    {
        public:

        CompareSizesT(const wxSize& Ref) : m_Ref(Ref) { }

        bool operator () (const wxSize& s1, const wxSize& s2) const
        {
            const wxSize d1=s1-m_Ref;
            const wxSize d2=s2-m_Ref;

            return d1.x*d1.x + d1.y*d1.y <
                   d2.x*d2.x + d2.y*d2.y;
        }


        private:

        const wxSize& m_Ref;
    };
}


ArtProviderT::ArtProviderT(const wxString& Theme)
    : m_Theme(Theme),
      m_Sizes()
{
    // TODO: Traverse the "res/icons/$theme" directory in order to discover the available sizes.
    m_Sizes.push_back(wxSize(16, 16));
    m_Sizes.push_back(wxSize(22, 22));
    m_Sizes.push_back(wxSize(24, 24));
    m_Sizes.push_back(wxSize(32, 32));
}


wxBitmap ArtProviderT::CreateBitmap(const wxArtID& ID, const wxArtClient& Client, const wxSize& Size)
{
    wxSize S=Size;

    if (!S.IsFullySpecified())
    {
        S=wxArtProvider::GetNativeSizeHint(Client);

        if (!S.IsFullySpecified())
        {
            if (Client==wxART_MENU || Client==wxART_BUTTON)
            {
                S=wxSize(16, 16);
            }
            else if (Client==wxART_TOOLBAR)
            {
                S=wxSize(22, 22);
            }
            else
            {
                S=wxSize(32, 32);
            }
        }
    }


    // Sort the available sizes by distance to S.
    sort(m_Sizes.begin(), m_Sizes.end(), CompareSizesT(S));


    // Translate some wxART IDs to the Standard Icon Naming Specification,
    // http://tango.freedesktop.org/Standard_Icon_Naming_Specification
    wxString Name=ID;

         if (Name==wxART_NEW)          Name="document-new";
    else if (Name==wxART_FILE_OPEN)    Name="document-open";
    else if (Name==wxART_FILE_SAVE)    Name="document-save";
    else if (Name==wxART_FILE_SAVE_AS) Name="document-save-as";
    else if (Name==wxART_UNDO)         Name="edit-undo";
    else if (Name==wxART_REDO)         Name="edit-redo";
    else if (Name==wxART_CUT)          Name="edit-cut";
    else if (Name==wxART_COPY)         Name="edit-copy";
    else if (Name==wxART_PASTE)        Name="edit-paste";
    else if (Name==wxART_DELETE)       Name="edit-delete";


    // Iterate through the available bitmap sizes in the order of best size match.
    for (size_t i=0; i<m_Sizes.size(); i++)
    {
        const wxString ThemeSizeStr(wxString::Format("CaWE/res/icons/%s/%ix%i", m_Theme, m_Sizes[i].x, m_Sizes[i].y));
        wxDir          ThemeSizeDir(ThemeSizeStr);
        wxString       Context;

        // Iterate through all "context" subdirectories in "CaWE/res/icons/$theme/$size/".
        for (bool more=ThemeSizeDir.GetFirst(&Context, "", wxDIR_DIRS); more; more=ThemeSizeDir.GetNext(&Context))
        {
            const wxString FileName(ThemeSizeStr + wxString::Format("/%s/%s.png", Context, Name));

            if (!wxFileExists(FileName))
                continue;

            wxBitmap Bitmap(FileName, wxBITMAP_TYPE_PNG);

            if (Bitmap.IsOk())
            {
                if (Bitmap.GetSize().x==22 && S.x==24)
                {
                    wxImage img=Bitmap.ConvertToImage();

                    img.Resize(wxSize(24, Bitmap.GetSize().y), wxPoint(1, 0));
                    Bitmap=wxBitmap(img);
                }

                if (Bitmap.GetSize().y==22 && S.y==24)
                {
                    wxImage img=Bitmap.ConvertToImage();

                    img.Resize(wxSize(Bitmap.GetSize().x, 24), wxPoint(0, 1));
                    Bitmap=wxBitmap(img);
                }

                if (Bitmap.GetSize()!=S)
                {
                    wxImage img=Bitmap.ConvertToImage();

                    wxLogWarning("%s(%u): Scaling bitmap from %ix%i to %ix%i.", __FILE__, __LINE__, Bitmap.GetSize().x, Bitmap.GetSize().y, S.x, S.y);
                    img.Rescale(S.x, S.y);
                    Bitmap=wxBitmap(img);
                }

                return Bitmap;
            }
        }
    }

    return wxNullBitmap;
}
