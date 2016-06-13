/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_COMMAND_ADD_HPP_INCLUDED
#define CAFU_MODELEDITOR_COMMAND_ADD_HPP_INCLUDED

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

        CommandAddT(ModelDocumentT* ModelDoc, const CafuModelT::SkinT& Skin);
        CommandAddT(ModelDocumentT* ModelDoc, const ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
        CommandAddT(ModelDocumentT* ModelDoc, const ArrayT<CafuModelT::AnimT>& Anims);
        CommandAddT(ModelDocumentT* ModelDoc, const CafuModelT::ChannelT& Channel);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*                 m_ModelDoc;
        const ModelElementTypeT         m_Type;
     // ArrayT<CafuModelT::JointT>      m_Joints;       ///< The added joints       (if m_Type==JOINT).
     // ArrayT<CafuModelT::MeshT>       m_Meshes;       ///< The added meshes       (if m_Type==MESH).
     // ArrayT<MatSys::MeshT>           m_DrawMs;       ///< The draw meshes related to m_Meshes.
        ArrayT<CafuModelT::SkinT>       m_Skins;        ///< The added skins        (if m_Type==SKIN).
        ArrayT<CafuModelT::GuiFixtureT> m_GuiFixtures;  ///< The added GUI fixtures (if m_Type==GFIX).
        ArrayT<CafuModelT::AnimT>       m_Anims;        ///< The added anims        (if m_Type==ANIM).
        ArrayT<CafuModelT::ChannelT>    m_Channels;     ///< The added channels     (if m_Type==CHAN).
    };
}

#endif
