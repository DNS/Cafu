/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************/
/*** RenderMaterial ***/
/**********************/

#include "RenderMaterial.hpp"
#include "Shader.hpp"
#include "TextureMapImpl.hpp"


using namespace MatSys;


RenderMaterialT::RenderMaterialT(const MaterialT* Material_)
    : Material(Material_),
      AmbientShader(NULL),
      LightShader(NULL),
      DiffTexMap(NULL),
      NormTexMap(NULL),
      SpecTexMap(NULL),
      LumaTexMap(NULL),
      LightTexMap(NULL),
      SHLTexMap(NULL),
      Cube1TexMap(NULL),
      Cube2TexMap(NULL),
      UseDefaultLightMap(Material->LightMapComp.GetString()=="$lightmap"),
      UseDefaultSHLMap(Material->SHLMapComp.GetString()=="$shlmap")
{
    // Here comes one of the key concepts of the Cafu Material System:
    // Materials get shaders assigned, which are chosen from the repository of all available shaders of this renderer.
    // Those shaders that best fulfil the rendering of the material are selected.
    // For rendering, the combination of the material and its shader (ambient or light) work together ie. to setup the OpenGL state machine.

    // Determine the shader for the ambient contribution of this material (no light source involved).
    if (Material->AmbientShaderName!="")
    {
        unsigned long ShaderNr;

        for (ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
            if (Material->AmbientShaderName==GetShaderRepository()[ShaderNr]->GetName())
            {
                AmbientShader=GetShaderRepository()[ShaderNr];
                break;
            }

        // if (ShaderNr>=GetShaderRepository().Size()) Warning("Ambient user shader %s not found.\n", Material->AmbientShaderName);
    }

    if (AmbientShader==NULL)
    {
        // Either did the user not specify an explicit shader for the ambient contribution of the Material, or it was not found.
        ShaderT* BestShader     =NULL;
        char     BestShaderValue=0;

        for (unsigned long ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
        {
            char ThisShaderValue=GetShaderRepository()[ShaderNr]->CanHandleAmbient(*Material);

            if (ThisShaderValue>BestShaderValue)
            {
                BestShader     =GetShaderRepository()[ShaderNr];
                BestShaderValue=ThisShaderValue;
            }
        }

        AmbientShader=BestShader;
    }

    // if (AmbientShader==NULL) Error("No shader found for ambient contrib of Material %s.\n", Material->Name);


    // Determine shader for the per-light contribution of this material.
    if (Material->LightShaderName!="")
    {
        unsigned long ShaderNr;

        for (ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
            if (Material->LightShaderName==GetShaderRepository()[ShaderNr]->GetName())
            {
                LightShader=GetShaderRepository()[ShaderNr];
                break;
            }

        // if (ShaderNr>=GetShaderRepository().Size()) Warning("Per-light user shader %s not found.\n", Material->LightShaderName);
    }

    if (LightShader==NULL)
    {
        // Either did the user not specify an explicit shader for the per-lightsource contribution of the Material, or it was not found.
        ShaderT* BestShader     =NULL;
        char     BestShaderValue=0;

        for (unsigned long ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
        {
            char ThisShaderValue=GetShaderRepository()[ShaderNr]->CanHandleLighting(*Material);

            if (ThisShaderValue>BestShaderValue)
            {
                BestShader     =GetShaderRepository()[ShaderNr];
                BestShaderValue=ThisShaderValue;
            }
        }

        LightShader=BestShader;
    }

    // This is not really an error - OpenGL 1.2 may choose to not provide any light-dependent rendering at all.
    // if (LightShader==NULL) Error("No shader found for per-light contrib of Material %s.\n", Material->Name);


    DiffTexMap =TextureMapManagerImplT::Get().GetTextureMap2DInternal  (Material->DiffMapComp );
    NormTexMap =TextureMapManagerImplT::Get().GetTextureMap2DInternal  (Material->NormMapComp );
    SpecTexMap =TextureMapManagerImplT::Get().GetTextureMap2DInternal  (Material->SpecMapComp );
    LumaTexMap =TextureMapManagerImplT::Get().GetTextureMap2DInternal  (Material->LumaMapComp );
    LightTexMap=TextureMapManagerImplT::Get().GetTextureMap2DInternal  (Material->LightMapComp);
    SHLTexMap  =TextureMapManagerImplT::Get().GetTextureMap2DInternal  (Material->SHLMapComp  );
    Cube1TexMap=TextureMapManagerImplT::Get().GetTextureMapCubeInternal(Material->CubeMap1Comp);
    Cube2TexMap=TextureMapManagerImplT::Get().GetTextureMapCubeInternal(Material->CubeMap2Comp);
}


RenderMaterialT::~RenderMaterialT()
{
    TextureMapManagerImplT::Get().FreeTextureMap(DiffTexMap );
    TextureMapManagerImplT::Get().FreeTextureMap(NormTexMap );
    TextureMapManagerImplT::Get().FreeTextureMap(SpecTexMap );
    TextureMapManagerImplT::Get().FreeTextureMap(LumaTexMap );
    TextureMapManagerImplT::Get().FreeTextureMap(LightTexMap);
    TextureMapManagerImplT::Get().FreeTextureMap(SHLTexMap  );
    TextureMapManagerImplT::Get().FreeTextureMap(Cube1TexMap);
    TextureMapManagerImplT::Get().FreeTextureMap(Cube2TexMap);
}
