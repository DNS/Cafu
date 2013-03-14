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

#include "CompBasics.hpp"
#include "CompTransform.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"

#include <cstdarg>
#include <climits>
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


namespace cf
{
    namespace GuiSys
    {
        class ComponentBaseT;
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
            /// @param Params   The creation parameters for the window.
            WindowT(const WindowCreateParamsT& Params);

            /// The copy constructor.
            /// Copies a window (optionally with all of its children recursively).
            /// The parent of the copy is always NULL and it is up to the caller to put the copy into a window hierarchy.
            /// @param Window      The window to construct this window from.
            /// @param Recursive   Whether to recursively copy all children.
            WindowT(const WindowT& Window, bool Recursive=false);

            /// The virtual copy constructor.
            /// Callers can use this method to create a copy of this window without knowing its concrete type.
            /// Overrides in derived classes use a covariant return type to facilitate use when the concrete type is known.
            /// @param Recursive   Whether to recursively clone all children of this window.
            virtual WindowT* Clone(bool Recursive=false) const;

            /// The virtual destructor. Deletes this window and all its children.
            virtual ~WindowT();

            GuiImplT& GetGui() const { return m_Gui; }

            /// Returns the ExtDataT instance for this window (possibly NULL).
            ExtDataT* GetExtData() { return m_ExtData; }

            /// Assigns the editor sibling for this window.
            void SetExtData(ExtDataT* ExtData);

            /// Returns the name of this window.
            const std::string& GetName() const { return m_Basics->GetWindowName(); }

            /// Sets a new name for this window.
            ///
            /// Note that the new name that is actually set for this window is not necessarily exactly the given
            /// string Name, but possibly a variant thereof. That is, GetName() can return a different string than
            /// what was given to a preceeding call to SetName().
            /// This is because the name of a window must be unique among its siblings (the children of its parent),
            /// and SetName() modifies the given string as necessary to enforce this rule.
            ///
            /// @param Name   The new name to be set for this window.
            void SetName(const std::string& Name);

            /// Returns the parent window of this window.
            IntrusivePtrT<WindowT> GetParent() const { return m_Parent; }

            /// Returns the immediate children of this window.
            /// This is analogous to calling GetChildren(Chld, false) with an initially empty Chld array.
            const ArrayT< IntrusivePtrT<WindowT> >& GetChildren() const { return m_Children; }

            /// Returns the children of this window.
            /// @param Chld      The array to which the children of this window are appended. Note that Chld gets *not* initially cleared by this function!
            /// @param Recurse   Determines if also the grand-children, grand-grand-children etc. are returned.
            void GetChildren(ArrayT< IntrusivePtrT<WindowT> >& Chld, bool Recurse=false) const;

            /// Adds the given window to the children of this window, and sets this window as the parent of the child.
            ///
            /// This method also makes sure that the name of the Child is unique among its siblings,
            /// modifying it as necessary. See SetName() for more details.
            ///
            /// @param Child   The window to add to the children of this window.
            /// @param Pos     The position among the children to insert the child window at.
            /// @returns true on success, false on failure (Child has a parent already, or is the root of this window).
            bool AddChild(IntrusivePtrT<WindowT> Child, unsigned long Pos=0xFFFFFFFF);

            /// Removes the given window from the children of this window.
            /// @param Child   The window to remove from the children of this window.
            /// @returns true on success, false on failure (Child is not a child of this window).
            bool RemoveChild(IntrusivePtrT<WindowT> Child);

            /// Returns the top-most parent of this window, that is, the root of the hierarchy this window is in.
            IntrusivePtrT<WindowT> GetRoot();     // Method cannot be const because return type is not const -- see implementation.


            /// Returns `true` if this window is currently shown. Returns `false` if the window is currently hidden.
            bool IsShown() const { return m_Basics->IsShown(); }

            /// Returns the "Basics" component of this window.
            /// The "Basics" component defines the name and the "show" flag of the window.
            IntrusivePtrT<ComponentBasicsT> GetBasics() const { return m_Basics; }

            /// Returns the "Transform" component of this window.
            /// The "Transform" component defines the position, size and orientation of the window.
            IntrusivePtrT<ComponentTransformT> GetTransform() const { return m_Transform; }


            /// Returns the components that this window is composed of.
            const ArrayT< IntrusivePtrT<ComponentBaseT> >& GetComponents() const { return m_Components; }

            /// Returns the (n-th) component of the given (type) name.
            IntrusivePtrT<ComponentBaseT> GetComponent(const std::string& TypeName, unsigned int n=0) const;

            /// Adds the given component to this window.
            ///
            /// @param Comp    The component to add to this window.
            /// @param Index   The position among the other components to insert `Comp` at.
            /// @returns `true` on success, `false` on failure (if `Comp` is part of a window already).
            bool AddComponent(IntrusivePtrT<ComponentBaseT> Comp, unsigned long Index=ULONG_MAX);

            /// Deletes the component at the given index from this window.
            void DeleteComponent(unsigned long CompNr);

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

            /// Get the a member variable of this class.
            /// @param VarName The name of the member variable.
            /// @return The member variable with the name VarName.
            MemberVarT& GetMemberVar(std::string VarName) { return MemberVars[VarName]; }


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // Methods called from Lua scripts on cf::GuiSys::WindowTs.
            // They are protected so that derived window classes can access them when implementing overloads.
            static int Set(lua_State* LuaState);            ///< Sets a member variable of this class.
            static int Get(lua_State* LuaState);            ///< Gets a member variable of this class.
            static int Interpolate(lua_State* LuaState);    ///< Schedules a value for interpolation between a start and end value over a given period of time.
            static int AddChild(lua_State* LuaState);       ///< Adds a child to this window.
            static int RemoveChild(lua_State* LuaState);    ///< Removes a child from this window.
            static int GetParent(lua_State* LuaState);      ///< Returns the parent of this window (or nil if there is no parent).
            static int GetChildren(lua_State* LuaState);    ///< Returns an array of the children of this window.
            static int GetBasics(lua_State* LuaState);      ///< Returns the "Basics" component of this window.
            static int GetTransform(lua_State* LuaState);   ///< Returns the "Transform" component of this window.
            static int AddComponent(lua_State* LuaState);   ///< Adds a component to this window.
            static int RmvComponent(lua_State* LuaState);   ///< Removes a component from this window.
            static int GetComponents(lua_State* LuaState);  ///< Returns an array of the components of this window.
            static int GetComponent(lua_State* LuaState);   ///< Returns the (n-th) component of the given (type) name.
            static int toString(lua_State* LuaState);       ///< Returns a readable string representation of this object.

            static const luaL_Reg MethodsList[]; ///< List of methods registered with Lua.

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

            void FillMemberVars();              ///< Helper method that fills the MemberVars array with entries for each class member.
            void operator = (const WindowT&);   ///< Use of the Assignment Operator is not allowed.

            GuiImplT&                               m_Gui;            ///< The GUI instance in which this window was created and exists. Useful in many regards, but especially for access to the underlying Lua state.
            ExtDataT*                               m_ExtData;        ///< The GuiEditor's "dual" or "sibling" of this window.
            WindowT*                                m_Parent;         ///< The parent of this window. May be NULL if there is no parent. In order to not create cycles of IntrusivePtrT's, the type is intentionally a raw pointer only.
            ArrayT< IntrusivePtrT<WindowT> >        m_Children;       ///< The list of children of this window.
            float                                   m_Time;           ///< This windows local time (starting from 0.0).
            IntrusivePtrT<ComponentBasicsT>         m_Basics;         ///< The component that defines the name and the "show" flag of this window.
            IntrusivePtrT<ComponentTransformT>      m_Transform;      ///< The component that defines the position, size and orientation of this window.
            ArrayT< IntrusivePtrT<ComponentBaseT> > m_Components;     ///< The components that this window is composed of.
            ArrayT<InterpolationT*>                 m_PendingInterp;  ///< The currently pending interpolations.
        };
    }
}

#endif
