/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_APP_CAWE_HPP_INCLUDED
#define CAFU_APP_CAWE_HPP_INCLUDED

#include "wx/cmdline.h"
#include "wx/wx.h"

class ParentFrameT;
class wxFileConfig;


/// This class represents the CaWE application.
class AppCaWE : public wxApp
{
    public:

    AppCaWE();

    bool OnInit();
    int  OnExit();

    // CaWE-specific additional methods.
    ParentFrameT* GetParentFrame() { return m_ParentFrame; }


    private:

    void WriteLuaDoxygenHeaders() const;

    wxLocale*       m_Locale;
    wxCmdLineParser m_CmdLineParser;
    wxFileConfig*   m_FileConfig;
    ParentFrameT*   m_ParentFrame;
};


/// This macro provides the wxGetApp() function, which returns a reference to AppCaWE, for use in other files.
DECLARE_APP(AppCaWE)

#endif
