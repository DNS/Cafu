/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_ELEMENT_TYPES_HPP_INCLUDED
#define CAFU_MODELEDITOR_ELEMENT_TYPES_HPP_INCLUDED


namespace ModelEditor
{
    /// Enumerates the types of the elements that a model is composed of.
    enum ModelElementTypeT
    {
        JOINT=0,
        MESH =1,
        SKIN =2,
        GFIX =3,
        ANIM =4,
        CHAN =5
    };
}

#endif
