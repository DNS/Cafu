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

#ifndef CAFU_GUISYS_COMPONENT_TEXT_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_TEXT_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    class TrueTypeFontT;


    namespace GuiSys
    {
        /// This components adds text to its window.
        class ComponentTextT : public ComponentBaseT
        {
            public:

            /// A variable of type std::string, specifically for font names. It updates the related
            /// font instance in the parent ComponentTextT whenever a new font name is set.
            class VarFontNameT : public TypeSys::VarT<std::string>
            {
                public:

                VarFontNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentTextT& CompText);
                VarFontNameT(const VarFontNameT& Var, ComponentTextT& CompText);

                // Base class overrides.
                void Set(const std::string& v);


                private:

                ComponentTextT& m_CompText;     ///< The parent ComponentTextT that contains this variable.
            };


            /// A variable of type int, specifically for horizontal alignments of text.
            class VarTextAlignHorT : public TypeSys::VarT<int>
            {
                public:

                enum { LEFT = -1, CENTER, RIGHT };

                VarTextAlignHorT(const char* Name, const int& Value, const char* Flags[]=NULL);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const;
            };


            /// A variable of type int, specifically for vertical alignments of text.
            class VarTextAlignVerT : public TypeSys::VarT<int>
            {
                public:

                enum { TOP = -1, MIDDLE, BOTTOM };

                VarTextAlignVerT(const char* Name, const int& Value, const char* Flags[]=NULL);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const;
            };


            /// The constructor.
            ComponentTextT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentTextT(const ComponentTextT& Comp);

            /// This method sets this components text value.
            /// Other C++ code (especially other components) would normally have to use `GetMemberVars().Find("Text")`
            /// to set this components text value. This auxiliary method makes the task much easier.
            void SetText(const std::string& t) { m_Text.Set(t); }

            /// This method appends the given text to the components text value.
            void AppendText(const std::string& t) { m_Text.Set(m_Text.Get() + t); }

            // Base class overrides.
            ComponentTextT* Clone() const;
            const char* GetName() const { return "Text"; }
            void UpdateDependencies(WindowT* Window);
            void Render() const;


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            friend class ComponentListBoxT;
            friend class ComponentTextEditT;

            void FillMemberVars();      ///< A helper method for the constructors.

            // The Lua API methods of this class.
            static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
            static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

            VarFontNameT               m_FontName;      ///< The name of the font.
            TrueTypeFontT*             m_FontInst;      ///< The font instance used to render text in this window.
            TypeSys::VarT<std::string> m_Text;          ///< The text to show inside this window.
            TypeSys::VarT<float>       m_Scale;         ///< Scale of this windows text.
            TypeSys::VarT<Vector2fT>   m_Padding;       ///< Padding between text and window rectangle.
            TypeSys::VarT<Vector3fT>   m_Color;         ///< The text color.
            TypeSys::VarT<float>       m_Alpha;         ///< The alpha component of the color.
            VarTextAlignHorT           m_AlignHor;      ///< How the text is aligned horizontally (left, right, centered, block).
            VarTextAlignVerT           m_AlignVer;      ///< How the text is aligned vertically (top, middle, bottom).
        };
    }
}

#endif
