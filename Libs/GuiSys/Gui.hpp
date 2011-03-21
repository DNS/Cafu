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

#ifndef _CF_GUISYS_GUI_HPP_
#define _CF_GUISYS_GUI_HPP_

#include "WindowPtr.hpp"
#include <string>


struct CaKeyboardEventT;
struct CaMouseEventT;
struct luaL_Reg;


namespace cf
{
    namespace GuiSys
    {
        /// General GUI interface.
        class GuiI
        {
            public:

            /// The virtual destructor, so that derived classes can safely be deleted via a GuiI (base class) pointer.
            virtual ~GuiI() { }

            /// Returns the name of the script file of this GUI.
            virtual const std::string& GetScriptName() const=0;

            /// Returns the root window of this GUI.
            virtual WindowPtrT GetRootWindow()=0;

            /// Returns the window in this GUI that has the keyboard input focus.
            virtual WindowPtrT GetFocusWindow()=0;

            /// Activates or deactivates this GUI.
            virtual void Activate(bool doActivate=true)=0;

            /// Returns whether this GUI is active or not. This is of importance mainly for the GuiMan, which doesn't send us events and doesn't draw us if we're not active.
            virtual bool GetIsActive() const=0;

            /// Sets whether this GUI is interactive or not. See GetIsInteractive() for additional information.
            virtual void SetInteractive(bool IsInteractive_=true)=0;

            /// Returns whether this GUI is interactive (reacts to device events) or not. This is of important mainly for the GuiMan, which doesn't send us device events if we are not interactive, and sends device events only to the top-most interactive GUI.
            virtual bool GetIsInteractive() const=0;

            /// Returns whether this GUI is fullscreen and fully opaque, i.e. whether this GUI covers everything under it. If true, the GuiSys saves the rendering of the GUIs "below" this one. This can improve the GUI performance significantly if e.g. the player is at a point in the game where the world rendering FPS is low.
            virtual bool GetIsFullCover() const=0;

            /// Returns the position of the mouse cursor.
            virtual void GetMousePos(float& MousePosX, float& MousePosY) const=0;

            /// Sets the position of the mouse cursor.
            virtual void SetMousePos(float MousePosX_, float MousePosY_)=0;

            /// Sets whether this GUI shows a mouse cursor.
            virtual void SetShowMouse(bool ShowMouse_)=0;

            /// Returns whether this GUI shows a mouse cursor.
            virtual bool IsMouseShown() const=0;

            /// Renders this GUI.
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices: it's up to the caller to do that!
            virtual void Render()=0;

            /// Processes a keyboard event by forwarding it to the window that currently has the input focus.
            /// The GuiMan should make the descision to call this method dependend on the result of the GetIsInteractive() method.
            /// @param KE Keyboard event to process.
            /// @returns true if the device has been successfully processed, false otherwise.
            virtual bool ProcessDeviceEvent(const CaKeyboardEventT& KE)=0;

            /// Processes a mouse event by forwarding it to the window that currently has the input focus.
            /// The GuiMan should make the descision to call this method dependend on the result of the GetIsInteractive() method.
            /// @param ME Mouse event to process.
            /// @returns true if the device has been successfully processed, false otherwise.
            virtual bool ProcessDeviceEvent(const CaMouseEventT& ME)=0;

            /// "Creates" a time tick event for each window of the GUI (no matter whether its currently visible (shown) or not)
            /// by calling its OnTimeTickEvent() methods.
            /// @param t   The time in seconds since the last clock-tick.
            virtual void DistributeClockTickEvents(float t)=0;

            /// Calls the Lua global function with name FuncName.
            ///
            /// @param FuncName    The name of the Lua global function to be called.
            /// @param Signature   Describes the arguments to and results from the Lua function. See below for more details.
            /// @param ...         The arguments to the Lua function and the variables that receive its results as described by the Signature parameter.
            ///
            /// The Signature parameter is a string of individual letters, where each letter represents a variable and its type.
            /// The letters 'b' for bool, 'i' for int, 'f' for float, 'd' for double and 's' for const char* (string) can be used.
            /// A '>' separates the arguments from the results, and is optional if the function returns no results.
            /// For the results, additionally to the other letters, 'S' can be used for (address of) std::string.
            ///
            /// @returns whether the function call was sucessful.
            /// Note that when a signature was provided that expects one or more return values and the called script code yields
            /// (calls coroutine.yield()), the returned values are undefined and thus the call is considered a failure and false is returned.
            /// Nonetheless, the related Lua thread is added to the list of pending coroutines for later resumption.
            virtual bool CallLuaFunc(const char* FuncName, const char* Signature="", ...)=0;

            /// Calls the Lua method with name MethodName of the given window.
            /// @param Window       A window of this GUI for which the specified Lua method is to be called.
            /// @param MethodName   The name of the Lua method of the specified window that is to be called.
            /// @param Signature    Same as in CallLuaFunc(), see there for details.
            /// @param ...          Same as in CallLuaFunc(), see there for details.
            /// @returns whether the method call was sucessful.
            /// This method is analogous to CallLuaFunc(), see there for more details.
            virtual bool CallLuaMethod(WindowPtrT Window, const char* MethodName, const char* Signature="", ...)=0;

            /// If this GUI is used as a 3D world GUI, the owner/parent entity (on which this GUI is "attached" in the world)
            /// can call this method to let this GUI know its name and the pointer to its instance.
            /// @param EntityName_          The name of the parent entity.
            /// @param EntityInstancePtr_   The pointer to the instance of the parent entity.
            virtual void SetEntityInfo(const char* EntityName_, void* EntityInstancePtr_)=0;

            // /// Same as SetEntityInfo(), but for the target entity of our owner/parent entity.
            // /// @param TargetEntityName          The name of the target entity of our parent entity.
            // /// @param TargetEntityInstancePtr   The pointer to the instance of the target entity of our parent entity.
            // virtual void SetTargetEntityInfo(const char* TargetEntityName, void* TargetEntityInstancePtr);

            /// Registers another library of C++ implemented functions for use by the script of this GUI.
            /// This is intended for 3D world GUIs, for which the game code registers an additional set of game-specific functions.
            /// @param LibName Name of the library to register.
            /// @param Functions List of function in this lib.
            virtual void RegisterScriptLib(const char* LibName, const luaL_Reg Functions[])=0;
        };
    }
}

#endif
