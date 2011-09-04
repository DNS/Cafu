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

#include "ModifyWindow.hpp"

#include "../GuiDocument.hpp"
#include "../EditorData/Window.hpp"

#include "GuiSys/GuiMan.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/WindowChoice.hpp"
#include "GuiSys/WindowModel.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "Fonts/FontTT.hpp"
#include "TextParser/TextParser.hpp"


using namespace GuiEditor;


CommandModifyWindowT::CommandModifyWindowT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, const wxString& PropertyName, cf::GuiSys::WindowT::MemberVarT& MemberVar, const wxString& NewValue)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_PropertyName(PropertyName),
      m_MemberVar(MemberVar),
      m_NewString(std::string(NewValue)),
      m_NewInt(0),
      m_OldString(m_MemberVar.Member!=NULL ? *(std::string*)m_MemberVar.Member : ""), // The member of string properties can be NULL when these are specially treated properties.
      m_OldInt(0)
{
    for (int i=0; i<4; i++)
    {
        m_NewFloat[i]=0.0f;
        m_OldFloat[i]=0.0f;
    }
}


CommandModifyWindowT::CommandModifyWindowT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, const wxString& PropertyName, cf::GuiSys::WindowT::MemberVarT& MemberVar, const float* NewValue)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_PropertyName(PropertyName),
      m_MemberVar(MemberVar),
      m_NewString(""),
      m_NewInt(0),
      m_OldString(""),
      m_OldInt(0)
{
    int Max=4;

    if (m_MemberVar.Type==cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT)  Max=1;
    if (m_MemberVar.Type==cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT2) Max=2;

    for (int i=0; i<Max; i++)
    {
        m_NewFloat[i]=NewValue[i];
        m_OldFloat[i]=((float*)m_MemberVar.Member)[i];
    }

    // Fill the remaining unused float values.
    for (int i=Max; i<4; i++)
    {
        m_NewFloat[i]=0.0f;
        m_OldFloat[i]=0.0f;
    }
}


CommandModifyWindowT::CommandModifyWindowT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, const wxString& PropertyName, cf::GuiSys::WindowT::MemberVarT& MemberVar, const int NewValue)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_PropertyName(PropertyName),
      m_MemberVar(MemberVar),
      m_NewString(""),
      m_NewInt(NewValue),
      m_OldString(""),
      m_OldInt(0) // Can't set this value directly here since int is used for real int values as well as bool values.
{
    for (int i=0; i<4; i++)
    {
        m_NewFloat[i]=0.0f;
        m_OldFloat[i]=0.0f;
    }

    if (m_MemberVar.Type==cf::GuiSys::WindowT::MemberVarT::TYPE_BOOL)
        m_OldInt=(*(bool*)m_MemberVar.Member) ? 1 : 0;
    else
        m_OldInt=*(int*)m_MemberVar.Member;
}


bool CommandModifyWindowT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    if (m_PropertyName=="Name")
    {
        m_OldString=m_Window->Name;

        if (!((EditorDataWindowT*)m_Window->EditorData)->SetName(m_NewString))
        {
            m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_PROPERTY_CHANGED, m_PropertyName); // This is needed to reset the property grid correctly.
            return false;
        }
    }
    else if (m_PropertyName=="BackMatName")
    {
        m_OldString=m_Window->BackRenderMatName;

        m_Window->BackRenderMatName=m_NewString;
        MatSys::Renderer->FreeMaterial(m_Window->BackRenderMat);
        m_Window->BackRenderMat=m_Window->BackRenderMatName.empty() ? NULL : MatSys::Renderer->RegisterMaterial(m_GuiDocument->GetGui()->GetMaterialManager().GetMaterial(m_Window->BackRenderMatName));
    }
    else if (m_PropertyName=="FontName")
    {
        m_OldString=m_Window->Font->GetName();

        m_Window->Font=cf::GuiSys::GuiMan->GetFont(m_NewString);
    }
    else if (m_PropertyName=="Choices")
    {
        cf::GuiSys::ChoiceT* ChoiceWindow=dynamic_cast<cf::GuiSys::ChoiceT*>(m_Window);
        wxASSERT(ChoiceWindow!=NULL);

        m_OldString="";

        ArrayT<std::string>& Choices=ChoiceWindow->GetChoices();

        for (unsigned long ChoiceNr=0; ChoiceNr<Choices.Size(); ChoiceNr++)
            m_OldString+=Choices[ChoiceNr]+";";

        // Parse choice list and update the window choices.
        TextParserT ChoiceParser(m_NewString.c_str(), ";", false);
        Choices.Clear();

        try
        {
            while (!ChoiceParser.IsAtEOF())
            {
                Choices.PushBack(ChoiceParser.GetNextToken());
                ChoiceParser.AssertAndSkipToken(";");
            }
        }
        catch (const TextParserT::ParseError&)
        {
            // Undo all changes made up to this point.
            try
            {
                TextParserT ChoiceParser(m_OldString.c_str(), ";", false);
                Choices.Clear();

                while (!ChoiceParser.IsAtEOF())
                {
                    Choices.PushBack(ChoiceParser.GetNextToken());
                    ChoiceParser.AssertAndSkipToken(";");
                }
            }
            catch (const TextParserT::ParseError&)
            {
                wxASSERT(false);
                return false;
            }

            return false;
        }
    }
    else if (m_PropertyName=="Model")
    {
        cf::GuiSys::ModelWindowT* ModelWindow=dynamic_cast<cf::GuiSys::ModelWindowT*>(m_Window);
        wxASSERT(ModelWindow!=NULL);

        ModelProxyT& Model=ModelWindow->GetModel();

        m_OldString=Model.GetFileName();

        Model=ModelProxyT(m_NewString);
    }
    else
    {
        switch (m_MemberVar.Type)
        {
            case cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT:
                *(float*)m_MemberVar.Member=m_NewFloat[0];
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT2:
                ((float*)m_MemberVar.Member)[0]=m_NewFloat[0];
                ((float*)m_MemberVar.Member)[1]=m_NewFloat[1];
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT4:
                ((float*)m_MemberVar.Member)[0]=m_NewFloat[0];
                ((float*)m_MemberVar.Member)[1]=m_NewFloat[1];
                ((float*)m_MemberVar.Member)[2]=m_NewFloat[2];
                ((float*)m_MemberVar.Member)[3]=m_NewFloat[3];
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_INT:
                *(int*)m_MemberVar.Member=m_NewInt;
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_BOOL:
                *(bool*)m_MemberVar.Member=m_NewInt>0;
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_STRING:
                *(std::string*)m_MemberVar.Member=m_NewString;
                break;
        }
    }

    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_PROPERTY_CHANGED, m_PropertyName);

    m_Done=true;

    return true;
}


void CommandModifyWindowT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    if (m_PropertyName=="Name")
    {
        m_Window->Name=m_OldString;
    }
    else if (m_PropertyName=="BackMatName")
    {
        m_Window->BackRenderMatName=m_OldString;
        MatSys::Renderer->FreeMaterial(m_Window->BackRenderMat);
        m_Window->BackRenderMat=m_Window->BackRenderMatName.empty() ? NULL : MatSys::Renderer->RegisterMaterial(m_GuiDocument->GetGui()->GetMaterialManager().GetMaterial(m_Window->BackRenderMatName));
    }
    else if (m_PropertyName=="FontName")
    {
        m_Window->Font=cf::GuiSys::GuiMan->GetFont(m_OldString);
    }
    else if (m_PropertyName=="Choices")
    {
        cf::GuiSys::ChoiceT* ChoiceWindow=dynamic_cast<cf::GuiSys::ChoiceT*>(m_Window);
        wxASSERT(ChoiceWindow!=NULL);

        ArrayT<std::string>& Choices=ChoiceWindow->GetChoices();

        // Parse choice list and update the window choices.
        TextParserT ChoiceParser(m_OldString.c_str(), ";", false);
        Choices.Clear();

        try
        {
            while (!ChoiceParser.IsAtEOF())
            {
                Choices.PushBack(ChoiceParser.GetNextToken());
                ChoiceParser.AssertAndSkipToken(";");
            }
        }
        catch (const TextParserT::ParseError&)
        {
            wxASSERT(false);
        }
    }
    else if (m_PropertyName=="Model")
    {
        cf::GuiSys::ModelWindowT* ModelWindow=dynamic_cast<cf::GuiSys::ModelWindowT*>(m_Window);
        wxASSERT(ModelWindow!=NULL);

        ModelWindow->GetModel()=ModelProxyT(m_OldString);
    }
    else
    {
        switch (m_MemberVar.Type)
        {
            case cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT:
                *(float*)m_MemberVar.Member=m_OldFloat[0];
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT2:
                ((float*)m_MemberVar.Member)[0]=m_OldFloat[0];
                ((float*)m_MemberVar.Member)[1]=m_OldFloat[1];
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_FLOAT4:
                ((float*)m_MemberVar.Member)[0]=m_OldFloat[0];
                ((float*)m_MemberVar.Member)[1]=m_OldFloat[1];
                ((float*)m_MemberVar.Member)[2]=m_OldFloat[2];
                ((float*)m_MemberVar.Member)[3]=m_OldFloat[3];
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_INT:
                *(int*)m_MemberVar.Member=m_OldInt;
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_BOOL:
                *(bool*)m_MemberVar.Member=m_OldInt>0;
                break;

            case cf::GuiSys::WindowT::MemberVarT::TYPE_STRING:
                *(std::string*)m_MemberVar.Member=m_OldString;
                break;
        }
    }

    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_PROPERTY_CHANGED, m_PropertyName);

    m_Done=false;
}


wxString CommandModifyWindowT::GetName() const
{
    return "Modify "+m_PropertyName;
}
