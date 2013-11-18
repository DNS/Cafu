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

#ifndef CAFU_GUISYS_GUIMAN_HPP_INCLUDED
#define CAFU_GUISYS_GUIMAN_HPP_INCLUDED

#include "Templates/Array.hpp"
#include <string>


struct CaKeyboardEventT;
struct CaMouseEventT;


namespace cf
{
    namespace GuiSys
    {
        class GuiImplT;
        class GuiResourcesT;


        /// This class implements a GUI manager.
        class GuiManImplT
        {
            public:

            /// The constructor.
            /// The MatSys *must* be initialized *before* this constructor is called (i.e. a GuiManImplT is instantiated)!
            GuiManImplT(GuiResourcesT& GuiRes);

            /// The destructor.
            ~GuiManImplT();

            /// Creates a GUI from the script with name GuiScriptName and registers it with the GUI manager.
            GuiImplT* Register(const std::string& GuiScriptName);

            /// Registers a programmatically instantiated GUI with the GUI manager.
            GuiImplT* Register(GuiImplT* NewGui);

            /// Removes the Gui from the GUI manager and deletes the pointer.
            void Free(GuiImplT* Gui);

            /// Searches the GUI manager for a GUI whose script name is GuiScriptName.
            /// If the GUI was found, the pointer to the GuiImplT instance is returned.
            /// Otherwise (GUI not found), Register() is called and its result returned if AutoRegister was true,
            /// NULL is returned if AutoRegister was false.
            /// @param GuiScriptName The filename of the GUI script to search for.
            /// @param AutoRegister Whether the script should be registered with this GUI manager if it is not found.
            GuiImplT* Find(const std::string& GuiScriptName, bool AutoRegister=false);

            /// Makes sure that if multiple GUIs are active, Gui is the topmost one.
            void BringToFront(GuiImplT* Gui);

            /// Returns the top-most GUI that is both active and interactive.
            /// NULL is returned if no such GUI exists.
            GuiImplT* GetTopmostActiveAndInteractive();

            /// Reloads all registered GUIs.
            void ReloadAllGuis();

            /// Renders all the GUIs.
            void RenderAll();

            /// Processes a keyboard event from the device that this GuiMan is running under,
            /// sending it to the top-most GUI that is both active and interactive.
            /// @param KE Keyboard event to process.
            void ProcessDeviceEvent(const CaKeyboardEventT& KE);

            /// Processes a mouse event from the device that this GuiMan is running under,
            /// sending it to the top-most GUI that is both active and interactive.
            /// @param ME Mouse event to process.
            void ProcessDeviceEvent(const CaMouseEventT& ME);

            /// "Creates" a time tick event for each window of each active GUI by calling its OnTimeTickEvent() methods.
            /// Note that inactive GUIs do not get the time tick event, but active GUIs who get them usually pass them
            /// to all their (sub-)windows, even if that part of the window(-hierarchy) is currently invisible.
            /// @param t   The time in seconds since the last clock-tick.
            void DistributeClockTickEvents(float t);


            private:

            GuiManImplT(const GuiManImplT&);            ///< Use of the Copy Constructor    is not allowed.
            void operator = (const GuiManImplT&);       ///< Use of the Assignment Operator is not allowed.

            GuiResourcesT&    m_GuiResources;       ///< The provider for resources (fonts and models) that are used in the GUIs created by this GuiMan.
            ArrayT<GuiImplT*> Guis;
            bool              SuppressNextChar;     ///< Whether the next character (CaKeyboardEventT::CKE_CHAR) event should be suppressed. This is true whenever the preceeding CaKeyboardEventT::CKE_KEYDOWN event was positively processed.
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
        extern GuiManImplT* GuiMan;
    }
}

#endif
