/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_GUISYS_COMPONENT_IMAGE_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_IMAGE_HPP_INCLUDED

#include "CompBase.hpp"


namespace MatSys { class RenderMaterialT; }


namespace cf
{
    namespace GuiSys
    {
        /// This component adds an image to its window.
        class ComponentImageT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentImageT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentImageT(const ComponentImageT& Comp);

            /// The destructor.
            ~ComponentImageT();

            /// Returns the name of the MatSys material that is used for the image.
            const std::string& GetMatName() const { return m_MatName.Get(); }

            // Base class overrides.
            ComponentImageT* Clone() const;
            const char* GetName() const { return "Image"; }
            void UpdateDependencies(WindowT* Window);
            void Render() const;


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            /// A variable of type std::string, specifically for material names. It updates the related
            /// render material instance in the parent ComponentImageT whenever a new material name is set.
            class VarMatNameT : public TypeSys::VarT<std::string>
            {
                public:

                VarMatNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentImageT& CompImg);
                VarMatNameT(const VarMatNameT& Var, ComponentImageT& CompImg);

                // Base class overrides.
                void Set(const std::string& v);


                private:

                ComponentImageT& m_CompImg;     ///< The parent ComponentImageT that contains this variable.
            };


            void FillMemberVars();      ///< A helper method for the constructors.

            // The Lua API methods of this class.
            static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
            static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

            VarMatNameT              m_MatName;   ///< The name of the image material.
            MatSys::RenderMaterialT* m_MatInst;   ///< The render instance of the material.
            TypeSys::VarT<Vector3fT> m_Color;     ///< The color with which the image is tinted.
            TypeSys::VarT<float>     m_Alpha;     ///< The alpha component of the color.
        };
    }
}

#endif
