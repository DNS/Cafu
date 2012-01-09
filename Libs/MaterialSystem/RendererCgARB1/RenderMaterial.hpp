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

/**********************/
/*** RenderMaterial ***/
/**********************/

#ifndef _CA_MATSYS_RENDERMATERIAL_HPP_
#define _CA_MATSYS_RENDERMATERIAL_HPP_

#include "../Material.hpp"


class ShaderT;
class TextureMap2DT;
class TextureMapCubeT;


namespace MatSys
{
    /// This class represents a surface render material.
    class RenderMaterialT
    {
        private:

        RenderMaterialT(const RenderMaterialT&);    // Use of the Copy    Constructor is not allowed.
        void operator = (const RenderMaterialT&);   // Use of the Assignment Operator is not allowed.


        public:

        RenderMaterialT(const MaterialT* Material_);
        ~RenderMaterialT();

        const MaterialT* Material;

        ShaderT*         AmbientShader;
        ShaderT*         LightShader;

        TextureMap2DT*   DiffTexMap;
        TextureMap2DT*   NormTexMap;
        TextureMap2DT*   SpecTexMap;
        TextureMap2DT*   LumaTexMap;
        TextureMap2DT*   LightTexMap;
        TextureMap2DT*   SHLTexMap;
        TextureMapCubeT* Cube1TexMap;
        TextureMapCubeT* Cube2TexMap;

        const bool       UseDefaultLightMap;
        const bool       UseDefaultSHLMap;
    };
}

#endif
