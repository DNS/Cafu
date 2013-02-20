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

#ifndef CAFU_MATH_ERRORS_HPP_INCLUDED
#define CAFU_MATH_ERRORS_HPP_INCLUDED

#include <stdexcept>


/// General math error.
struct Math3DErrorE : public std::runtime_error
{
    /// Constructor.
    Math3DErrorE(const std::string& what_arg) : runtime_error(what_arg)
    {
    }
};


/// Division by zero error.
struct DivisionByZeroE : public Math3DErrorE
{
    DivisionByZeroE() : Math3DErrorE("Division by 0.")
    {
    }
};


/// Invalid operation (invalid use of method, etc.).
struct InvalidOperationE : public Math3DErrorE
{
    InvalidOperationE() : Math3DErrorE("Invalid operation.")
    {
    }
};

#endif
