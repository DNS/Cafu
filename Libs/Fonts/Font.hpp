/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATSYS_FONT_HPP_INCLUDED
#define CAFU_MATSYS_FONT_HPP_INCLUDED

#include <string>


namespace MatSys { class RenderMaterialT; }


/// A class for MatSys-based font rendering.
/// The only requirement is that the MatSys in fully initialized (the global MatSys::Renderer pointer is set)
/// before any object of this class is instantiated.
class FontT
{
    public:

    /// The constructor.
    FontT(const std::string& MaterialName);

    /// The destructor.
    ~FontT();

    /// The copy constructor.
    FontT(const FontT& Other);

    /// The assignment operator.
    FontT& operator = (const FontT& Other);

    /// Prints PrintString at (PosX, PosY) in color Color.
    void Print(int PosX, int PosY, float FrameWidth, float FrameHeight, unsigned long Color, const std::string& PrintString);

    /// Accumulative printing functions. Faster if you have to call Print() a lot.
    void AccPrintBegin(float FrameWidth, float FrameHeight);
    void AccPrint(int PosX, int PosY, unsigned long Color, const std::string& PrintString);
    void AccPrintEnd();


    private:

    MatSys::RenderMaterialT* RenderMaterial;
};

#endif
