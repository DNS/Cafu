/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************/
/*** RenderMaterial ***/
/**********************/

#ifndef CAFU_MATSYS_RENDERMATERIAL_HPP_INCLUDED
#define CAFU_MATSYS_RENDERMATERIAL_HPP_INCLUDED

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
