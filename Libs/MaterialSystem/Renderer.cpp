/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/****************/
/*** Renderer ***/
/****************/

#include "Renderer.hpp"


// A global pointer to the current renderer, for common access by all modules that use the MatSys.
MatSys::RendererI* MatSys::Renderer=0;
