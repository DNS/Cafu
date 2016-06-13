/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_LOAD_MATERIAL_REPL_MAN_HPP_INCLUDED
#define CAFU_LOAD_MATERIAL_REPL_MAN_HPP_INCLUDED

#include "wx/string.h"
#include "Templates/Array.hpp"


class EditorMaterialI;


/// This class manages the replacement of materials in imported maps with native Cafu materials.
/// Materials in the imported map file are replaced by Cafu materials listed in MaterialsReplacePool,
/// except those that contain a string that is mentioned in the ExceptionList.
class MatReplaceManT
{
    public:

    MatReplaceManT(const wxString& OtherGameName_, const ArrayT<EditorMaterialI*>& Materials_);

    wxString GetReplacement(const wxString& D3MaterialName);


    private:

    const wxString   OtherGameName;
    wxString         MaterialPrefix;
    ArrayT<wxString> SubstitutePool;
    ArrayT<wxString> ExceptionList;
    unsigned long    SubstitutePool_NextNr;
    ArrayT<wxString> AlreadyReplaced_OldName;
    ArrayT<wxString> AlreadyReplaced_NewName;
};

#endif
