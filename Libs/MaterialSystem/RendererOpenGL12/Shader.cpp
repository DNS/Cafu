/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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
