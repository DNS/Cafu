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

#ifndef CAFU_GUISYS_GUI_HPP_INCLUDED
#define CAFU_GUISYS_GUI_HPP_INCLUDED

#include "Templates/Pointer.hpp"
#include <string>


namespace cf { class UniScriptStateT; }
struct CaKeyboardEventT;
struct CaMouseEventT;
struct luaL_Reg;


namespace cf
{
    namespace GuiSys
    {
        class WindowT;

        /// Note that it is very difficult to change these constants later, because then all GUI scripts
        /// in the world had to be changed too (and in a non-trivial way)!
        const float VIRTUAL_SCREEN_SIZE_X=640.0f;
        const float VIRTUAL_SCREEN_SIZE_Y=480.0f;


        /// General GUI interface.
        class GuiI
        {
            public:

            /// The virtual destructor, so that derived classes can safely be deleted via a GuiI (base class) pointer.
            virtual ~GuiI() { }

            /// Returns the name of the script file of this GUI.
            virtual const std::string& GetScriptName() const=0;

            /// Returns the script state of this GUI.
            virtual UniScriptStateT& GetScriptState()=0;

            /// Returns the root window of this GUI.
            virtual IntrusivePtrT<WindowT> GetRootWindow() const=0;

            /// Returns the window in this GUI that has the keyboard input focus.
            virtual IntrusivePtrT<WindowT> GetFocusWindow() const=0;

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
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices:
            /// it's up to the caller to do that.
            /// @param zLayerCoating   Whether a z-layer coating should be applied to the GUI screen when finishing the rendering.
            ///     This is useful whenever the z-ordering of scene elements can be imperfect, e.g. in the Map Editor.
            ///     Generally, 3D world GUIs should use \c true, 2D GUIs should use \c false.
            virtual void Render(bool zLayerCoating=false) const=0;

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

            /// If this GUI is used as a 3D world GUI, the host entity (on which this GUI is "attached" in the world)
            /// can call this method to let this GUI know the map script state and its name therein.
            /// @param MapScriptState   The script state of the map that this GUI and its host entity are in.
            /// @param EntityName       The name of the host entity.
            virtual void SetEntityInfo(UniScriptStateT* MapScriptState, const std::string& EntityName)=0;

            /// Registers another library of C++ implemented functions for use by the script of this GUI.
            /// This is intended for 3D world GUIs, for which the game code registers an additional set of game-specific functions.
            /// @param LibName Name of the library to register.
            /// @param Functions List of function in this lib.
            virtual void RegisterScriptLib(const char* LibName, const luaL_Reg Functions[])=0;
        };
    }
}

#endif
