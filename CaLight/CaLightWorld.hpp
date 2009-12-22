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

/******************************/
/*** CaLight World (Header) ***/
/******************************/

#ifndef _CALIGHTWORLD_HPP_
#define _CALIGHTWORLD_HPP_

#include "../Common/World.hpp"


class CaLightWorldT
{
    public:

    CaLightWorldT(const char* FileName);

    const cf::SceneGraph::BspTreeNodeT& GetBspTree() const { return *World.BspTree; }
    const ArrayT<PointLightT>&          GetPointLights() const { return World.PointLights; }

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
    void CreateLightMapsForEnts();

    // Forwarded functions.
    void SaveToDisk(const char* FileName) const;


    private:

    WorldT World;
};

#endif
