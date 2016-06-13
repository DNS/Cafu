/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
