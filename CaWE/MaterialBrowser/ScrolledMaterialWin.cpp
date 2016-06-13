/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ScrolledMaterialWin.hpp"

#include "MaterialBrowserDialog.hpp"
#include "ControlsBar.hpp"
#include "FilterSettings.hpp"

#include "../EditorMaterial.hpp"

#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Bitmap/Bitmap.hpp"

#include "wx/tokenzr.h"

// Turn off bogus warnings that occur with VC11's static code analysis.
// (Should move this to a better place though, e.g. some `compat.h` file...)
#if defined(_WIN32) && defined(_MSC_VER)
    // warning C28182: Dereferencing NULL pointer.
    #pragma warning(disable:28182)
#endif


using namespace MaterialBrowser;


BEGIN_EVENT_TABLE(ScrolledMaterialWindowT, wxScrolledWindow)
    EVT_SIZE(ScrolledMaterialWindowT::OnSize)
    EVT_LEFT_DOWN(ScrolledMaterialWindowT::OnLeftButtonDown)
    EVT_LEFT_DCLICK(ScrolledMaterialWindowT::OnLeftButtonDoubleClick)
END_EVENT_TABLE()


const int ScrolledMaterialWindowT::SCROLL_INCREMENT_X   = 1;
const int ScrolledMaterialWindowT::SCROLL_INCREMENT_Y   =64;
const int ScrolledMaterialWindowT::Padding              = 4;
const int ScrolledMaterialWindowT::MaterialNameBoxHeight=14;


ScrolledMaterialWindowT::ScrolledMaterialWindowT(DialogT* Parent, wxWindowID OurID, const ArrayT<EditorMaterialI*>& Materials)
    : wxScrolledWindow(Parent, OurID, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL|wxSUNKEN_BORDER),
      m_Parent(Parent),
      m_Materials(Materials)
{
    // It seems that somebody exchanged x and y in the wxScrolledWindow constructor:
    // wxSize(200, -1) sets a minimum y size, wxSize(-1, 200) does not... so let's simply use wxSize(200, 200) and be safe.

    // Don't call UpdateVirtualSize() here, because it accesses members from the parent
    // that are not yet initialized at this point. The parent calls UpdateVirtualSize() anyway.
}


void ScrolledMaterialWindowT::UpdateVirtualSize()
{
    int TotalX=0;
    int TotalY=0;

    for (TexPosEnumT* TPE=EnumTexturePositions(true); TPE; TPE=EnumTexturePositions(false))
    {
        // wxLogDebug("TPE->MatRect right and bottom   %i %i   ClientSize  %i %i", TPE->MatRect.GetRight(), TPE->MatRect.GetBottom(), TPE->ClientSize.GetWidth(), TPE->ClientSize.GetHeight());

        if (TPE->MatRect.GetRight() >TotalX) TotalX=TPE->MatRect.GetRight();
        if (TPE->MatRect.GetBottom()>TotalY) TotalY=TPE->MatRect.GetBottom();
    }

    TotalX+=Padding;
    TotalY+=Padding;

    // wxLogDebug("SetVirtualSize %i %i", TotalX, TotalY);
    SetScrollbars(SCROLL_INCREMENT_X, SCROLL_INCREMENT_Y, ceil(float(TotalX)/float(SCROLL_INCREMENT_X)), ceil(float(TotalY)/float(SCROLL_INCREMENT_Y)));
}


// Keywords is a string that contains several keywords, separated by space, comma or semicolon.
// If all those keywords occur in TestString, true is returned, false otherwise.
static bool MatchKeywords(const wxString& TestString, const wxString& Keywords)
{
    static wxString         LastSeenKeywordString="";
    static ArrayT<wxString> DecomposedKeywords;

    if (Keywords!=LastSeenKeywordString)
    {
        wxStringTokenizer Tokenizer(Keywords, " ,;", wxTOKEN_STRTOK);

        DecomposedKeywords.Clear();
        while (Tokenizer.HasMoreTokens()) DecomposedKeywords.PushBack(Tokenizer.GetNextToken().Lower());

        LastSeenKeywordString=Keywords;
    }

    for (unsigned long Nr=0; Nr<DecomposedKeywords.Size(); Nr++)
        if (TestString.Lower().Find(DecomposedKeywords[Nr])==-1)
            return false;

    return true;
}


ScrolledMaterialWindowT::TexPosEnumT* ScrolledMaterialWindowT::EnumTexturePositions(bool Start) const
{
    static TexPosEnumT TPE;

    if (Start)
    {
        // Start the enumeration - initialize.
        TPE.Index     =0;
        TPE.Mat       =NULL;
        TPE.MatRect   =wxRect(0, 0, 0, 0);
        TPE.CurrentX  =Padding;
        TPE.CurrentY  =Padding;
        TPE.LargestY  =Padding;
        TPE.ClientSize=GetClientSize();
    }


    while (true)
    {
        if (TPE.Index>=m_Materials.Size()) return NULL;

        TPE.Mat=m_Materials[TPE.Index];
        TPE.Index++;

        // Only show editor materials if filter option is checked.
        if (!TPE.Mat->ShowInMaterialBrowser() && m_Parent->m_FilterSettings->OnlyShowEditorMats()) continue;

        // Filter by used materials list.
        if (m_Parent->m_UsedMaterialsList!=NULL)
            if (m_Parent->m_UsedMaterialsList->Find(TPE.Mat)==-1) continue;

        // Filter by folder name.
        if (!TPE.Mat->GetName().StartsWith(m_Parent->MatFolderFilter)) continue;

        // Filter by material name.
        if (!MatchKeywords(TPE.Mat->GetName(), m_Parent->m_FilterSettings->GetNameFilterValue())) continue;

        // Found a material that survived all filtering.
        break;
    }


    TPE.MatRect=wxRect(TPE.CurrentX, TPE.CurrentY, m_Parent->DisplaySize<=0 ? TPE.Mat->GetWidth () : m_Parent->DisplaySize,
                                                   m_Parent->DisplaySize<=0 ? TPE.Mat->GetHeight() : m_Parent->DisplaySize);

    // If we've got one material on this row already, and this one goes out of the client area, jump to the next row.
    if (TPE.MatRect.GetLeft()>Padding && TPE.MatRect.GetRight()+Padding>TPE.ClientSize.GetWidth())
    {
        TPE.CurrentX=Padding;
        TPE.CurrentY=TPE.LargestY+Padding;

        TPE.MatRect=wxRect(TPE.CurrentX, TPE.CurrentY, m_Parent->DisplaySize<=0 ? TPE.Mat->GetWidth () : m_Parent->DisplaySize,
                                                       m_Parent->DisplaySize<=0 ? TPE.Mat->GetHeight() : m_Parent->DisplaySize);
    }

    TPE.MatRect.height+=MaterialNameBoxHeight;
    if (TPE.MatRect.GetBottom()>TPE.LargestY) TPE.LargestY=TPE.MatRect.GetBottom();
    TPE.CurrentX=TPE.MatRect.GetRight()+Padding;

    return &TPE;
}


void ScrolledMaterialWindowT::SelectMaterial(EditorMaterialI* NewMat)
{
    // Explicitly check NewMat for NULL.
    if (!NewMat) return;

    for (TexPosEnumT* TPE=EnumTexturePositions(true); TPE; TPE=EnumTexturePositions(false))
        if (TPE->Mat==NewMat)
        {
            int ViewStartX;
            int ViewStartY;
            GetViewStart(&ViewStartX, &ViewStartY);

            const int ViewEndY=ViewStartY+GetClientSize().y/SCROLL_INCREMENT_Y;

            const int y1=TPE->MatRect.GetTop   ()/SCROLL_INCREMENT_Y;
            const int y2=TPE->MatRect.GetBottom()/SCROLL_INCREMENT_Y+1;

                 if (y1<ViewStartY) Scroll(-1, y1);
            else if (y2>ViewEndY  ) Scroll(-1, y2-ViewEndY+ViewStartY);

            Refresh();
            return;
        }
}


void ScrolledMaterialWindowT::ExportDiffuseMaps(const wxString& DestinationDir) const
{
    if (DestinationDir=="") return;

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    wxBusyCursor BusyCursor;

    for (TexPosEnumT* TPE=EnumTexturePositions(true); TPE; TPE=EnumTexturePositions(false))
    {
        wxString MatName=TPE->Mat->GetName();

#if 0
        MatName.Replace("/",  "-");
        MatName.Replace("\\", "-");

        TPE->Mat->GetImage().SaveFile(DestinationDir+"/"+MatName+".png", wxBITMAP_TYPE_PNG);
#else
        MatName=MatName.AfterLast('/');
        MatName=MatName.AfterLast('\\');

        MatSys::RenderMaterialT* RenderMat=TPE->Mat->GetRenderMaterial(true); wxASSERT(RenderMat!=NULL); if (RenderMat==NULL) continue;
        const MaterialT*         Material =MatSys::Renderer->GetMaterialFromRM(RenderMat);

        if (!Material->DiffMapComp.IsEmpty())
        {
            BitmapT* Bitmap=Material->DiffMapComp.GetBitmap();

            Bitmap->SaveToDisk((DestinationDir+"/"+MatName+"_diff.png").c_str());
            delete Bitmap;
        }
        else
        {
            // Do *NOT* issue a message box here, or else a redraw (paint event) is triggered,
            // which in turn invalidates our TPE count!!
            // wxMessageBox(MatName+" has no diffuse-map!");

            // Just do this instead:
            TPE->Mat->GetImage().SaveFile(DestinationDir+"/"+MatName+"_diff.png", wxBITMAP_TYPE_PNG);
        }

        if (!Material->NormMapComp.IsEmpty())
        {
            BitmapT* Bitmap=Material->NormMapComp.GetBitmap();

            Bitmap->SaveToDisk((DestinationDir+"/"+MatName+"_norm.png").c_str());
            delete Bitmap;

            if (Material->NormMapComp.GetType()==MapCompositionT::CombineNormals)
            {
                int Count=0;

                // This normal-map has been composed of either two normal-maps (which in turn may have been converted from a height-map).
                // Now also save the individual maps out.
                for (int ChildNr=0; ChildNr<2; ChildNr++)
                {
                    const MapCompositionT* Child =Material->NormMapComp.GetChild(ChildNr);
                    wxString               Suffix="_norm_clean";

                    if (Child->GetType()==MapCompositionT::HeightMapToNormalMap)
                    {
                        Child =Child->GetChild(0);
                        Suffix="_bump";
                        Count++;
                    }
                    else Count--;

                    BitmapT* ChildBitmap=Child->GetBitmap();

                    ChildBitmap->SaveToDisk((DestinationDir+"/"+MatName+Suffix+".png").c_str());
                    delete ChildBitmap;
                }

                if (Count!=0) wxMessageBox("Count!=0 for "+MatName, "Warning!");
            }
        }

        if (!Material->SpecMapComp.IsEmpty())
        {
            BitmapT* Bitmap=Material->SpecMapComp.GetBitmap();

            Bitmap->SaveToDisk((DestinationDir+"/"+MatName+"_spec.png").c_str());
            delete Bitmap;
        }

        if (!Material->LumaMapComp.IsEmpty())
        {
            BitmapT* Bitmap=Material->LumaMapComp.GetBitmap();

            Bitmap->SaveToDisk((DestinationDir+"/"+MatName+"_luma.png").c_str());
            delete Bitmap;
        }
#endif
    }
}


void ScrolledMaterialWindowT::OnDraw(wxDC& dc)
{
    wxPen   NameBoxFramePen (wxColor(0, 0, 128), 1, wxPENSTYLE_SOLID);
    wxBrush NameBoxFillBrush(wxColor(0, 0, 128),    wxBRUSHSTYLE_SOLID);

    wxPen   SelectionIndicatorFramePen (wxColor(255, 255, 255), 1, wxPENSTYLE_SOLID);
    wxBrush SelectionIndicatorFillBrush(wxColor(  0,   0,   0),    wxBRUSHSTYLE_TRANSPARENT);

    // Determine the visible portion of the scroll window.
    int ViewStartX;
    int ViewStartY;

    GetViewStart(&ViewStartX, &ViewStartY);

    wxRect ViewRect(wxPoint(ViewStartX*SCROLL_INCREMENT_X, ViewStartY*SCROLL_INCREMENT_Y), GetClientSize());

    // Clear the background to black.
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();

    for (TexPosEnumT* TPE=EnumTexturePositions(true); TPE; TPE=EnumTexturePositions(false))
    {
        if (ViewRect.Intersects(TPE->MatRect))
        {
            dc.SetPen(NameBoxFramePen);                     // Name box frame color.
            dc.SetBrush(NameBoxFillBrush);                  // Name box fill  color.

            dc.SetFont(*wxNORMAL_FONT);
            dc.SetTextForeground(wxColor(255, 255, 255));   // Text color for name box.
            dc.SetBackgroundMode(wxTRANSPARENT);            // Text gets transparent background.

            TPE->Mat->Draw(dc, TPE->MatRect, MaterialNameBoxHeight, true /*DrawNameBox*/);
        }

        if (TPE->Mat==m_Parent->m_CurrentMaterial)
        {
            dc.SetPen(SelectionIndicatorFramePen);
            dc.SetBrush(SelectionIndicatorFillBrush);
            dc.DrawRectangle(TPE->MatRect.x-2, TPE->MatRect.y-2, TPE->MatRect.width+4, TPE->MatRect.height+4);
        }
    }

    // For testing, this draws a line diagonally across the screen.
    // dc.DrawLine(ViewRect.x, ViewRect.y, ViewRect.GetRight(), ViewRect.GetBottom());

    dc.SetPen(wxNullPen);
    dc.SetBrush(wxNullBrush);
}


void ScrolledMaterialWindowT::OnSize(wxSizeEvent& Event)
{
    // wxLogDebug("OnSize Event!\n");
    UpdateVirtualSize();
    SelectMaterial(m_Parent->m_CurrentMaterial);
}


void ScrolledMaterialWindowT::OnLeftButtonDown(wxMouseEvent& Event)
{
    wxClientDC dc(this);
    PrepareDC(dc);
    wxPoint Pos=Event.GetLogicalPosition(dc);
    TexPosEnumT* TPE;

    Event.Skip();   // Enable default event handling, e.g. for keyboard focus setting.

    for (TPE=EnumTexturePositions(true); TPE; TPE=EnumTexturePositions(false))
        if (TPE->MatRect.Contains(Pos)) break;

    if (TPE==NULL) return;

    m_Parent->SelectMaterial(TPE->Mat);
}


void ScrolledMaterialWindowT::OnLeftButtonDoubleClick(wxMouseEvent& Event)
{
    m_Parent->SaveAndQuitDialog(wxID_OK);
}
