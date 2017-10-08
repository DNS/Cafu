/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GLFW_WINDOW_HPP_INCLUDED
#define CAFU_GLFW_WINDOW_HPP_INCLUDED

struct GLFWmonitor;
struct GLFWwindow;


namespace cf
{
    /// This class wraps a GLFWwindow of the GLFW library.
    class glfwWindowT
    {
        public:

        /// The constructor. Throws a std::runtime_error on failure.
        /// `width` and `height` are assumed to be valid full screen sizes,
        /// even if only a windowed window is opened (`monitor` is `NULL`).
        glfwWindowT(int width, int height, const char* title, GLFWmonitor* monitor=0);

        /// The destructor.
        ~glfwWindowT();

        virtual void FramebufferSizeEvent(int width, int height) { }
        virtual void KeyEvent(int key, int scancode, int action, int mods) { }
        virtual void CharEvent(unsigned int codepoint) { }
        virtual void MouseMoveEvent(double posX, double posY) { }
        virtual void MouseButtonEvent(int button, int action, int mods) { }
        virtual void MouseScrollEvent(double xoffset, double yoffset) { }

        void makeContextCurrent();
        void triggerFramebufferSizeEvent();   ///< Useful after makeContextCurrent(), e.g. in order to set the GL viewport.
        void getFramebufferSize(unsigned int& width, unsigned int& height) const;
        void swapBuffers();

        bool shouldClose() const;
        void setShouldClose(bool close);
        void setMouseCursorMode(int value);
        void getMouseCursorPos(double& posX, double& posY) const;
        int  getWindowAttrib(int attrib) const;
        void getWindowSize(unsigned int& width, unsigned int& height) const;
        void toggleFullScreen(bool mode_change);
        bool isKeyPressed(int key) const;
        bool isMouseButtonPressed(int button) const;


        private:

        GLFWwindow* m_win;
        int         m_last_fs_width;
        int         m_last_fs_height;
        int         m_last_win_width;
        int         m_last_win_height;
    };
}

#endif
