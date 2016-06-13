/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_TOOL_MANAGER_HPP_INCLUDED
#define CAFU_GUIEDITOR_TOOL_MANAGER_HPP_INCLUDED

#include "Tool.hpp"

#include "Templates/Array.hpp"


namespace GuiEditor
{
    class GuiDocumentT;
    class ChildFrameT;

    class ToolManagerT
    {
        public:

        ToolManagerT(GuiDocumentT* GuiDocument, ChildFrameT* Parent);
        ~ToolManagerT();

        ToolI* GetActiveTool();
        void   SetActiveTool(ToolID ToolID);


        private:

        ArrayT<ToolI*> m_Tools;
        ToolI*         m_ActiveTool;
    };
}

#endif
