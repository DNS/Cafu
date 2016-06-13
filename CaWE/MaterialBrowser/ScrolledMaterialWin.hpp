/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCROLLED_MATERIAL_WIN_HPP_INCLUDED
#define CAFU_SCROLLED_MATERIAL_WIN_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "wx/scrolwin.h"


class EditorMaterialI;


namespace MaterialBrowser
{
    class DialogT;


    class ScrolledMaterialWindowT : public wxScrolledWindow
    {
        public:

        // Constructor.
        ScrolledMaterialWindowT(DialogT* Parent, wxWindowID OurID, const ArrayT<EditorMaterialI*>& Materials);

        // Recomputes the size of the virtual window.
        void UpdateVirtualSize();

        // If NewMat==NULL, this function does nothing.
        // Otherwise NewMat is selected as the new current material, the material description text is updated,
        // the window scrolled to the selected material, and finally the window is redrawn.
        void SelectMaterial(EditorMaterialI* NewMat);

        /// A helper function exclusively for DialogT::OnButton_ExportDiffMaps().
        void ExportDiffuseMaps(const wxString& DestinationDir) const;


        private:

        struct TexPosEnumT
        {
            unsigned long    Index;         // Current index number of this material position.
            EditorMaterialI* Mat;           // Pointer to material.
            wxRect           MatRect;       // The coordinates of this material position in the virtual window.
            int              CurrentX;      // The current position. The next material position will be here.
            int              CurrentY;
            int              LargestY;      // The largest y-coordinate encountered so far. Needed to know where the next row starts.
            wxSize           ClientSize;    // The size of the client area (the visible fraction of the virtual window).
        };


        static const int SCROLL_INCREMENT_X;
        static const int SCROLL_INCREMENT_Y;
        static const int Padding;
        static const int MaterialNameBoxHeight;

        DialogT*                        m_Parent;
        const ArrayT<EditorMaterialI*>& m_Materials;


        TexPosEnumT* EnumTexturePositions(bool Start) const;

        void OnDraw(wxDC& dc);                              // Overridden event handler.
        void OnSize(wxSizeEvent& Event);                    // My own event handler for resize events.
        void OnLeftButtonDown(wxMouseEvent& Event);         // Left mouse button click handler.
        void OnLeftButtonDoubleClick(wxMouseEvent& Event);  // Left mouse button double click handler.

        DECLARE_EVENT_TABLE()
    };
}

#endif
