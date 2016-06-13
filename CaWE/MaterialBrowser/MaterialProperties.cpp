/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MaterialProperties.hpp"
#include "MaterialBrowserDialog.hpp"
#include "../EditorMaterial.hpp"

#include "MaterialSystem/Material.hpp"


using namespace MaterialBrowser;


BEGIN_EVENT_TABLE(MaterialPropertiesT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, MaterialPropertiesT::OnValueChanging)
END_EVENT_TABLE()


MaterialPropertiesT::MaterialPropertiesT(DialogT* Parent)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Material Properties");
}


void MaterialPropertiesT::ShowMaterial(EditorMaterialI* EditorMaterial)
{
    ClearPage(0);

    if (!EditorMaterial)
    {
        RefreshGrid();  // Needed? Or implied by ClearPage()?
        return;
    }

    Append(new wxStringProperty("Name", wxPG_LABEL, EditorMaterial->GetName()));

    wxPGProperty* PixelSize=Append(new wxStringProperty("Pixel size", wxPG_LABEL, "<composed>"));
    AppendIn(PixelSize, new wxFloatProperty("X", wxPG_LABEL, EditorMaterial->GetWidth()));
    AppendIn(PixelSize, new wxFloatProperty("Y", wxPG_LABEL, EditorMaterial->GetHeight()));


    MaterialT* Material=EditorMaterial->GetMaterial();

    if (Material)
    {
        Append(new wxStringProperty("Ambient shader", wxPG_LABEL, Material->AmbientShaderName));
        Append(new wxStringProperty("Light shader",   wxPG_LABEL, Material->LightShaderName));

        Append(new wxStringProperty("Diffuse map",  wxPG_LABEL, Material->DiffMapComp .GetString()));
        Append(new wxStringProperty("Normal map",   wxPG_LABEL, Material->NormMapComp .GetString()));
        Append(new wxStringProperty("Specular map", wxPG_LABEL, Material->SpecMapComp .GetString()));
        Append(new wxStringProperty("Luma map",     wxPG_LABEL, Material->LumaMapComp .GetString()));
        Append(new wxStringProperty("Light map",    wxPG_LABEL, Material->LightMapComp.GetString()));
        Append(new wxStringProperty("SHL map",      wxPG_LABEL, Material->SHLMapComp  .GetString()));
        Append(new wxStringProperty("Cubemap1",     wxPG_LABEL, Material->CubeMap1Comp.GetString()));
        Append(new wxStringProperty("Cubemap2",     wxPG_LABEL, Material->CubeMap2Comp.GetString()));

        Append(new wxBoolProperty("Don't draw", wxPG_LABEL, Material->NoDraw));
        SetPropertyAttribute("Don't draw", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        Append(new wxBoolProperty("Two sided", wxPG_LABEL, Material->TwoSided));
        SetPropertyAttribute("Two sided", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        Append(new wxFloatProperty("Depth offset", wxPG_LABEL, Material->DepthOffset));

        wxPGChoices PolygonMode;
        PolygonMode.Add("Filled");
        PolygonMode.Add("WireFrame");
        PolygonMode.Add("Points");

        Append(new wxEnumProperty("Polygon mode", wxPG_LABEL, PolygonMode, Material->PolygonMode));

        Append(new wxStringProperty("Alpha test value", wxPG_LABEL, Material->AlphaTestValue.GetString()));

        wxPGChoices BlendFactor;
        BlendFactor.Add("None");
        BlendFactor.Add("Zero");
        BlendFactor.Add("One");
        BlendFactor.Add("DstColor");
        BlendFactor.Add("SrcColor");
        BlendFactor.Add("OneMinusDstColor");
        BlendFactor.Add("OneMinusSrcColor");
        BlendFactor.Add("DstAlpha");
        BlendFactor.Add("SrcAlpha");
        BlendFactor.Add("OneMinusDstAlpha");
        BlendFactor.Add("OneMinusSrcAlpha");

        wxPGProperty* BlendFactors=Append(new wxStringProperty("Blend factors", wxPG_LABEL, "<composed>"));
        AppendIn(BlendFactors, new wxEnumProperty("Source",      wxPG_LABEL, BlendFactor, Material->BlendFactorSrc));
        AppendIn(BlendFactors, new wxEnumProperty("Destination", wxPG_LABEL, BlendFactor, Material->BlendFactorDst));

        wxPGProperty* ModColor=Append(new wxStringProperty("Modulate color", wxPG_LABEL, "<composed>"));
        AppendIn(ModColor, new wxStringProperty("Red",   wxPG_LABEL, Material->RedGen.GetString()));
        AppendIn(ModColor, new wxStringProperty("Green", wxPG_LABEL, Material->GreenGen.GetString()));
        AppendIn(ModColor, new wxStringProperty("Blue",  wxPG_LABEL, Material->BlueGen.GetString()));
        AppendIn(ModColor, new wxStringProperty("Alpha", wxPG_LABEL, Material->AlphaGen.GetString()));

        wxPGChoices Flags; // Array of choices.

        Flags.Add("Red",   0x01);
        Flags.Add("Green", 0x02);
        Flags.Add("Blue",  0x04);
        Flags.Add("Alpha", 0x08);
        Flags.Add("Depth", 0x10);

        long int FlagBits=0;

        if (Material->AmbientMask[0]) FlagBits|=0x01;
        if (Material->AmbientMask[1]) FlagBits|=0x02;
        if (Material->AmbientMask[2]) FlagBits|=0x04;
        if (Material->AmbientMask[3]) FlagBits|=0x08;
        if (Material->AmbientMask[4]) FlagBits|=0x10;

        Append(new wxFlagsProperty("Ambient mask", wxPG_LABEL, Flags, FlagBits));
        SetPropertyAttribute("Ambient mask", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        Append(new wxBoolProperty("Use mesh colors", wxPG_LABEL, Material->UseMeshColors));
        SetPropertyAttribute("Use mesh colors", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        Append(new wxBoolProperty("No dynamic light", wxPG_LABEL, Material->NoDynLight));
        SetPropertyAttribute("No dynamic light", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        Append(new wxBoolProperty("No shadows", wxPG_LABEL, Material->NoShadows));
        SetPropertyAttribute("No shadows", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        FlagBits=0;

        if (Material->LightMask[0]) FlagBits|=0x01;
        if (Material->LightMask[1]) FlagBits|=0x02;
        if (Material->LightMask[2]) FlagBits|=0x04;
        if (Material->LightMask[3]) FlagBits|=0x08;
        if (Material->LightMask[4]) FlagBits|=0x10;

        Append(new wxFlagsProperty("Light mask", wxPG_LABEL, Flags, FlagBits)); // Same flags as above (AmbientMask).
        SetPropertyAttribute("Light mask", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        wxPGChoices ClipFlags;
        ClipFlags.Add("Player",      0x0001);
        ClipFlags.Add("Monsters",    0x0002);
        ClipFlags.Add("Moveables",   0x0004);
        ClipFlags.Add("IK",          0x0008);
        ClipFlags.Add("Projectiles", 0x0010);
        ClipFlags.Add("Sight",       0x0020);
        ClipFlags.Add("BspPortals",  0x0040);
        ClipFlags.Add("Radiance",    0x0080);
        ClipFlags.Add("AllBlocking", 0x00FF);
        ClipFlags.Add("BlkButUtils", 0x00FF & ~0x0040 & ~0x0080);
        ClipFlags.Add("Trigger",     0x0100);

        Append(new wxFlagsProperty("Clip flags", wxPG_LABEL, ClipFlags, Material->ClipFlags));
        SetPropertyAttribute("Clip flags", wxPG_BOOL_USE_CHECKBOX, true, wxPG_RECURSE);

        wxPGChoices SurfaceType;
        SurfaceType.Add("None");
        SurfaceType.Add("Stone");
        SurfaceType.Add("Metal");
        SurfaceType.Add("Sand");
        SurfaceType.Add("Wood");
        SurfaceType.Add("Liquid");
        SurfaceType.Add("Glass");
        SurfaceType.Add("Plastic");

        Append(new wxEnumProperty("Surface type", wxPG_LABEL, SurfaceType, Material->SurfaceType));

        Append(new wxStringProperty("Editor image",  wxPG_LABEL, Material->meta_EditorImage.GetString()));

        wxPGProperty* RadiantExitance=Append(new wxStringProperty("Radiant exitance values", wxPG_LABEL, "<composed>"));
        AppendIn(RadiantExitance, new wxFloatProperty("R", wxPG_LABEL, Material->meta_RadiantExitance_Values[0]));
        AppendIn(RadiantExitance, new wxFloatProperty("G", wxPG_LABEL, Material->meta_RadiantExitance_Values[1]));
        AppendIn(RadiantExitance, new wxFloatProperty("B", wxPG_LABEL, Material->meta_RadiantExitance_Values[2]));

        wxPGProperty* SunLightIrr=Append(new wxStringProperty("Sunlight irradiance", wxPG_LABEL, "<composed>"));
        AppendIn(SunLightIrr, new wxFloatProperty("R", wxPG_LABEL, Material->meta_SunLight_Irr[0]));
        AppendIn(SunLightIrr, new wxFloatProperty("G", wxPG_LABEL, Material->meta_SunLight_Irr[1]));
        AppendIn(SunLightIrr, new wxFloatProperty("B", wxPG_LABEL, Material->meta_SunLight_Irr[2]));

        wxPGProperty* SunLightDir=Append(new wxStringProperty("Sunlight direction", wxPG_LABEL, "<composed>"));
        AppendIn(SunLightDir, new wxFloatProperty("X", wxPG_LABEL, Material->meta_SunLight_Dir[0]));
        AppendIn(SunLightDir, new wxFloatProperty("Y", wxPG_LABEL, Material->meta_SunLight_Dir[1]));
        AppendIn(SunLightDir, new wxFloatProperty("Z", wxPG_LABEL, Material->meta_SunLight_Dir[2]));
    }
    else
    {
        wxString          Info     ="This material is referred to in the map, but no corresponding shader definition exists in the Material System.";
        wxStringProperty* DummyProp=new wxStringProperty("Dummy Material", wxPG_LABEL, Info);

        Append(DummyProp);
        DummyProp->SetHelpString(Info);
    }

    CollapseAll();
    RefreshGrid();
}


void MaterialPropertiesT::OnValueChanging(wxPropertyGridEvent& Event)
{
    // TODO
    //Event.Veto();
}
