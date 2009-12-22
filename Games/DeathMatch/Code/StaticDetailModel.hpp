/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

/************************************/
/*** Static Detail Model (Header) ***/
/************************************/

#ifndef _STATIC_DETAIL_MODEL_HPP_
#define _STATIC_DETAIL_MODEL_HPP_

#include "../../BaseEntity.hpp"
#include "Models/Model_proxy.hpp"


class EntityCreateParamsT;
namespace cf { namespace GuiSys { class GuiI; } }


class EntStaticDetailModelT : public BaseEntityT
{
    public:

    EntStaticDetailModelT(const EntityCreateParamsT& Params);
    ~EntStaticDetailModelT();

    void Think(float FrameTime, unsigned long ServerFrameNr);

    void Cl_UnserializeFrom();
    void Draw(bool FirstPersonView, float LodDist) const;
    void PostDraw(float FrameTime, bool FirstPersonView);

    /// Returns the GUI of this entity, or NULL if there is none.
    cf::GuiSys::GuiI* GetGUI() const;

    /// If this entity has a panel for an associated GUI, this method returns its related current plane in world space.
    /// @param GuiOrigin    The origin (Stützvektor)     of the GUI plane in world space is returned in this parameter.
    /// @param GuiAxisX     The x-axis (Richtungsvektor) of the GUI plane in world space is returned in this parameter.
    /// @param GuiAxisY     The y-axis (Richtungsvektor) of the GUI plane in world space is returned in this parameter.
    /// @returns whether the call was successful, i.e. whether this model has a GUI panel at all and the GuiOrigin,
    ///     GuiAxisX and GuiAxisY parameters were filled-out properly.
    bool GetGuiPlane(Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    ModelProxyT       Model;
    int               ModelSequenceNr;
    float             ModelFrameNr;
    std::string       GuiName;  ///< If our "gui" entity key is set, store the value here.
    cf::GuiSys::GuiI* Gui;      ///< If the Model can display a GUI (it has a "Textures/meta/EntityGUI" surface), we load it here, *both* on the server- as well as on the client-side.
};

#endif
