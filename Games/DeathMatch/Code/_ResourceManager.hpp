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

/*******************************/
/*** Global Resource Manager ***/
/*******************************/

#ifndef CAFU_RESOURCE_MANAGER_HPP_INCLUDED
#define CAFU_RESOURCE_MANAGER_HPP_INCLUDED

#include "Templates/Array.hpp"


namespace MatSys { class RenderMaterialT; }


// Nothing prevents you from allocating, managing and deleting your own resources as member variables of an entity.
// Nothing is really wrong with that, and the MatSys will even make sure that the graphics resources are managed efficiently,
// but...
// This however is often wasteful:
// The resources are often big and precious, and entities can very frequently be contructed and destructed,
// so that at any time there can be between 0 and a very large number of instances of a single entity class.
//
// The multiply duplicated resources and the possible expense of constructing and destructing them suggest
// to share the resources among entities. The sharing may be limited to entities of the same class,
// or may be globally available to all entities, that is also those of different classes.
//
// I've spent *hours* thinking about good ways to share resources among entities of the same class,
// using static variables. The problem is that global static variables are initialized before the DLL has
// initialized the interfaces (that means that the MatSys::Renderer pointer is not yet set at the time)!
// Thus global static variables won't work.
// Local static variables are only initialized when the control flow passes them for the first time.
// This is much better, because then MatSys::Renderer is properly set, but resource sharing is still not global
// and even worse, if we have to call some Free() function on one of those pointers, we never get a chance to!
// Introducing some reference counting scheme might help, but it requires de-init and re-init each time
// the count drops to 0 and then increases again. Things also start to get uncomfortably complex.
//
// Thus, I've settled for the most straightforward and simple solution:
// A global resource manager that is explicitly initialized after the DLL is running,
// is "technically" the owner of the resources rather than the individual entities but offers them gobally for everybodys use,
// and is explicitly shutdown when the DLL is about to close.
// Even precaching is possible with this solution (but not with any of the others,
// and we don't need it anyway (the engine does it, by accessing the MatSys'es global list of alloc'ed RenderMaterials)).
//
// TODO: Keep a list of all alloced RenderMaterials in each Renderer, reduce the list when FreeMaterial() is called.
// This is good for precaching and for knowing if someone eventually didn't free his material!
class ResourceManagerT
{
    public:

    ArrayT<MatSys::RenderMaterialT*> RenderMats;

    // Just some special "names" (indices) for accessing the RenderMats.
    // They exist for nothing but pure convenience.
    unsigned long PARTICLE_GENERIC1;
    unsigned long PARTICLE_EXPLOSION1_FRAME1;
    unsigned long PARTICLE_EXPLOSIONVERT_FRAME1;
    unsigned long PARTICLE_EXPLOSION2_FRAME1;
    unsigned long PARTICLE_WHITESMOKE_FRAME1;


    void Init();

    void ShutDown();
};


extern ResourceManagerT ResMan;

#endif
