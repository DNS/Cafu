/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_SET_MATERIAL_HPP_INCLUDED
#define CAFU_MODELEDITOR_SET_MATERIAL_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MaterialT;
namespace MatSys { class RenderMaterialT; }


namespace ModelEditor
{
    class ModelDocumentT;

    /// This command sets a new material for a given mesh in a given skin.
    class CommandSetMaterialT : public CommandT
    {
        public:

        CommandSetMaterialT(ModelDocumentT* ModelDoc, unsigned int MeshNr, int SkinNr, const wxString& NewName);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        MaterialT*&               GetMaterial();
        MatSys::RenderMaterialT*& GetRenderMaterial();

        ModelDocumentT*    m_ModelDoc;
        const unsigned int m_MeshNr;
        const int          m_SkinNr;
        MaterialT*         m_NewMat;
        MaterialT*         m_OldMat;
    };
}

#endif
