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

#ifndef CAFU_GUISYS_COMPONENT_TRANSFORM_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_TRANSFORM_HPP_INCLUDED

#include "CompBase.hpp"
#include "Window.hpp"   // For the temporary/transitional implementation of the `Get...()` methods.


namespace cf
{
    namespace GuiSys
    {
        /// This components adds information about the position and size of the window;
        /// every window must have exactly one.
        class ComponentTransformT : public ComponentBaseT
        {
            public:

            ComponentTransformT(WindowT* Window);

            // The implementation of these methods is *temporary/transitional*
            // (the data should be here in ComponentTransformT, not in m_Window).
            float GetPosX() const { return GetWindow()->Rect[0]; }
            float GetPosY() const { return GetWindow()->Rect[1]; }
            float GetWidth() const { return GetWindow()->Rect[2]; }
            float GetHeight() const { return GetWindow()->Rect[3]; }
            float GetRotation() const { return GetWindow()->RotAngle; }


            private:

            enum SizeFlagsT { RATIO, FIXED };

            // TODO: Should x, y, w, h be rounded to the nearest integer?? Should we have a flag for it?
            //float x, y, w, h;
            //float RotAngle;          ///< The angle in degrees by how much this entire window is rotated. Obsolete if we have 3D transforms?
            // SizeFlagsT HorzFlags[3];
            // SizeFlagsT VertFlags[3];
        };
    }
}

#endif
