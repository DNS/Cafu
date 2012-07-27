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

#include "GuiResources.hpp"
#include "ConsoleCommands/Console.hpp"
#include "Fonts/FontTT.hpp"
#include "Models/ModelManager.hpp"
#include "TextParser/TextParser.hpp"


using namespace cf::GuiSys;


GuiResourcesT::GuiResourcesT(ModelManagerT& ModelMan)
    : m_ModelMan(ModelMan)
{
}


GuiResourcesT::~GuiResourcesT()
{
    for (unsigned long FontNr=0; FontNr<m_Fonts.Size(); FontNr++)
        delete m_Fonts[FontNr];
}


cf::TrueTypeFontT* GuiResourcesT::GetFont(const std::string& FontName)
{
    // See if FontName has been loaded successfully before.
    for (unsigned long FontNr=0; FontNr<m_Fonts.Size(); FontNr++)
        if (m_Fonts[FontNr]->GetName()==FontName)
            return m_Fonts[FontNr];

    // See if FontName has been loaded UNsuccessfully before.
 // for (unsigned long FontNr=0; FontNr<m_FontsFailed.Size(); FontNr++)
 //     if (m_FontsFailed[FontNr]==FontName)
 //         return NULL;

    // FontName has never been attempted to be loaded, try now.
    try
    {
        m_Fonts.PushBack(new cf::TrueTypeFontT(FontName));
        return m_Fonts[m_Fonts.Size()-1];
    }
    catch (const TextParserT::ParseError&) { }

    Console->Warning(std::string("Failed to load font \"")+FontName+"\".\n");
 // FontsFailed.PushBack(FontName);
    return m_Fonts.Size()>0 ? m_Fonts[0] : NULL;
}


const CafuModelT* GuiResourcesT::GetModel(const std::string& FileName, std::string& ErrorMsg)
{
    return m_ModelMan.GetModel(FileName, &ErrorMsg);
}
