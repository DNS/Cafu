/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GLFW_MONITOR_HPP_INCLUDED
#define CAFU_GLFW_MONITOR_HPP_INCLUDED

#include <string>

struct GLFWmonitor;


namespace cf
{
    /// This class wraps a GLFWmonitor of the GLFW library.
    class glfwMonitorT
    {
        public:

        /// The constructor. Throws a std::runtime_error on failure.
        /// `width` and `height` are assumed to be valid full screen sizes,
        /// even if only a windowed window is opened (`monitor` is `NULL`).
        glfwMonitorT(GLFWmonitor* monitor);

        /// Returns the human-readable, UTF-8 encoded name of the monitor.
        std::string getName() const;

        /// Returns the physical size of the monitor, measured in millimeters.
        void getPhysicalSize(unsigned int& widthMM, unsigned int& heightMM) const;

        /// Returns the position of the monitor on the virtual desktop, in screen coordinates.
        void getPos(int& x, int& y) const;

        /// Prints all modes that this monitor can use.
        void printAllModes() const;

        /// Returns a list of screen sizes that can be presented to the user.
        std::string getUserChoiceList() const;


        private:

        GLFWmonitor* m_monitor;
    };
}

#endif
