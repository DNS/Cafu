/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_UPDATE_GUI_FIXTURE_HPP_INCLUDED
#define CAFU_MODELEDITOR_UPDATE_GUI_FIXTURE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Models/Model_cmdl.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandUpdateGuiFixtureT : public CommandT
    {
        public:

        CommandUpdateGuiFixtureT(ModelDocumentT* ModelDoc, unsigned int GFNr, const CafuModelT::GuiFixtureT& GF);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*               m_ModelDoc;
        const unsigned int            m_GFNr;
        const CafuModelT::GuiFixtureT m_NewGF;
        const CafuModelT::GuiFixtureT m_OldGF;
    };
}

#endif
