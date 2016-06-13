/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************/
/*** Shader ***/
/**************/

#include "Shader.hpp"
#include "Templates/Array.hpp"


// We cannot simply have an "extern ArrayT<ShaderT*> ShaderRepository;" in the Shader.hpp file,
// and an "ArrayT<ShaderT*> ShaderRepository;" in here, because in combination with the static shader objects,
// this might lead to the "static initialization order fiasco" - see the "C++ FAQs", pp. 231.
ArrayT<ShaderT*>& GetShaderRepository()
{
    // This prevents the "static initialization order fiasco".
    // As abandoning the ShaderRepository on program exit on the heap is fine,
    // I don't risk the static DEinitialization order problem and use a safe pointer instead. See C++ FAQ 16.18.
    static ArrayT<ShaderT*>* ShaderRepository=new ArrayT<ShaderT*>();

    return *ShaderRepository;
}


ShaderT* GetStencilShadowVolumesShader()
{
    static ShaderT* StencilShadowVolumesShader=NULL;
    static bool     HaveSearched=false;

    if (!HaveSearched)
    {
        for (unsigned long ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
            if (GetShaderRepository()[ShaderNr]->CanHandleStencilShadowVolumes())
                StencilShadowVolumesShader=GetShaderRepository()[ShaderNr];

        HaveSearched=true;
    }

    return StencilShadowVolumesShader;
}


/**************/
/*** Shader ***/
/**************/

// The shader constructor.
// Registers itself with the shader repository.
ShaderT::ShaderT()
{
    // If ShaderRepository was a global object, it might not yet be initialized when we get here!
    // Thus, the GetShaderRepository() solves the problem.
    GetShaderRepository().PushBack(this);

    // Note that it is NOT possible to call CanHandleStencilShadowVolumes() here!
    // (Thus there is NO way to abbreviate the search in GetStencilShadowVolumesShader().)
    // See C++ FAQ 20.13 on page 283 for an explanation.
}
