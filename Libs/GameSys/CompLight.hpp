/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_LIGHT_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_LIGHT_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// The common base class for light source components.
        class ComponentLightT : public ComponentBaseT
        {
            public:

            // Base class overrides.
            ComponentLightT* Clone() const;
            const char* GetName() const { return "Light"; }
            unsigned int GetEditorColor() const { return 0xCCFFFF; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            static const char* DocClass;
        };
    }
}

#endif
