/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CF_GUISYS_GUIMAN_HPP_
#define _CF_GUISYS_GUIMAN_HPP_

#include <string>


struct CaKeyboardEventT;
struct CaMouseEventT;


namespace MatSys
{
    class RenderMaterialT;
}


namespace cf
{
    class TrueTypeFontT;


    namespace GuiSys
    {
        class GuiI;

        /// Note that it is very difficult to change these constants later, because then all GUI scripts
        /// in the world had to be changed too (and in a non-trivial way)!
        const float VIRTUAL_SCREEN_SIZE_X=640.0f;
        const float VIRTUAL_SCREEN_SIZE_Y=480.0f;


        /// This class provides an interface to a GUI manager.
        /// The interface is specified as an ABC in order to be able to share it across exe/dll boundaries.
        class GuiManI
        {
            public:

            /// Creates a GUI from the script with name GuiScriptName and registers it with the GUI manager.
            virtual GuiI* Register(const std::string& GuiScriptName)=0;

            /// Registers a programmatically instantiated GUI with the GUI manager.
            virtual GuiI* Register(GuiI* NewGui)=0;

            /// Removes the Gui from the GUI manager and deletes the pointer.
            virtual void Free(GuiI* Gui)=0;

            /// Searches the GUI manager for a GUI whose script name is GuiScriptName.
            /// If the GUI was found, the pointer to the GuiI instance is returned.
            /// Otherwise (GUI not found), Register() is called and its result returned if AutoRegister was true,
            /// NULL is returned if AutoRegister was false.
            /// @param GuiScriptName The filename of the GUI script to search for.
            /// @param AutoRegister Whether the script should be registered with this GUI manager if it is not found.
            virtual GuiI* Find(const std::string& GuiScriptName, bool AutoRegister=false)=0;

            /// Makes sure that if multiple GUIs are active, Gui is the topmost one.
            virtual void BringToFront(GuiI* Gui)=0;

            /// Returns the top-most GUI that is both active and interactive.
            /// NULL is returned if no such GUI exists.
            virtual GuiI* GetTopmostActiveAndInteractive()=0;

            /// Reloads all registered GUIs.
            virtual void ReloadAllGuis()=0;

            /// Renders all the GUIs.
            virtual void RenderAll()=0;

            /// Processes a keyboard event from the device that this GuiMan is running under,
            /// sending it to the top-most GUI that is both active and interactive.
            /// @param KE Keyboard event to process.
            virtual void ProcessDeviceEvent(const CaKeyboardEventT& KE)=0;

            /// Processes a mouse event from the device that this GuiMan is running under,
            /// sending it to the top-most GUI that is both active and interactive.
            /// @param ME Mouse event to process.
            virtual void ProcessDeviceEvent(const CaMouseEventT& ME)=0;

            /// "Creates" a time tick event for each window of each active GUI by calling its OnTimeTickEvent() methods.
            /// Note that inactive GUIs do not get the time tick event, but active GUIs who get them usually pass them
            /// to all their (sub-)windows, even if that part of the window(-hierarchy) is currently invisible.
            /// @param t   The time in seconds since the last clock-tick.
            virtual void DistributeClockTickEvents(float t)=0;

            /// Returns (a pointer to) the font with name FontName.
            /// The returned pointer is valid throughout the lifetime of the implementation of this GuiManI,
            /// and must not be freed (the GuiMan retains the ownership of the font).
            /// @param FontName Name of the font to get.
            /// @returns a pointer to the desired font, or NULL if there was an error (e.g. the font could not be loaded).
            virtual TrueTypeFontT* GetFont(const std::string& FontName)=0;

            /// Returns the default RenderMaterialT that should be used for borders and backgrounds if no other material is specified for that window.
            virtual MatSys::RenderMaterialT* GetDefaultRM() const=0;

            /// The destructor.
            /// This ABC does neither have nor need a destructor, because no implementation will ever be deleted via a pointer to a GuiManI.
            /// (The implementations are singletons after all.)  See the Singleton pattern and the C++ FAQ 21.05 (the "precise rule") for more information.
            /// g++ however issues a warning with no such destructor, so I provide one anyway and am safe.
            virtual ~GuiManI() { }
        };


        /// A global pointer to an implementation of the GuiManI interface.
        ///
        /// Each module (exe or dll) that uses this pointer must somewhere provide exactly one definition for it (none is provided by the GuiSys library).
        /// That is, typically the main.cpp or similar file of each exe and dll must contain a line like
        ///     cf::GuiSys::GuiManI* cf::GuiSys::GuiMan=NULL;
        /// or else the module will not link successfully due to an undefined symbol.
        ///
        /// Exe files will then want to reset this pointer to an instance of a GuiManImplT during their initialization
        /// e.g. by code like:   cf::GuiSys::GuiMan=new cf::GuiSys::GuiManImplT;
        /// Note that the GuiManImplT ctor may require that other interfaces (e.g. the MatSys) have been inited first.
        ///
        /// Dlls typically get one of their init functions called immediately after they have been loaded.
        /// By doing so, the exe passes a pointer to its above instance to the dll, which in turn copies it to its GuiMan variable.
        extern GuiManI* GuiMan;
    }
}

#endif
