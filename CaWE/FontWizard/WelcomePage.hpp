/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONTWIZARD_WELCOME_PAGE_HPP_INCLUDED
#define CAFU_FONTWIZARD_WELCOME_PAGE_HPP_INCLUDED

#include "wx/wizard.h"


class FontWizardT;


class WelcomePageT : public wxWizardPageSimple
{
    public:

    WelcomePageT(FontWizardT* Parent);
};

#endif
