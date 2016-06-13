/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
