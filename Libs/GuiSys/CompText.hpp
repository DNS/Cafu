/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<std::string>& Values) const;


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


            protected:

            // The Lua API methods of this class.
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            friend class ComponentListBoxT;
            friend class ComponentTextEditT;

            void FillMemberVars();      ///< A helper method for the constructors.

            TypeSys::VarT<std::string> m_Text;      ///< The text to show in this window.
            VarFontNameT               m_FontName;  ///< The name of the font.
            TrueTypeFontT*             m_FontInst;  ///< The font instance used to render text in this window.
            TypeSys::VarT<float>       m_Scale;     ///< The scale that is applied for rendering the text.
            TypeSys::VarT<Vector2fT>   m_Padding;   ///< Padding between text and window rectangle.
            TypeSys::VarT<Vector3fT>   m_Color;     ///< The text color.
            TypeSys::VarT<float>       m_Alpha;     ///< The alpha component of the color.
            VarTextAlignHorT           m_AlignHor;  ///< How the text is aligned horizontally (left, center, right).
            VarTextAlignVerT           m_AlignVer;  ///< How the text is aligned vertically (top, middle, bottom).
        };
    }
}

#endif
