/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GLFW_LIBRARY_HPP_INCLUDED
#define CAFU_GLFW_LIBRARY_HPP_INCLUDED


namespace cf
{
    /// This class encapsulates the GLFW library itself. Its main purpose is to
    /// make sure that GLFW is properly terminated at program end ("RAII").
    class glfwLibraryT
    {
        public:

        glfwLibraryT();
        ~glfwLibraryT();


        private:

        glfwLibraryT(const glfwLibraryT&);      ///< Use of the Copy    Constructor is not allowed.
        void operator = (const glfwLibraryT&);  ///< Use of the Assignment Operator is not allowed.
    };
}

#endif
