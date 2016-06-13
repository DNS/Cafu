/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_GROUP_SET_PROPERTY_HPP_INCLUDED
#define CAFU_COMMAND_GROUP_SET_PROPERTY_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class GroupT;
class MapDocumentT;


/// This class implements a command for setting the properties (other than the visibility) of a group.
class CommandGroupSetPropT : public CommandT
{
    public:

    enum PropT { PROP_NAME, PROP_COLOR, PROP_CANSELECT, PROP_SELECTASGROUP };

    /// The constructor for setting a new name.
    /// @param MapDoc    The map document the group is in.
    /// @param Group     The group whose property is set.
    /// @param NewName   The new name for the group.
    CommandGroupSetPropT(MapDocumentT& MapDoc, GroupT* Group, const wxString& NewName);

    /// The constructor for setting a new color.
    /// @param MapDoc     The map document the group is in.
    /// @param Group      The group whose property is set.
    /// @param NewColor   The new color for the group. If Group->IsVisible is already NewVis, Do() will fail.
    CommandGroupSetPropT(MapDocumentT& MapDoc, GroupT* Group, const wxColor& NewColor);

    /// The constructor for setting a new value for CanSelect or SelectAsGroup.
    /// @param MapDoc    The map document the group is in.
    /// @param Group     The group whose property is set.
    /// @param Prop      The boolean property that is newly set. Must be either PROP_CANSELECT or PROP_SELECTASGROUP.
    /// @param NewFlag   The new value for the above specified boolean properly.
    CommandGroupSetPropT(MapDocumentT& MapDoc, GroupT* Group, PropT Prop, bool NewFlag);

    /// Returns the group whose property is set.
    const GroupT* GetGroup() const { return m_Group; }

    /// Returns which property is set.
    PropT GetProp() const { return m_Prop; }

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT& m_MapDoc;
    GroupT*       m_Group;
    const PropT   m_Prop;

    const wxString m_OldName;
    const wxColor  m_OldColor;
    const bool     m_OldFlag;

    const wxString m_NewName;
    const wxColor  m_NewColor;
    const bool     m_NewFlag;
};

#endif
