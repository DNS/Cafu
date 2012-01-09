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

#ifndef _CF_GUISYS_WINDOW_POINTER_HPP_
#define _CF_GUISYS_WINDOW_POINTER_HPP_


struct lua_State;


namespace cf
{
    namespace GuiSys
    {
        class WindowT;


        /**
         * This class represents a reference to a window OUTSIDE OF LUA.
         *
         * Instances of the WindowT class are *only* created by Lua scripts, and kept there in form of Lua objects
         * (we've chosen to represent windows in Lua as a table with an embedded userdata object that in turn keeps
         *  a pointer to the C++ WindowT instance).
         *
         * As such, it is also *only* Lua that determines when a WindowT object is to be deleted again.
         * This occurs in a four step process:
         *   -# Lua detects that there are no more references to a window object.
         *   -# It then sets a flag that indicates that the object can be garbage collected at the next opportunity.
         *   -# When the garbage collection for the object is run, the __gc metamethod of the userdata is called,
         *   -# which is implemented to delete the C++ WindowT instance.
         *
         * That means that everything is fine as long as we keep no references (that is, pointers) to WindowT objects
         * elsewhere, i.e. outside of Lua.
         * Now here comes the catch: When we keep pointers to WindowT objects somewhere in C++ code,
         * Lua knows nothing about this, and might still gargabe collect such a window as described above when it sees fit.
         * This in turn means that our C++ reference to the window might suddently point to deleted memory: crash!
         *
         * Keeping references (pointers) to WindowT objects occurs by the way more often than one thinks, namely with all
         * methods that ask us (the C++ code) to do so. Examples include the gui:SetRootWindow(), ParentWin:AddChild(ChildWin),
         * etc. In practice, it is quite likely that we get a reference to a window that Lua might wish to garbage collect,
         * just consider this example:
         *     function createMyNewWindow() local myWin=gui:new("WindowT"); ...; return myWin; end
         *     Parent:AddChild(createMyNewWindow());
         *
         * The solution to this problem is to explicitly "anchor" window objects for which we keep references in C++ code
         * again in the Lua state. This is achieved by entering the Lua representation of the object once more into a
         * dedicated table in the registry, where as index the light userdata with the C++ pointer to the window instance is used.
         *
         * Scope: Note that this is not related to the similar table "__windows_list_cf" (also in the registry).
         * This table serves to be able to conclude from a given WindowT* to the related Lua instance, not for preventing
         * Lua from pulling the rug out from under us by garbage collecting window instances that we're still referring to.
         */
        class WindowPtrT
        {
            public:

            /// This constructor relies on the "__windows_list_cf" table in the registry for obtaining the "alter ego"
            /// (the Lua instance) of Win_.
            /// @param Win_   The pointer for which an external reference should be created. Can also be NULL.
            WindowPtrT(WindowT* Win_=0);

            /// The copy constructor.
            WindowPtrT(const WindowPtrT& Other);

            /// The destructor.
            ~WindowPtrT();

            /// This methods provides access to the raw resource.
            /// It is provided instead of an implicit conversion operator for better visibility in the code and
            /// in order to avoid accidental type conversions. See Meyers "Effective C++, 3rd edition", item 15.
            WindowT* GetRaw() { return Win; }

            /// The assignment operator.
            WindowPtrT& operator = (const WindowPtrT& Other);

            /// The arrow operator.
            WindowT* operator -> () { return Win; }

            /// The arrow operator.
            const WindowT* operator -> () const { return Win; }

            /// The == operator.
            bool operator == (const WindowPtrT& Other) const { return Win==Other.Win; }

            /// The != operator.
            bool operator != (const WindowPtrT& Other) const { return Win!=Other.Win; }


            private:

            void Anchor();
            void Unanchor();

            WindowT* Win;
        };
    }
}

#endif
