/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _MODELEDITOR_COMMAND_ADD_HPP_
#define _MODELEDITOR_COMMAND_ADD_HPP_

#include "../../CommandPattern.hpp"
#include "../ElementTypes.hpp"
#include "Models/Loader_cmdl.hpp"


namespace ModelEditor
{
    class CommandSelectT;
    class ModelDocumentT;


    class CommandAddT : public CommandT
    {
        public:

        CommandAddT(ModelDocumentT* ModelDoc, const ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*                 m_ModelDoc;
        const ModelElementTypeT         m_Type;
     // ArrayT<CafuModelT::JointT>      m_Joints;       ///< The added joints (if m_Type==JOINT).
     // ArrayT<CafuModelT::MeshT>       m_Meshes;       ///< The added meshes (if m_Type==MESH).
     // ArrayT<MatSys::MeshT>           m_DrawMs;       ///< The draw meshes related to m_Meshes.
     // ArrayT<CafuModelT::AnimT>       m_Anims;        ///< The added anims (if m_Type==ANIM).
        ArrayT<CafuModelT::GuiFixtureT> m_GuiFixtures;  ///< The added GUI fixtures (if m_Type==GFIX).
    };
}

#endif
