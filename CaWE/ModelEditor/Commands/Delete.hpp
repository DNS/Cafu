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

#ifndef _MODELEDITOR_COMMAND_DELETE_HPP_
#define _MODELEDITOR_COMMAND_DELETE_HPP_

#include "../../CommandPattern.hpp"
#include "../ElementTypes.hpp"
#include "Models/Loader_cmdl.hpp"


namespace ModelEditor
{
    class CommandSelectT;
    class ModelDocumentT;


    class CommandDeleteT : public CommandT
    {
        public:

        CommandDeleteT(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices);
        ~CommandDeleteT();

        const wxString& GetMessage() const { return m_Message; }

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        struct MeshInfoT
        {
            CafuModelT::MeshT                Mesh;                  ///< The deleted mesh.
            ArrayT<MaterialT*>               SkinsMaterials;        ///< The material in each skin for this mesh.
            ArrayT<MatSys::RenderMaterialT*> SkinsRenderMaterials;  ///< The render material in each skin for this mesh.
        };

        ModelDocumentT*                 m_ModelDoc;
        const ModelElementTypeT         m_Type;
        const ArrayT<unsigned int>      m_Indices;
        ArrayT<CafuModelT::JointT>      m_Joints;           ///< The deleted joints (if m_Type==JOINT).
        ArrayT<MeshInfoT>               m_MeshInfos;        ///< Information about the deleted meshes (if m_Type==MESH).
        ArrayT<CafuModelT::SkinT>       m_Skins;            ///< The deleted skins (if m_Type==SKIN).
        ArrayT<CafuModelT::GuiFixtureT> m_GuiFixtures;      ///< The deleted GUI fixtures (if m_Type==GFIX).
        ArrayT<CafuModelT::AnimT>       m_Anims;            ///< The deleted anims (if m_Type==ANIM).
        ArrayT<CafuModelT::ChannelT>    m_Channels;         ///< The deleted channels (if m_Type==CHAN).
        wxString                        m_Message;          ///< Calling Do() may place an error or info message here that the caller is supposed to show to the user (when the command is first run).
        CommandSelectT*                 m_CommandSelect;    ///< The command that unselects the elements before they are deleted.
    };
}

#endif
