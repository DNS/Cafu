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

#ifndef CAFU_GUISYS_COMPONENT_MODEL_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_MODEL_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        class ComponentTransformT;

        /// This component adds a 3D model to its window.
        class ComponentModelT : public ComponentBaseT
        {
            public:

            /// The constructor.
            /// @param Window   The window that the new component becomes a part of.
            ComponentModelT(WindowT& Window);

            /// The copy constructor.
            /// The new component can become a part of the same or a different window than the component it was copied from.
            /// @param Comp     The component to create a copy of.
            /// @param Window   The window that the new component becomes a part of.
            ComponentModelT(const ComponentModelT& Comp, WindowT& Window);

            // Base class overrides.
            ComponentModelT* Clone(WindowT& Window) const;
            void ResolveDependencies();


            private:

            ComponentTransformT*        m_Transform;
            // ...
        };
    }
}

#endif
