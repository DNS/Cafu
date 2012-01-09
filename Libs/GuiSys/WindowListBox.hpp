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

#ifndef _CF_GUISYS_WINDOW_LISTBOX_HPP_
#define _CF_GUISYS_WINDOW_LISTBOX_HPP_

#include "Window.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// A listbox window.
        class ListBoxT : public WindowT
        {
            public:

            /// Constructor for creating a listbox.
            ListBoxT(const cf::GuiSys::WindowCreateParamsT& Params);

            /// The Copy Constructor.
            ListBoxT(const ListBoxT& Window, bool Recursive=false);

            virtual ListBoxT* Clone(bool Recursive=false) const;

            /// Destructor.
            ~ListBoxT();

            const float  GetRowHeight() const            { return RowHeight; }              ///< Returns the height of each row.
            const float* GetOddRowBgColor() const        { return OddRowBgColor; }          ///< Returns the background color for unselected odd  rows (for coloring, numering starts at one, not zero).
            const float* GetEvenRowBgColor() const       { return EvenRowBgColor; }         ///< Returns the background color for unselected even rows (for coloring, numering starts at one, not zero).
            const float* GetRowTextColor() const         { return RowTextColor; }           ///< Returns the text color for unselected rows.
            const float* GetSelectedRowBgColor() const   { return SelectedRowBgColor; }     ///< Returns the background color for selected rows.
            const float* GetSelectedRowTextColor() const { return SelectedRowTextColor; }   ///< Returns the text color for selected rows.

            /// Inserts a new row into the list box.
            /// @param RowNr     The index where to insert the new row.
            /// @param RowText   Text of the row to insert.
            void Insert(unsigned long RowNr, const std::string& RowText);

            // Overloaded methods from the base class.
            void Render() const;
            bool OnInputEvent(const CaKeyboardEventT& KE);
            bool OnInputEvent(const CaMouseEventT&    ME, float PosX, float PosY);


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            void FillMemberVars(); ///< Helper method that fills the MemberVars array with entries for each class member.


            private:

            ArrayT<WindowT*> Rows;
            unsigned long    SelectedRow;   ///< Number of the currently selected row, 0xFFFFFFFF for none.
            float            RowHeight;     ///< The height of each row.

            float OddRowBgColor[4];         ///< Background color for unselected odd  rows (for coloring, numering starts at one, not zero).
            float EvenRowBgColor[4];        ///< Background color for unselected even rows (for coloring, numering starts at one, not zero).
            float RowTextColor[4];          ///< Text color for unselected rows.
            float SelectedRowBgColor[4];    ///< Background color for selected rows.
            float SelectedRowTextColor[4];  ///< Text color for selected rows.


            // Lua script methods.
            static int Clear(lua_State* LuaState);
            static int Append(lua_State* LuaState);
            static int Insert(lua_State* LuaState);
            static int GetNumRows(lua_State* LuaState);
            static int GetRowText(lua_State* LuaState);
            static int SetRowText(lua_State* LuaState);
            static int GetSelection(lua_State* LuaState);
            static int SetSelection(lua_State* LuaState);

            static int GetRowHeight(lua_State* LuaState);
            static int SetRowHeight(lua_State* LuaState);

            static int SetOddRowBgColor(lua_State* LuaState);
            static int SetEvenRowBgColor(lua_State* LuaState);
            static int SetRowTextColor(lua_State* LuaState);
            static int SetSelRowBgColor(lua_State* LuaState);
            static int SetSelRowTextColor(lua_State* LuaState);

            static const luaL_Reg MethodsList[];
        };
    }
}

#endif
