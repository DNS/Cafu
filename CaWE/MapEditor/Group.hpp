/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GROUP_HPP_INCLUDED
#define CAFU_GROUP_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "wx/wx.h"


class MapDocumentT;
class MapElementT;
class TextParserT;


/// This class represents groups.
class GroupT
{
    public:

    /// The constructor.
    GroupT(const MapDocumentT& MapDoc, const wxString& Name_);

    /// The constructor for loading a group from a cmap file.
    GroupT(const MapDocumentT& MapDoc, TextParserT& TP);

    void Save_cmap(std::ostream& OutFile, unsigned long GroupNr) const;

    ArrayT<MapElementT*> GetMembers() const;    ///< Returns all map elements in this group.
    bool                 HasMembers() const;    ///< Returns whether this group has members (like `GetMembers().Size() > 0`, but more efficiently).

    wxString Name;
    wxColour Color;
    bool     IsVisible;     ///< Are the members of this group visible in the (2D and 3D) views?
    bool     CanSelect;     ///< Can the members of this group be selected in the (2D and 3D) views? When false, the members are considered "locked" in order to prevent their editing.
    bool     SelectAsGroup; ///< Are the members of this group selected "as a group" (normal group behaviour) or can they be selected individually?


    private:

    const MapDocumentT& m_MapDoc;
};

#endif
