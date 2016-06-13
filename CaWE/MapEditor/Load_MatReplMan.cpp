/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Load_MatReplMan.hpp"

#include "../EditorMaterial.hpp"
#include "../EditorMaterialManager.hpp"

#include "TextParser/TextParser.hpp"

#include "wx/confbase.h"
#include "wx/msgdlg.h"


MatReplaceManT::MatReplaceManT(const wxString& OtherGameName_, const ArrayT<EditorMaterialI*>& Materials_)
    : OtherGameName(OtherGameName_)
{
    // If MaterialPrefix!="", it *disables* / *overrides* the substitution that would normally occur!
    MaterialPrefix                      =wxConfigBase::Get()->Read("Map Import/"+OtherGameName+"/Material Prefix",   "");
    const wxString SubstitutePool_String=wxConfigBase::Get()->Read("Map Import/"+OtherGameName+"/Substitute Pool",   "Textures/Industri/ Textures/Frank/ Textures/Kai/");
    const wxString ExceptionList_String =wxConfigBase::Get()->Read("Map Import/"+OtherGameName+"/Replace Materials", "/common/ /editor/ /sfx/ /skies/");

    if (MaterialPrefix=="")
    {
        try
        {
            TextParserT TP_Subst(SubstitutePool_String.c_str(), "", false);

            while (!TP_Subst.IsAtEOF())
            {
                wxString SubstPattern=TP_Subst.GetNextToken().c_str();

                for (unsigned long MatNr=0; MatNr<Materials_.Size(); MatNr++)
                {
                    const wxString& MatName=Materials_[MatNr]->GetName();

                    if (MatName.find(SubstPattern)!=wxString::npos)
                        SubstitutePool.PushBack(MatName);
                }
            }

            TextParserT TP_Excep(ExceptionList_String.c_str(), "", false);
            while (!TP_Excep.IsAtEOF()) ExceptionList.PushBack(TP_Excep.GetNextToken().c_str());
        }
        catch (const TextParserT::ParseError&)
        {
            wxMessageBox("The materials replace expressions could not be parsed.\nNo materials will be replaced.");

            SubstitutePool.Clear();
            ExceptionList.Clear();
        }
    }

    SubstitutePool_NextNr=0;


    // Write the data back in order to make sure that even the default values eventually are written into the config file.
    wxConfigBase::Get()->Write("Map Import/"+OtherGameName+"/Material Prefix",   MaterialPrefix);
    wxConfigBase::Get()->Write("Map Import/"+OtherGameName+"/Substitute Pool",   SubstitutePool_String);
    wxConfigBase::Get()->Write("Map Import/"+OtherGameName+"/Replace Materials", ExceptionList_String);


    if (SubstitutePool.Size()==0)
    {
        return;
    }

    wxMessageBox(OtherGameName+" materials are automatically replaced with Cafu materials.\n\n"+
        "Only Cafu materials whose name contains one of\n    "+SubstitutePool_String+"\n"+
        "are considered for the replacement (the substitution pool).\n\n"+
        OtherGameName+" materials whose name contains one of\n    "+ExceptionList_String+"\n"+
        "are exempted from the material substitution.\n\n"+
        "These settings can (currently only) be changed by editing the CaWE config file.", "Automatic material replacement");
}


wxString MatReplaceManT::GetReplacement(const wxString& D3MaterialName)
{
    if (MaterialPrefix!="") return MaterialPrefix+D3MaterialName;

    if (SubstitutePool.Size()==0) return D3MaterialName;

    // If one of the exceptions is found in D3MaterialName, then just return D3MaterialName.
    for (unsigned long ExceptionNr=0; ExceptionNr<ExceptionList.Size(); ExceptionNr++)
        if (D3MaterialName.find(ExceptionList[ExceptionNr])!=wxString::npos)
            return D3MaterialName;

    // It's not an exception.
    // Try to find D3MaterialName in the list of previously replaced materials next.
    for (unsigned long ReplaceNr=0; ReplaceNr<AlreadyReplaced_OldName.Size(); ReplaceNr++)
        if (AlreadyReplaced_OldName[ReplaceNr]==D3MaterialName)
            return AlreadyReplaced_NewName[ReplaceNr];

    // D3MaterialName has not been replaced before.
    AlreadyReplaced_OldName.PushBack(D3MaterialName);
    AlreadyReplaced_NewName.PushBack(SubstitutePool[SubstitutePool_NextNr]);

    SubstitutePool_NextNr=(SubstitutePool_NextNr+1) % SubstitutePool.Size();

    return AlreadyReplaced_NewName[AlreadyReplaced_NewName.Size()-1];
}
