/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_CALIGHTWORLD_HPP_INCLUDED
#define CAFU_CALIGHTWORLD_HPP_INCLUDED

#include "../Common/World.hpp"
#include "GameSys/Entity.hpp"


class CaLightWorldT
{
    public:

    CaLightWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes);

    const cf::SceneGraph::BspTreeNodeT& GetBspTree() const { return *m_BspTree; }

    double TraceRay(const Vector3dT& Start, const Vector3dT& Ray) const;

    /// Creates (fake) lightmaps for (brush or bezier patch based) entities.
    /// The generated lightmaps are only "fakes", i.e. they are not a result of the true Radiosity computations of CaLight.
    /// Instead, they are obtained by sampling the environment (the world entity) and interpolating the obtained values.
    /// This of course is only possible after the world has been lit by the true Radiosity process.
    /// While the disadvantages are clear (objects that have not participated in the original Radiosity computations can
    /// obviously not blend into the environment visually as smoothly as if they had been there right from the start),
    /// the advantages are a reasonably quick completion of the computations and independency of the internal data of the
    /// original Radiosity computations.
    /// For example, this function can be implemented without reference to the "bins" of the tone-mapping operator of the
    /// world, which in turn makes the implementation of the "-onlyEnts" command-line option primally possible.
    void CreateLightMapsForEnts(const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& AllEnts);

    // Forwarded functions.
    void SaveToDisk(const char* FileName) const;


    private:

    WorldT                              m_World;
    const cf::SceneGraph::BspTreeNodeT* m_BspTree;
    cf::ClipSys::CollisionModelStaticT* m_CollModel;
};

#endif
