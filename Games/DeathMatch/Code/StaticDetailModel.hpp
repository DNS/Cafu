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

#ifndef CAFU_STATIC_DETAIL_MODEL_HPP_INCLUDED
#define CAFU_STATIC_DETAIL_MODEL_HPP_INCLUDED

#include "../../BaseEntity.hpp"
#include "Models/AnimExpr.hpp"


class CafuModelT;
class EntityCreateParamsT;
namespace cf { namespace GuiSys { class GuiI; } }
struct luaL_Reg;


/// This class represents a static detail model.
/// A static detail model adds geometric detail to a map and can optionally hold a scripted GUI that the user can interact with.
/// Despite its name, a static detail model can run animated sequences, but note that these animations are essentially a client-side
/// effect only, as only a <em>restart</em> of a sequence is sync'ed over the network.
class EntStaticDetailModelT : public BaseEntityT
{
    public:

    EntStaticDetailModelT(const EntityCreateParamsT& Params);
    ~EntStaticDetailModelT();

    void Think(float FrameTime, unsigned long ServerFrameNr);

    void ProcessEvent(unsigned int EventType, unsigned int NumEvents);
    void Draw(bool FirstPersonView, float LodDist) const;
    void PostDraw(float FrameTime, bool FirstPersonView);

    /// Returns the GUI of this entity, or NULL if there is none.
    cf::GuiSys::GuiI* GetGUI() const;

    /// If this entity has a panel for an associated GUI, this method returns its related current plane in world space.
    /// @param GFNr         The number of the GUI fixture.
    /// @param GuiOrigin    The origin (Stützvektor)     of the GUI plane in world space is returned in this parameter.
    /// @param GuiAxisX     The x-axis (Richtungsvektor) of the GUI plane in world space is returned in this parameter.
    /// @param GuiAxisY     The y-axis (Richtungsvektor) of the GUI plane in world space is returned in this parameter.
    /// @returns whether the call was successful, i.e. whether this model has a GUI panel at all and the GuiOrigin,
    ///     GuiAxisX and GuiAxisY parameters were filled-out properly.
    bool GetGuiPlane(unsigned int GFNr, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    enum EventTypesT { EVENT_TYPE_RESTART_SEQU, NUM_EVENT_TYPES };

    // Override the base class methods.
    void DoSerialize(cf::Network::OutStreamT& Stream) const;
    void DoDeserialize(cf::Network::InStreamT& Stream);

    const CafuModelT* m_Model;
    bool              m_PlayAnim;   ///< If 1, play the animation, i.e. advance the frames over time. If 0, keep still.
    int32_t           m_SequNr;     ///< The number of the animation sequence to play.

    mutable IntrusivePtrT<AnimExpressionT>   m_AnimExpr;    ///< The state of the currently playing animation sequence. Used <em>independently</em> on the server and the clients; only a <em>restart</em> of a sequence is sync'ed over the network via the EventID_RestartSequ event.
    mutable IntrusivePtrT<AnimExprStandardT> m_LastStdAE;   ///< The most recent standard expression that we set (as a subexpression of m_AnimExpr).

    std::string       GuiName;      ///< If our "gui" entity key is set, store the value here.
    cf::GuiSys::GuiI* Gui;          ///< If the model has GUI fixtures, we load the GUI here, *both* on the server- as well as on the client-side.


    // Script methods (to be called from the map/entity Lua scripts).
    static int IsPlayingAnim(lua_State* LuaState);
    static int PlayAnim(lua_State* LuaState);
    static int GetSequNr(lua_State* LuaState);
    static int SetSequNr(lua_State* LuaState);
    static int RestartSequ(lua_State* LuaState);
    static int GetNumSequences(lua_State* LuaState);

    static const luaL_Reg MethodsList[];
};

#endif
