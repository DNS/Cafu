/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "glfwMonitor.hpp"
#include "GLFW/glfw3.h"

#include <algorithm>
#include <vector>

using namespace cf;


static void printMode(const GLFWvidmode& mode, float phys_ar = 0.0f)
{
    const float ar = float(mode.width) / mode.height;

    printf("    %4i * %4i, 16:%4.1f (%5.3f %6.3f), %i %i %i, %iHz\n",
        mode.width, mode.height, 16.0f / ar, ar, phys_ar - ar, mode.redBits, mode.greenBits, mode.blueBits, mode.refreshRate);
}


static void printModes(const char* headline, const std::vector<GLFWvidmode>& ModeList, float phys_ar = 0.0f)
{
    const bool verbose = false;   // Normally false, true for debugging.

    if (!verbose)
        return;

    printf("\n%s:", headline);
    if (phys_ar != 0.0f)
        printf("    (physical monitor a-r is %5.3f)", phys_ar);
    printf("\n");

    for (size_t i = 0; i < ModeList.size(); i++)
        printMode(ModeList[i], phys_ar);
}


struct CompareByWidth
{
    inline bool operator() (const GLFWvidmode& m1, const GLFWvidmode& m2)
    {
        // Compare the widths.
        if (m1.width < m2.width) return true;
        if (m1.width > m2.width) return false;

        // The widths are equal, now compare the heights.
        if (m1.height < m2.height) return true;
        if (m1.height > m2.height) return false;

        // The modes are equal.
        return false;
    }
};


glfwMonitorT::glfwMonitorT(GLFWmonitor* monitor)
    : m_monitor(monitor)
{
}


std::string glfwMonitorT::getName() const
{
    return glfwGetMonitorName(m_monitor);
}


void glfwMonitorT::getPhysicalSize(unsigned int& widthMM, unsigned int& heightMM) const
{
    widthMM = 0;
    heightMM = 0;

    glfwGetMonitorPhysicalSize(m_monitor, (int*)&widthMM, (int*)&heightMM);
}


void glfwMonitorT::getPos(int& x, int& y) const
{
    x = 0;
    y = 0;

    glfwGetMonitorPos(m_monitor, &x, &y);
}


void glfwMonitorT::printAllModes() const
{
    int xpos, ypos;
    getPos(xpos, ypos);

    unsigned int phys_width;
    unsigned int phys_height;
    getPhysicalSize(phys_width, phys_height);

    const float phys_ar = float(phys_width) / float(phys_height);
    int num_modes;
    const GLFWvidmode* modes = glfwGetVideoModes(m_monitor, &num_modes);
    std::vector<GLFWvidmode> ModeList;

    ModeList.reserve(num_modes);
    for (int i = 0; i < num_modes; i++)
        ModeList.push_back(modes[i]);
    std::sort(ModeList.begin(), ModeList.end(), CompareByWidth());

    printf("\n%s:   (pos (%i, %i), size %imm * %imm, a-r %5.3f)\n", glfwGetMonitorName(m_monitor), xpos, ypos, phys_width, phys_height, phys_ar);
    for (size_t i = 0; i < ModeList.size(); i++)
        printMode(ModeList[i], phys_ar);
}


std::string glfwMonitorT::getUserChoiceList() const
{
    // For overall strategy, see http://discourse.glfw.org/t/filter-video-modes-for-user-choice/1002
    unsigned int phys_width;
    unsigned int phys_height;
    getPhysicalSize(phys_width, phys_height);
    const float phys_ar = float(phys_width) / float(phys_height);

    int num_modes;
    const GLFWvidmode* modes = glfwGetVideoModes(m_monitor, &num_modes);
    std::vector<GLFWvidmode> ModeList;

    ModeList.reserve(num_modes);
    for (int i = 0; i < num_modes; i++)
        ModeList.push_back(modes[i]);
    std::sort(ModeList.begin(), ModeList.end(), CompareByWidth());

    printModes("All modes, sorted by width", ModeList, phys_ar);

    // Among modes that are of (almost) the same width, keep the one that best matches
    // the aspect-ratio of the physical screen.
    // This also removes duplicates from ignoring the bits per pixel and refresh rate.
    for (size_t i = 0; i < ModeList.size(); i++)
    {
        float mi_ar = float(ModeList[i].width) / float(ModeList[i].height);
        float mi_df = abs(phys_ar - mi_ar);

        for (size_t j = i + 1; j < ModeList.size(); j++)
        {
            if (abs(ModeList[i].width - ModeList[j].width) < 80)
            {
                const float mj_ar = float(ModeList[j].width) / float(ModeList[j].height);
                const float mj_df = abs(phys_ar - mj_ar);

                if (mj_df < mi_df)
                {
                    ModeList[i] = ModeList[j];
                    mi_ar = mj_ar;
                    mi_df = mj_df;
                }

                ModeList.erase(ModeList.begin() + j);
                j--;
            }
        }
    }

    printModes("Kept the best match for each width", ModeList, phys_ar);

#if 0
    // Remove duplicates.
    for (size_t i = 0; i < ModeList.size(); i++)
        for (size_t j = i + 1; j < ModeList.size(); j++)
            if (ModeList[i].width == ModeList[j].width && ModeList[i].height == ModeList[j].height)
            {
                ModeList.erase(ModeList.begin() + j);
                j--;
            }

    printModes("Duplicates removed", ModeList, phys_ar);
#endif

#if 0
    // Remove low-res modes.
    for (size_t i = 0; i < ModeList.size() && ModeList.size() > 3; i++)
        if (ModeList[i].width < 800)
        {
            ModeList.erase(ModeList.begin() + i);
            i--;
        }

    printModes("Low-res modes removed", ModeList, phys_ar);
#endif

    // Remove modes with mismatching aspect ratios.
    for (size_t i = 0; i < ModeList.size() && ModeList.size() > 3; i++)
    {
        const float mi_ar = float(ModeList[i].width) / float(ModeList[i].height);
        const float mi_df = abs(phys_ar - mi_ar);

        if (mi_df >= 0.1f)
        {
            ModeList.erase(ModeList.begin() + i);
            i--;
        }
    }

    printModes("Mismatching aspect ratios removed", ModeList, phys_ar);

    // Assemble result.
    std::string s;
    for (size_t i = 0; i < ModeList.size(); i++)
        s += std::to_string(ModeList[i].width) + " x " + std::to_string(ModeList[i].height) + "\n";
    return s;
}
