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

#ifndef CAFU_GUISYS_WINDOW_HPP_INCLUDED
#define CAFU_GUISYS_WINDOW_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"

#include <cstdarg>
#include <map>
#include <string>

// This macro is introduced by some header (gtk?) under Linux...
#undef CurrentTime


struct CaKeyboardEventT;
struct CaMouseEventT;
struct lua_State;
struct luaL_Reg;
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }
namespace MatSys { class RenderMaterialT; }


namespace cf
{
    class TrueTypeFontT;


    namespace GuiSys
    {
        class GuiImplT;
        class WindowCreateParamsT;


        /// The TypeInfoTs of all WindowT derived classes must register with this TypeInfoManT instance.
        cf::TypeSys::TypeInfoManT& GetWindowTIM();


        /**
         * This class represents a window of the GuiSys.
         * A WindowT is the most basic element of a GUI, and all other windows are derived and/or combined from this.
         *
         * WindowT instances can be created in C++ code or in Lua scripts, using the gui:new() function.
         * They can be passed from C++ code to Lua and from Lua to C++ code at will.
         * In C++ code, all WindowT instances are kept in IntrusivePtrT's. Their lifetime is properly managed:
         * A window is deleted automatically when it is no longer used in Lua \emph{and} in C++.
         * That is, code like
         *     Example 1:    w=gui:new("WindowT"); gui:SetRootWindow(w); w=nil;
         *     Example 2:    w=gui:new("WindowT"); w:AddChild(gui:new("WindowT"));
         * works as expected. See the cf::ScriptBinderT class for technical and implementation details.
         */
        class WindowT : public RefCountedT
        {
            public:

            /// Extra / extern / extension data that user code can derive from and assign to this window
            /// in order to "configure" it with "callbacks" (e.g.\ the \c Render() method) and to have it
            /// store additional user-specific data and functions.
            struct ExtDataT
            {
                virtual ~ExtDataT() { }
                //virtual xy* Clone();    // ???
                virtual void Render() const { }   ///< Callback for rendering additional items, called from WindowT::Render().
            };

            /// Describes the member variable of a class consisting of a variable type and a void pointer
            /// pointing to the real member.
            struct MemberVarT
            {
                enum TypeT { TYPE_FLOAT, TYPE_FLOAT2, TYPE_FLOAT4, TYPE_INT, TYPE_BOOL, TYPE_STRING };

                TypeT Type;   ///< Type of the member.
                void* Member; ///< Pointer to the member variable.

                MemberVarT(TypeT t=TYPE_FLOAT, void* v=NULL) : Type(t), Member(v) { }
                MemberVarT(float& f) : Type(TYPE_FLOAT), Member(&f) { }
                MemberVarT(int& i) : Type(TYPE_INT), Member(&i) { }
                MemberVarT(bool& b) : Type(TYPE_BOOL), Member(&b) { }
                MemberVarT(std::string& s) : Type(TYPE_STRING), Member(&s) { }
            };

            /// The normal constructor.
            /// This constructor can *not* be declared as "protected", because even though only derived classes and
            /// the CreateInstance() function access it, having it protected would not allow derived classes to create
            /// new instances (e.g. for subwindows)! See the "C++ FAQs" by Cline etc., FAQs 18.02 and 18.03 for details.
            /// @param Params The creation parameters for the window.
            WindowT(const WindowCreateParamsT& Params);

            /// The Copy Constructor.
            /// Copies a window (optionally with all of its children recursively).
            /// The parent of the copy is always NULL and it is up to the caller to put the copy into a window hierarchy.
            /// @param Window The window to construct this window from.
            /// @param Recursive Whether to recursively copy all children.
            WindowT(const WindowT& Window, bool Recursive=false);

            /// Virtual Copy Constructor.
            /// Creates an exact clone of the window and due to its virtuality considers the real class not just the class
            /// on which the method is called as with the copy ctor.
            /// @param Recursive Whether to recursively clone all children of this window.
            virtual WindowT* Clone(bool Recursive=false) const;

            /// The virtual destructor. Deletes this window and all its children.
            virtual ~WindowT();

            const GuiImplT& GetGui() const { return m_Gui; }

            /// Assigns the editor sibling for this window.
            void SetExtData(ExtDataT* ExtData);

            /// Returns the ExtDataT instance for this window (possibly NULL).
            ExtDataT* GetExtData() { return m_ExtData; }

            /// Returns the name of this window.
            const std::string& GetName() const;

            /// Returns the parent window of this window.
            IntrusivePtrT<WindowT> GetParent() { return m_Parent; }

            /// Returns the children of this window.
            /// @param Chld      The array to which the children of this window are appended. Note that Chld gets *not* initially cleared by this function!
            /// @param Recurse   Determines if also the grand-children, grand-grand-children etc. are returned.
            void GetChildren(ArrayT< IntrusivePtrT<WindowT> >& Chld, bool Recurse=false);

            /// Returns the top-most parent of this window, that is, the root of the hierarchy this window is in.
            IntrusivePtrT<WindowT> GetRoot();     // Method cannot be const because return type is not const -- see implementation.

            /// Returns the position of the upper left corner of this window in absolute (vs. relative to the parent) virtual coordinates.
            /// @param x Variable to store the x coordinate of the upper left corner.
            /// @param y Variable to store the y coordinate of the upper left corner.
            void GetAbsolutePos(float& x, float& y) const;

            /// Finds the window with the name WantedName in the hierachy tree of this window.
            /// Use GetRoot()->Find("xy") in order to search the entire GUI for the window with name "xy".
            /// @param WantedName   The name of the window that is to be found.
            /// @returns The pointer to the desired window, or NULL if no window with this name exists.
            IntrusivePtrT<WindowT> Find(const std::string& WantedName);   // Method cannot be const because return type is not const -- see implementation.

            /// Finds the topmost window that contains the point (x, y) in the hierachy tree of this window
            /// (with (x, y) being (absolute) virtual screen coordinates, *not* relative to this window).
            /// Use GetRoot()->Find(x, y) in order to search the entire GUI for the window containing the point (x, y).
            /// @param x   The x-coordinate of the test point.
            /// @param y   The y-coordinate of the test point.
            /// @param OnlyVisible   If true, only visible windows are reported. If false, all windows are searched.
            /// @returns The pointer to the desired window, or NULL if there is no window that contains the point (x, y).
            IntrusivePtrT<WindowT> Find(float x, float y, bool OnlyVisible=true); // Method cannot be const because return type is not const -- see implementation.

            /// Renders this window.
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices: it's up to the caller to do that!
            virtual void Render() const;


            // Event handler methods.

            /// Keyboard input event handler.
            /// @param KE Keyboard event.
            virtual bool OnInputEvent(const CaKeyboardEventT& KE);

            /// Mouse input event handler.
            /// @param ME Mouse event.
            /// @param PosX Mouse position x.
            /// @param PosY Mouse position y.
            virtual bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY);

            /// The clock-tick event handler.
            /// @param t The time in seconds since the last clock-tick.
            virtual bool OnClockTickEvent(float t);


            /// Calls the Lua method with name MethodName of this window.
            /// This method is analogous to GuiI::CallLuaFunc(), see there for more details.
            /// @param MethodName The name of the lua method to call.
            /// @param Signature DOCTODO
            bool CallLuaMethod(const char* MethodName, const char* Signature="", ...);

            /// Same as CallLuaMethod() above, except that the arguments and results are passed via vl rather than "...".
            /// @param MethodName The name of the lua method to call.
            /// @param Signature DOCTODO
            /// @param vl DOCTODO
            bool CallLuaMethod(const char* MethodName, const char* Signature, va_list vl);

            /// Get the a member variable of this class.
            /// @param VarName The name of the member variable.
            /// @return The member variable with the name VarName.
            MemberVarT& GetMemberVar(std::string VarName) { return MemberVars[VarName]; }


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            /// Enumeartion of horizontal alignments of a window.
            /// The purpose of END_HOR is to ensure that an int is used as the underlying type.
            enum TextAlignHorT { left, right, center, END_HOR=0x10000000 };

            /// Enumeartion of vertical alignments of a window.
            /// The purpose of END_VER is to ensure that an int is used as the underlying type.
            enum TextAlignVerT { top, bottom, middle, END_VER=0x10000000 };


            IntrusivePtrT<WindowT>           m_Parent;    ///< The parent of this window. May be NULL if there is no parent.
            ArrayT< IntrusivePtrT<WindowT> > m_Children;  ///< The list of children of this window.

            std::string              Name;              ///< The name of this window. It must be unique throughout the entire GUI (hierarchy of parent and children).
            float                    Time;              ///< This windows local time (starting from 0.0).
            bool                     ShowWindow;        ///< Is this WindowT shown on screen?
            float                    Rect[4];           ///< The upper left corner of this window, relative to its parent, plus the width and height (all in virtual pixels).
            float                    RotAngle;          ///< The angle in degrees by how much this entire window is rotated.
            MatSys::RenderMaterialT* BackRenderMat;     ///< The render material used to render this windows background.
            std::string              BackRenderMatName; ///< The name of the render material.
            float                    BackColor[4];      ///< The windows background color.
            float                    BorderWidth;       ///< The windows border width.
            float                    BorderColor[4];    ///< The windows border color.
            TrueTypeFontT*           Font;              ///< The font used to render text in this window.
            std::string              Text;              ///< The text to show inside this window.
            float                    TextScale;         ///< Scale of this windows text.
            float                    TextColor[4];      ///< Color of the windows text.
            TextAlignHorT            TextAlignHor;      ///< How the text is aligned horizontally (left, right, centered, block).
            TextAlignVerT            TextAlignVer;      ///< How the text is aligned vertically (top, middle, bottom).


            protected:

            // Methods called from Lua scripts on cf::GuiSys::WindowTs.
            // They are protected so that derived window classes can access them when implementing overloads.
            static int Set(lua_State* LuaState);            ///< Sets a member variable of this class.
            static int Get(lua_State* LuaState);            ///< Gets a member variable of this class.
            static int Interpolate(lua_State* LuaState);    ///< Schedules a value for interpolation between a start and end value over a given period of time.
            static int GetName(lua_State* LuaState);        ///< Gets the windows name.
            static int SetName(lua_State* LuaState);        ///< Sets the windows name.
            static int AddChild(lua_State* LuaState);       ///< Adds a child to this window.
            static int RemoveChild(lua_State* LuaState);    ///< Removes a child from this window.
         // static int GetParent(lua_State* LuaState);      ///< Returns the parent of this window (or nil if there is no parent).  TODO!
         // static int GetChildren(lua_State* LuaState);    ///< Returns the children of this window.                               TODO!
            static int toString(lua_State* LuaState);       ///< Returns a readable string representation of this object.

            static const luaL_Reg MethodsList[]; ///< List of methods registered with Lua.

            void FillMemberVars();  ///< Helper method that fills the MemberVars array with entries for each class member.

            GuiImplT&               m_Gui;            ///< The GUI instance in which this window was created and exists. Useful in many regards, but especially for access to the underlying Lua state, which in turn keeps the alter ego instance of this window.
            ExtDataT*               m_ExtData;        ///< The GuiEditor's "dual" or "sibling" of this window.

            /// Maps strings (names) to member variables of this class.
            /// This map is needed for implementing the Lua-binding methods efficiently.
            /// It is also used in the GUI editor to easily modify members without the need for Get/Set methods.
            std::map<std::string, MemberVarT> MemberVars;


            private:

            /// A helper structure for interpolations between values.
            struct InterpolationT
            {
                float* Value;           ///< Value[0] = resulting value of the linear interpolation.
                float  StartValue;      ///< Start value.
                float  EndValue;        ///< End value.
                float  CurrentTime;     ///< Current time between 0 and TotalTime.
                float  TotalTime;       ///< Duration of the interpolation.

                void UpdateValue()
                {
                    Value[0]=StartValue+(EndValue-StartValue)*CurrentTime/TotalTime;
                }
            };

            void operator = (const WindowT&);           ///< Use of the Assignment Operator is not allowed.

            ArrayT<InterpolationT*> m_PendingInterp;    ///< The currently pending interpolations.
        };
    }
}

#endif
