/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "wx/wx.h"
#include "EditorMaterialEngine.hpp"
#include "Bitmap/Bitmap.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Vector3.hpp"


EngineMaterialT::EngineMaterialT(MaterialT* MatSysMaterial_)
    : Name(MatSysMaterial_->Name.c_str()),
      MatSysMaterial(MatSysMaterial_),
      MatSysRenderMaterial_Normal(NULL),
      MatSysRenderMaterial_Editor(NULL),
      Material_Editor(NULL),
      m_BrowserImage(NULL)
{
}


EngineMaterialT::~EngineMaterialT()
{
    // Unfortunately, when we get here, the MatSys is long gone (so we delete here something that doesn't exist any more)...
    // MatSys::Renderer->FreeMaterial(MatSysRenderMaterial_Normal);
    // MatSysRenderMaterial_Normal=NULL;

    // MatSys::Renderer->FreeMaterial(MatSysRenderMaterial_Editor);
    // MatSysRenderMaterial_Editor=NULL;

    delete Material_Editor;

    delete m_BrowserImage;
    m_BrowserImage=NULL;
}


int EngineMaterialT::GetWidth() const  { return GetImage().GetWidth();  }
int EngineMaterialT::GetHeight() const { return GetImage().GetHeight(); }


void EngineMaterialT::Draw(wxDC& dc, const wxRect& DestRect, int NameBoxHeight, bool DrawNameBox) const
{
    const bool FlagAlphaTest=MatSysMaterial->AlphaTestValue.Evaluate(ExpressionT::SymbolsT()).GetAsFloat()>=0.0f;
    const bool FlagBlend    =IsTranslucent();

    wxString FlagsString="";

    if (FlagAlphaTest || FlagBlend)
    {
        FlagsString="[";
        if (FlagAlphaTest) FlagsString+="A";
        if (FlagBlend    ) FlagsString+="B";
        FlagsString+="] ";

        dc.SetTextForeground(wxColor(255, 255, 0));   // Text color for name box.
    }

    // Start with drawing the name box.
    wxRect NameBoxRect=DestRect;

    NameBoxRect.y     =DestRect.GetBottom()-NameBoxHeight;
    NameBoxRect.height=NameBoxHeight;

    dc.DrawRectangle(NameBoxRect);
    dc.DrawText(FlagsString+Name, NameBoxRect.x+4, NameBoxRect.y+1);

    // Can unfortunately not draw anything scaled... (so only the 1:1 display size works correctly for now).
    dc.DrawBitmap(wxBitmap(GetImage()), DestRect.x, DestRect.y, false);
}


const wxImage& EngineMaterialT::GetImage() const
{
    if (m_BrowserImage==NULL)
    {
        // Create a browser image from the meta_EditorImage if valid or use the diffuse map.
        BitmapT* Bitmap=(MatSysMaterial->meta_EditorImage.IsEmpty() || MatSysMaterial->meta_EditorImage.GetString()=="noEditor")
                        ? MatSysMaterial->DiffMapComp.GetBitmap()
                        : MatSysMaterial->meta_EditorImage.GetBitmap();

        m_BrowserImage=new wxImage(Bitmap->SizeX, Bitmap->SizeY);

        for (unsigned int y=0; y<Bitmap->SizeY; y++)
            for (unsigned int x=0; x<Bitmap->SizeX; x++)
            {
                int r, g, b;

                Bitmap->GetPixel(x, y, r, g, b);
                m_BrowserImage->SetRGB(x, y, r, g, b);
            }

        delete Bitmap;

#if 0
        if (MatSysMaterial->meta_EditorImage.IsEmpty() && !MatSysMaterial->NormMapComp.IsEmpty())
        {
            BitmapT* NormalMap=MatSysMaterial->NormMapComp.GetBitmap();

            NormalMap->Scale(m_BrowserImage->GetWidth(), m_BrowserImage->GetHeight());

            for (int y=0; y<m_BrowserImage->GetHeight(); y++)
                for (int x=0; x<m_BrowserImage->GetWidth(); x++)
                {
                    int Nx, Ny, Nz;
                    NormalMap->GetPixel(x, y, Nx, Ny, Nz);

                    const Vector3fT Normal  =normalize(Vector3fT(Nx/255.0f-0.5f, Ny/255.0f-0.5f, Nz/255.0f-0.5f)*2.0f, 0.0f);
                    const Vector3fT LightDir=normalize(Vector3fT(-5.0f, -8.0f, 20.0f), 0.0f);
                    float           Factor  =dot(Normal, LightDir);

                    if (Factor<0.0f) Factor=0.0f;

                    int r=m_BrowserImage->GetRed  (x, y);
                    int g=m_BrowserImage->GetGreen(x, y);
                    int b=m_BrowserImage->GetBlue (x, y);

                    r*=Factor; wxASSERT(r>=0 && r<=255);
                    g*=Factor; wxASSERT(g>=0 && g<=255);
                    b*=Factor; wxASSERT(b>=0 && b<=255);

                    m_BrowserImage->SetRGB(x, y, r, g, b);
                }

            delete NormalMap;
        }
#endif
    }

    return *m_BrowserImage;
}


MatSys::RenderMaterialT* EngineMaterialT::GetRenderMaterial(bool PreviewMode) const
{
    if (PreviewMode)
    {
        // Preview Mode.
        if (MatSysRenderMaterial_Normal==NULL)
            MatSysRenderMaterial_Normal=MatSys::Renderer->RegisterMaterial(MatSysMaterial);

        return MatSysRenderMaterial_Normal;
    }
    else
    {
        // Edit Mode.
        if (MatSysRenderMaterial_Editor==NULL)
        {
            // Use a clean empty material for the edit mode.
            Material_Editor=new MaterialT;

            // Then set nothing but its diffuse map. No tricks that could tamper its visibility.
                 if (!MatSysMaterial->meta_EditorImage.IsEmpty()) Material_Editor->DiffMapComp=MatSysMaterial->meta_EditorImage;
            else if (!MatSysMaterial->DiffMapComp.IsEmpty()     ) Material_Editor->DiffMapComp=MatSysMaterial->DiffMapComp;
            else
            {
                // Create a MapCompositionT for a texture that won't be found,
                // so that we're sure to get the built-in "?File Not Found" bitmap as a result.
                // (The MatSys might not render this material at all if the DiffMapComp was left empty.)
                TextParserT TP("FileNotFound.png", "({[]}),", false);
                Material_Editor->DiffMapComp=MapCompositionT(TP, "");
            }

            MatSysRenderMaterial_Editor=MatSys::Renderer->RegisterMaterial(Material_Editor);
        }

        return MatSysRenderMaterial_Editor;
    }
}


bool EngineMaterialT::ShowInMaterialBrowser() const
{
    // Never show those that have meta_editorImage set to "noEditor".
    return !(MatSysMaterial->meta_EditorImage.GetType()==MapCompositionT::Map &&
             MatSysMaterial->meta_EditorImage.GetString()=="noEditor");
}


bool EngineMaterialT::IsTranslucent() const
{
    if (MatSysMaterial->BlendFactorSrc==MaterialT::None) return false;
    if (MatSysMaterial->BlendFactorDst==MaterialT::None) return false;
    if (MatSysMaterial->BlendFactorSrc==MaterialT::One && MatSysMaterial->BlendFactorDst==MaterialT::Zero) return false;

    return true;
}
