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

#ifndef _ENTITY_CLASS_HPP_
#define _ENTITY_CLASS_HPP_

#include "Math3D/BoundingBox.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


class EntClassVarT;
class GameConfigT;
struct lua_State;


class HelperInfoT
{
    public:

    wxString         Name;
    ArrayT<wxString> Parameters;
};


class EntityClassT
{
    public:

    class InitErrorT { };


    /// The constructor. It creates a new entity class instance from the contents of the given Lua stack.
    /// At Lua stack index -2, the name of the entity class is provided.
    /// At Lua stack index -1, there is a table with the variables and meta-information for this class.
    /// @param GameConfig   The game configuration that this entity class is for.
    /// @param LuaState     The representation of the Lua stack from whose contents the EntityClassT instance is created.
    EntityClassT(const GameConfigT& GameConfig, lua_State* LuaState);

    /// The constructor for creating an entity class that is *unknown/undefined* in the game config.
    /// Instances of such entity classes are kept in the map document (vs. the game config).
    EntityClassT(const GameConfigT& GameConfig, const wxString& Name, bool HasOrigin);

    /// The destructor.
    ~EntityClassT();

    const GameConfigT& GetGameConfig()  const { return m_GameConfig; }
    bool               IsInGameConfig() const;      ///< Returns whether this class is defined/known in the game config. When false, this is an undefined class from importing a map file that was made for another game or game config.
    wxString           GetName()        const { return m_Name; }
    wxString           GetDescription() const { return m_Description=="" ? m_Name : m_Description; }  ///< Returns the description of this entity class.
    wxColour           GetColor()       const { return m_Color; }                                     ///< Returns the (render) color of this entity class.

    /// Returns the substitutional bounding-box for entities of this class.
    /// Usually used for entities that have no other representation (e.g. brushes or patches),
    /// that is, for "point entities", not for "solid entities".
    const BoundingBox3fT& GetBoundingBox() const { return m_BoundingBox; }

    bool IsSolidClass() const { return m_Solid; }

    const EntClassVarT* FindVar(const wxString& Name, unsigned long* Index=NULL) const;
    const ArrayT<const EntClassVarT*>& GetVariables() const { return m_Variables; }

    const ArrayT<const HelperInfoT*>& GetHelpers() const { return m_Helpers; }


    private:

    const GameConfigT&          m_GameConfig;   ///< The game config this entity class is (or should be!) defined in.
    wxString                    m_Name;         ///< Name of this class.
    wxString                    m_Description;  ///< Description of this class.
    wxColour                    m_Color;        ///< Color of entity.
    BoundingBox3fT              m_BoundingBox;  ///< Bounding box for representing entities of this class that have no other representation ("point entities").

    bool                        m_Solid;        ///< Tied to solids only.
    bool                        m_Point;        ///< Point class, not tied to solids.

    ArrayT<const EntClassVarT*> m_Variables;    ///< Variables for this class.
    ArrayT<const HelperInfoT*>  m_Helpers;      ///< Helpers for this class.
};

#endif
