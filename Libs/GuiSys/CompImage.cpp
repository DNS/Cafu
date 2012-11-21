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

#include "CompImage.hpp"
#include "CompTransform.hpp"
#include "Window.hpp"

using namespace cf::GuiSys;


ComponentImageT::ComponentImageT(WindowT* Window)
    : ComponentBaseT(Window),
      m_Transform(NULL)
{
}


void ComponentImageT::ResolveDependencies()
{
    // It would be possible to break this loop as soon as we have assigned a non-NULL pointer to m_Transform.
    // However, this is only because the Transform component is, at this time, the only sibling component that
    // we're interested in, whereas the loop below is suitable for resolving additional dependencies, too.
    for (unsigned int CompNr = 0; CompNr < GetWindow()->GetComponents().Size(); CompNr++)
    {
        ComponentBaseT* Comp = GetWindow()->GetComponents()[CompNr];

        if (!m_Transform)
            m_Transform = dynamic_cast<ComponentTransformT*>(Comp);
    }
}
