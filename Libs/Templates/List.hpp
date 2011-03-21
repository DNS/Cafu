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

#ifndef _CF_LINKED_LIST_HPP_
#define _CF_LINKED_LIST_HPP_


/// This class represents a node of a singly-linked list.
template<class T> class ListNodeT
{
    public:

    ListNodeT();        ///< Constructor.
    // ~ListNodeT();    ///< Destructor.

    T             Data;
    ListNodeT<T>* Next;


    private:

    ListNodeT(const ListNodeT<T>& Other);                   ///< Use of the Copy Constructor    is not allowed.
    ListNodeT<T>& operator = (const ListNodeT<T>& Other);   ///< Use of the Assignment Operator is not allowed.
};


// The default constructor.
template<class T> inline ListNodeT<T>::ListNodeT()
{
}

#endif
