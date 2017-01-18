/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "glfwLibrary.hpp"
#include "ConsoleCommands/Console.hpp"
#include "GLFW/glfw3.h"
#include <stdexcept>

using namespace cf;


static void error_callback(int error, const char* description)
{
    // fprintf(stderr, "GLFW Error: %s\n", description);
    Console->Print("GLFW Error: ");
    Console->Print(description);
    Console->Print("\n");
}


glfwLibraryT::glfwLibraryT()
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        throw std::runtime_error("GLFW could not be initialized.");
}


glfwLibraryT::~glfwLibraryT()
{
    glfwTerminate();
}
