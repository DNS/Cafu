/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolTerrainEdit.hpp"
#include "ChildFrame.hpp"
#include "DialogTerrainEdit.hpp"
#include "ToolManager.hpp"
#include "MapDocument.hpp"
#include "MapElement.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "Renderer2D.hpp"
#include "ToolOptionsBars.hpp"

#include "../Camera.hpp"
#include "../CursorMan.hpp"
#include "../GameConfig.hpp"
#include "../ParentFrame.hpp"

#include "Commands/ModifyTerrain.hpp"
#include "Commands/ChangeTerrainRes.hpp"
#include "Commands/Select.hpp"

#include "Bitmap/Bitmap.hpp"

#include "wx/wx.h"
#include "wx/rawbmp.h"
#include "wx/filename.h"
#include "wx/file.h"

#include "noise.h"

#include <algorithm>


class ColorGradientsT
{
    public:

    ColorGradientsT()
    {
        // Fill grey array. Note: We need to insert each value 8 times in order to get 2048 gradient values.
        for (unsigned long i=0; i<256; i++)
            for (unsigned long j=0; j<8; j++)
                Colors[0][i*8+j]=(i<<16)+(i<<8)+i;

        // Fill color array. Note: We need to insert each value 2 times in order to get 2048 gradient values.
        unsigned long Red  =0;
        unsigned long Green=0;
        unsigned long Blue =251;

        // Insert first 4 values that are "below" blue.
        // This is necessary to get a color gradient with 1024 different colors.
        for (unsigned long i=0; i<4; i++)
        {
            Blue++;
            Colors[1][i*2  ]=(Red<<16)+(Green<<8)+Blue;
            Colors[1][i*2+1]=(Red<<16)+(Green<<8)+Blue;
        }

        for (unsigned long i=0; i<1020; i++)
        {
            switch (i/255)
            {
                case 0:
                    Green++;
                    wxASSERT(Red==0);
                    wxASSERT(Blue==255);
                    break;

                case 1:
                    Blue--;
                    wxASSERT(Red==0);
                    wxASSERT(Green==255);
                    break;

                case 2:
                    Red++;
                    wxASSERT(Green==255);
                    wxASSERT(Blue==0);
                    break;

                case 3:
                    Green--;
                    wxASSERT(Red==255);
                    wxASSERT(Blue==0);
                    break;
            }

            // Note: 8 is the offset of the first 4 values entered before this loop.
            Colors[1][i*2+8  ]=(Red<<16)+(Green<<8)+Blue;
            Colors[1][i*2+8+1]=(Red<<16)+(Green<<8)+Blue;
        }

        unsigned char GreyValue=0;

        // Fill debug array.
        for (unsigned long i=0; i<2048; i++)
        {
            Colors[2][i]=(GreyValue<<16)+(GreyValue<<8)+GreyValue;
            GreyValue++;
        }
    }

    unsigned long Colors[3][2048];
};

static const ColorGradientsT ColorGradients;


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolTerrainEditorT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolTerrainEditorT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolTerrainEditorT::TypeInfo(GetToolTIM(), "ToolTerrainEditorT", "ToolT", ToolTerrainEditorT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolTerrainEditorT::ToolTerrainEditorT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_TerrainEditorDialog(NULL),
      m_TerrainOrig(NULL),
      m_TerrainCopy(NULL),
      m_IsRecSelfNotify(false),
      m_RenderUpdateBitmap(true),
      m_RenderUpdateTool(true),
      m_LastRenderPosBL(-1, -1),
      m_LastRenderPosTR(-1, -1),
      m_PointTLToolOff(-1, -1),
      m_SizeXTool(0),
      m_SizeYTool(0),
      m_ToolRenderPosition(-1, -1),
      m_ToolRadiusX(0),
      m_ToolRadiusY(0),
      m_HeightDataPos(-1, -1),
      m_ToolMode(TOOLMODE_INACTIVE),
      m_EditBounds(),
      m_EditPlane(Vector3fT(0, 0, 1), 0),
      m_ReferenceHeight(0),
      m_NoiseWeightsRes(17),
      m_GaussWeightsRes(3),
      m_CurrentColorGradient(GREY),
      m_OptionsBar(new OptionsBar_EditFacePropsToolT(ParentOptionsBar))
{
    m_MapDoc.RegisterObserver(this);
}


ToolTerrainEditorT::~ToolTerrainEditorT()
{
    m_MapDoc.UnregisterObserver(this);
}


void ToolTerrainEditorT::SetToolDialog(TerrainEditorDialogT* TerrainEditorDialog)
{
    m_TerrainEditorDialog=TerrainEditorDialog;
}


void ToolTerrainEditorT::SetHeightDataPos(const wxPoint& HeightDataPos)
{
    if (m_TerrainEditorDialog==NULL) return;

    if (HeightDataPos.x<0 || HeightDataPos.y<0 || m_TerrainCopy==NULL || (unsigned long)HeightDataPos.x>m_TerrainCopy->GetResolution()-1 || (unsigned long)HeightDataPos.y>m_TerrainCopy->GetResolution()-1)
    {
        m_HeightDataPos=wxPoint(-1, -1);
    }
    else
    {
        m_HeightDataPos=HeightDataPos;
    }

    if (m_TerrainCopy!=NULL) m_TerrainCopy->SetToolBounds(m_HeightDataPos.x, m_HeightDataPos.y, GetRadius());

    UpdateToolInformation();
}


void ToolTerrainEditorT::SetResolution(unsigned long Resolution)
{
    if (m_TerrainOrig==NULL) return;

    if (Resolution==m_TerrainOrig->GetResolution()) return;

    // Change the resolution of the original terrain. The m_TerrainCopy member is updated when we receive
    // the change notification via the Observer pattern (call to our NotifySubjectChanged() method).
    m_MapDoc.CompatSubmitCommand(new CommandChangeTerrainResT(m_MapDoc, m_TerrainOrig, Resolution));

    // After a resolution change the tool position could become invalid, so we reset it here.
    m_HeightDataPos=wxPoint(-1, -1);

    UpdateToolInformation();

    m_TerrainEditorDialog->UpdateResolution(Resolution);

    UpdateModifyWeights();
    UpdateNoiseWeights();
    UpdateGaussWeights();

    m_RenderUpdateBitmap=true;

    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
}


void ToolTerrainEditorT::GenerateTerrain(int Octaves, double Frequency, double Persistence, double Lacunarity, int Seed)
{
    noise::module::Perlin PerlinModule;
    PerlinModule.SetNoiseQuality(noise::QUALITY_BEST                             );
    PerlinModule.SetOctaveCount (Octaves                                         );
    PerlinModule.SetFrequency   (Frequency  /10.0 /m_TerrainCopy->GetResolution());
    PerlinModule.SetPersistence (Persistence/100.0                               );
    PerlinModule.SetLacunarity  (Lacunarity /10.0                                );
    PerlinModule.SetSeed        (Seed                                            );

    for (unsigned long y=0; y<m_TerrainCopy->GetResolution(); y++)
    {
        const int FlipY=m_TerrainCopy->GetResolution()-1-y;

        for (unsigned long x=0; x<m_TerrainCopy->GetResolution(); x++)
        {
            float PerlinValue=PerlinModule.GetValue(x, y, 1.0);

            if (PerlinValue> 1.0f) PerlinValue= 1.0f;
            if (PerlinValue<-1.0f) PerlinValue=-1.0f;

            const unsigned short Height16=(PerlinValue+1.0f)/2.0f*65535.0f;

            m_TerrainCopy->m_HeightData[x+FlipY*m_TerrainCopy->GetResolution()]=Height16;
        }
    }

    m_RenderUpdateBitmap=true;

    m_EditBounds=wxRect(0, 0, m_TerrainCopy->GetResolution(), m_TerrainCopy->GetResolution());

    CommitChanges();
}


void ToolTerrainEditorT::ImportHeightMap(const wxString& FileName)
{
    if (m_TerrainCopy==NULL || m_TerrainOrig==NULL) return;

    // We could create and use a map command to load the heigth data from file. Instead we load the height data into the copy
    // and then alter resolution and heigth data of the original terrain. This makes sure that the command will even work if
    // the file containing the new height data is deleted.
    try
    {
        m_TerrainCopy->LoadHeightData(FileName);
    }
    catch (const BitmapT::LoadErrorT&)
    {
        wxMessageBox(
            "There was a problem importing the selected heightmap.\n\n"
            "Please make sure that your heightmap is 2^n+1 pixels wide and high,\n"
            "for example: ..., 129*129, 257*257, 513*513, 1025*1025, ...",
            "Could not load the heightmap", wxICON_ERROR);
        return;
    }

    ArrayT<CommandT*> Commands;

    if (m_TerrainOrig->GetResolution()!=m_TerrainCopy->GetResolution())
        Commands.PushBack(new CommandChangeTerrainResT(m_MapDoc, m_TerrainOrig, m_TerrainCopy->GetResolution()));

    Commands.PushBack(new CommandModifyTerrainT(m_MapDoc, m_TerrainOrig, m_TerrainCopy->GetHeightData(), wxPoint(0, 0), m_TerrainCopy->GetResolution(), m_TerrainCopy->GetResolution()));

    // Our copy has already been updated, so we don't need to update again on modification of the original terrain.
    m_IsRecSelfNotify=true;
    m_MapDoc.CompatSubmitCommand(new CommandMacroT(Commands, "Import height data into terrain"));
    m_IsRecSelfNotify=false;

    // After a resolution change the tool position could become invalid, so we reset it here.
    m_HeightDataPos=wxPoint(-1, -1);

    UpdateToolInformation();

    m_TerrainEditorDialog->UpdateResolution(m_TerrainCopy->GetResolution());

    m_RenderUpdateBitmap=true;

    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
}


void ToolTerrainEditorT::ExportHeightMap(wxString FileName, ExportFileTypeE ExportFileType)
{
    if (m_TerrainCopy==NULL) return;

    switch (ExportFileType)
    {
        case BMP:
        case PNG:
        case JPG:
        {
            // We have to "fix" the file extension if it isn't the right one according to export file type since
            // BitmapT saves files in a format according to their extension.
            if (ExportFileType==BMP && !FileName.EndsWith(".bmp")                               ) FileName.Append(".bmp");
            if (ExportFileType==PNG && !FileName.EndsWith(".png")                               ) FileName.Append(".png");
            if (ExportFileType==JPG && !FileName.EndsWith(".jpg") && !FileName.EndsWith(".jpeg")) FileName.Append(".jpg");

            BitmapT ExportBitmap(m_TerrainCopy->GetResolution(), m_TerrainCopy->GetResolution(), NULL);

            for (unsigned long y=0; y<m_TerrainCopy->GetResolution(); y++)
            {
                int FlipY=m_TerrainCopy->GetResolution()-1-y;

                for (unsigned long x=0; x<m_TerrainCopy->GetResolution(); x++)
                {
                    const uint32_t ColorValue=m_TerrainCopy->m_HeightData[x+FlipY*m_TerrainCopy->GetResolution()] >> 8;

                    ExportBitmap.Data[x+y*m_TerrainCopy->GetResolution()]=0xFF000000+(ColorValue<<16)+(ColorValue<<8)+ColorValue;
                }
            }

            if (!ExportBitmap.SaveToDisk(FileName.c_str()))
                wxMessageBox("Error exporting terrain");

            break;
        }

        case PGM_ASCII:
        case PGM_BINARY:
        {
            // First do common stuff needed for both file types.
            wxFile ExportFile(FileName, wxFile::write);
            if (!ExportFile.IsOpened())
            {
                wxMessageBox("Error exporting terrain to Graymap: Couldn't create file.");
                return;
            }

            if (ExportFileType==PGM_ASCII) ExportFile.Write("P2\n");
            else                           ExportFile.Write("P5\n");

            ExportFile.Write("# Terrain height map created with CaWE\n");
            ExportFile.Write(wxString::Format("%lu %lu\n", m_TerrainCopy->GetResolution(), m_TerrainCopy->GetResolution()));
            ExportFile.Write("255\n");

            for (unsigned long y=0; y<m_TerrainCopy->GetResolution(); y++)
            {
                int FlipY=m_TerrainCopy->GetResolution()-1-y;

                for (unsigned long x=0; x<m_TerrainCopy->GetResolution(); x++)
                {
                    unsigned char ColorValue=m_TerrainCopy->m_HeightData[x+FlipY*m_TerrainCopy->GetResolution()]>>8;

                    // Write height data according to file format.
                    if (ExportFileType==PGM_ASCII)
                        ExportFile.Write((wxString::Format("%i ", ColorValue)));
                    else
                        ExportFile.Write(&ColorValue, 1);
                }

                if (ExportFileType==PGM_ASCII) ExportFile.Write("\n");
            }

            break;
        }

        case TER:
        {
            wxFile ExportFile(FileName, wxFile::write);
            if (!ExportFile.IsOpened())
            {
                wxMessageBox("Error exporting terrain to Terragen: Couldn't create file.");
                return;
            }

            ExportFile.Write("TERRAGENTERRAIN ");
            ExportFile.Write("SIZE");

            unsigned short Size=m_TerrainCopy->GetResolution()-1;
            ExportFile.Write(&Size, 2);

            unsigned short Padding=0;
            ExportFile.Write(&Padding, 2);

            ExportFile.Write("SCAL");

            float TerrainSizeX_Inch =m_TerrainCopy->m_TerrainBounds.Max.x-m_TerrainCopy->m_TerrainBounds.Min.x;
            float TerrainSizeX_Meter=TerrainSizeX_Inch*0.0254f;

            float Scale=TerrainSizeX_Meter/Size;
            ExportFile.Write(&Scale, 4);
            ExportFile.Write(&Scale, 4);
            ExportFile.Write(&Scale, 4);

            ExportFile.Write("ALTW");

            float TerrainSizeZ_Inch =m_TerrainCopy->m_TerrainBounds.Max.z-m_TerrainCopy->m_TerrainBounds.Min.z;
            float TerrainSizeZ_Meter=TerrainSizeZ_Inch*0.0254f;

            float MaxHeightTerrainUnits=TerrainSizeZ_Meter/Scale;

            short int HeightScale=ceil(2.0f*MaxHeightTerrainUnits);
            ExportFile.Write(&HeightScale, 2);

            short int BaseHeight=0;
            ExportFile.Write(&BaseHeight, 2);

            float ElevationFactor=65536.0f/float(HeightScale);

            for (unsigned long y=0; y<m_TerrainCopy->GetResolution(); y++)
            {
                for (unsigned long x=0; x<m_TerrainCopy->GetResolution(); x++)
                {
                    short int Elevation=(m_TerrainCopy->m_HeightData[x+y*m_TerrainCopy->GetResolution()]/65535.0f)*MaxHeightTerrainUnits*ElevationFactor;
                    ExportFile.Write(&Elevation, 2);
                }
            }

            ExportFile.Write("EOF ");

            break;
        }
    }
}


static inline float ModifyFunc(float Radius, float Distance, float Hardness)
{
    float fx=(1.0f-cos((Distance-Radius)*3.141592f/Radius))/2;
    float hx=pow(fx, 1.0f/Hardness);

    return hx;
}


void ToolTerrainEditorT::UpdateModifyWeights()
{
    m_ModifyWeights.Clear();

    int ToolRadius  =GetRadius();
    int ToolHardness=m_TerrainEditorDialog->GetHardness();

    wxPoint Center(ToolRadius, ToolRadius);

    // Note: When calculating the distance of a point in the area of effect to the center the calculations are always made with square values
    // and the square root function is never performed. This is done due to performance reasons.
    // Another solution would be to precalculate square roots for all possible values and store them in a lookup table to calculate the "real"
    // distance of a point to the tool center.
    int ToolRadiusSquare=ToolRadius*ToolRadius;

    // Calculate hardness used in calculations.
    float Hardness=0.02f*(float)ToolHardness;

    // Values higher than 50 are calculated by another formula
    if (ToolHardness>50)
        Hardness=0.18f*((float)ToolHardness-44.444444f);

    int ModifyRes=ToolRadius*2+1;

    for (int y=0; y<ModifyRes; y++)
    {
        int DistY=Center.y-y;
        int DistYSquare=DistY*DistY;

        for (int x=0; x<ModifyRes; x++)
        {
            // Calculate distance to tool radius center.
            int DistX=Center.x-x;

            int DistanceSquare=DistX*DistX+DistYSquare;

            // If distance is greater than the tools radius we don't want to change this part at all.
            if (DistanceSquare>ToolRadiusSquare) { m_ModifyWeights.PushBack(0.0f); continue; }

            // Otherwise calculate the weight of this point using the modify function.
            m_ModifyWeights.PushBack(ModifyFunc((float)ToolRadiusSquare, (float)DistanceSquare, Hardness));
        }
    }
}


void ToolTerrainEditorT::UpdateNoiseWeights()
{
    m_NoiseWeights.Clear();

    int ToolRadius=GetRadius();

    m_NoiseWeightsRes=ToolRadius/2;

    for (int y=0; y<m_NoiseWeightsRes; y++)
    {
        int DistY=y-m_NoiseWeightsRes/2;
        int DistYSquare=DistY*DistY;

        for (int x=0; x<m_NoiseWeightsRes; x++)
        {
            // Calculate distance to tool radius center.
            int DistX=x-m_NoiseWeightsRes/2;

            float Distance=sqrt(float(DistX*DistX+DistYSquare));

            // If distance is greater than the noise weights radius we don't want to change this part at all.
            if (Distance>m_NoiseWeightsRes/2) { m_NoiseWeights.PushBack(0.0f); continue; }

            // Otherwise calculate the weight of this point using the modify function.
            m_NoiseWeights.PushBack(ModifyFunc(m_NoiseWeightsRes/2, Distance, 1.0f));
        }
    }
}


static float GaussFunc(float Distance, float Intensity)
{
    return 1.0f/(sqrt(2*3.141592f)*Intensity)*exp(-(pow(Distance, 2))/(2*pow(Intensity, 2)));
}


void ToolTerrainEditorT::UpdateGaussWeights()
{
    m_GaussWeights.Clear();

    int ToolEffect=m_TerrainEditorDialog->GetToolEffect();

    float Intensity=ToolEffect/50.0f;

    for (int y=0; y<m_GaussWeightsRes; y++)
    {
        int DistY=y-m_GaussWeightsRes/2;
        int DistYSquare=DistY*DistY;

        for (int x=0; x<m_GaussWeightsRes; x++)
        {
            int DistX=x-m_GaussWeightsRes/2;

            float Distance=sqrt(float(DistX*DistX+DistYSquare));

            // Otherwise calculate the weight of this point using the gauss function.
            m_GaussWeights.PushBack(GaussFunc(Distance, Intensity));
        }
    }
}


wxWindow* ToolTerrainEditorT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


void ToolTerrainEditorT::OnActivate(ToolT* OldTool)
{
    if (IsActiveTool()) return;

    wxASSERT(m_TerrainOrig==NULL);
    wxASSERT(m_TerrainCopy==NULL);

    if (m_MapDoc.GetSelection().Size()>0)
    {
        const ArrayT<MapElementT*>& Selection=m_MapDoc.GetSelection();

        for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
        {
            MapTerrainT* SelTerrain=dynamic_cast<MapTerrainT*>(Selection[SelNr]);

            if (SelTerrain!=NULL)
            {
                SetTerrain(SelTerrain);
                break;
            }
        }

        m_MapDoc.CompatSubmitCommand(CommandSelectT::Clear(&m_MapDoc));
    }

    m_MapDoc.GetChildFrame()->ShowPane(m_MapDoc.GetChildFrame()->GetTerrainEditorDialog(), true);
}


void ToolTerrainEditorT::OnDeactivate(ToolT* NewTool)
{
    if (NewTool!=this)
    {
        MapTerrainT* EditedTerrain=m_TerrainOrig;

        SetTerrain(NULL);   // Resets m_TerrainOrig to NULL.

        if (EditedTerrain!=NULL)
            m_MapDoc.CompatSubmitCommand(CommandSelectT::Add(&m_MapDoc, EditedTerrain));

        m_MapDoc.GetChildFrame()->ShowPane(m_MapDoc.GetChildFrame()->GetTerrainEditorDialog(), false);
    }
}


bool ToolTerrainEditorT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            // Switch to eyedropper mode if not yet happened.
            if (m_ToolMode!=TOOLMODE_EYEDROPPER)
            {
                m_ToolMode=TOOLMODE_EYEDROPPER;
                ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            }

            return true;

        case WXK_RETURN:
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
            {
                SetEditMode(TOOLMODE_INACTIVE);
                return true;
            }

        case WXK_ESCAPE:
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
            {
                m_EditRoadParts.Clear();
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
                return true;
            }

        case WXK_BACK:
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
            {
                m_EditRoadParts.DeleteBack();
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
                return true;
            }

        default:
            return false;
    }
}


bool ToolTerrainEditorT::OnKeyUp2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            m_ToolMode=TOOLMODE_INACTIVE;
            ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            return true;

        default:
            return false;
    }
}


bool ToolTerrainEditorT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    // Only try to select another terrain if the current terrains height data position is invalid.
    if (m_HeightDataPos.x<0 || m_HeightDataPos.y<0)
    {
        ArrayT<MapElementT*> HitElems=ViewWindow.GetElementsAt(ME.GetPosition());

        for (unsigned long HitNr=0; HitNr<HitElems.Size(); HitNr++)
        {
            MapTerrainT* HitTerrain=dynamic_cast<MapTerrainT*>(HitElems[HitNr]);

            if (HitTerrain!=NULL && HitTerrain!=m_TerrainOrig)
            {
                wxASSERT(HitTerrain!=m_TerrainCopy);    // Should never happen - m_TerrainCopy is not in the world.

                SetTerrain(HitTerrain);
                break;
            }
        }

        return true;    // Selecting another terrain should not immediately entail (the beginning of) an edit operation.
    }

    if (m_TerrainCopy==NULL) return true;

    if (ME.AltDown())
    {
        SetEditMode(TOOLMODE_EYEDROPPER);

        ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));

        PickReHeightValue();
    }
    else
    {
        SetEditMode(TOOLMODE_ACTIVE);

        ViewWindow.CaptureMouse();
        ViewWindow.SetCursor(wxCURSOR_BLANK);
    }

    return true;
}


bool ToolTerrainEditorT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (m_TerrainCopy==NULL) return true;

    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
    {
        m_ToolMode=TOOLMODE_INACTIVE; // Silently set toolmode to inactive (doesn't commit any changes).

        ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
    }
    else if (ME.AltDown())
    {
        SetEditMode(TOOLMODE_EYEDROPPER);

        ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));
    }
    else
    {
        SetEditMode(TOOLMODE_INACTIVE);

        ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
    }

    if (ViewWindow.HasCapture()) ViewWindow.ReleaseMouse();
    return true;
}


bool ToolTerrainEditorT::OnMMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
    {
        if (m_EditRoadParts.Size()>0) m_EditRoadParts.DeleteBack();
        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    }
    else if (!m_TerrainEditorDialog->OnMMouseUp())
        PickReHeightValue();

    return true;
}


// Helper function for code below.
static int Round(float a)
{
    return int(a>=0.0 ? a+0.5f : a-0.5f);
}


bool ToolTerrainEditorT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (ME.AltDown()) m_ToolMode=TOOLMODE_EYEDROPPER;

    if (m_ToolMode==TOOLMODE_INACTIVE)
        ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
    if (m_ToolMode==TOOLMODE_EYEDROPPER)
        ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));
    if (m_ToolMode==TOOLMODE_ACTIVE)
        ViewWindow.SetCursor(wxCURSOR_BLANK);

    if (m_TerrainCopy==NULL) return true;

    // Only calculate tool position in top down 2D view.
    if (ViewWindow.GetViewType()!=ViewWindowT::VT_2D_XY) return true;

    // Translate mouse screen position into a tool space position.
    wxPoint MousePosition=ViewWindow.WindowToTool(ME.GetPosition());

    // Read terrain edges in toolspace and calculate terrain size in toolspace.
    wxPoint      PointBLTool=ViewWindow.WorldToTool(m_TerrainCopy->m_TerrainBounds.Min);
    wxPoint      PointTRTool=ViewWindow.WorldToTool(m_TerrainCopy->m_TerrainBounds.Max);
    unsigned int SizeXTool  =PointTRTool.x-PointBLTool.x;
    unsigned int SizeYTool  =PointBLTool.y-PointTRTool.y;

    wxPoint HeightDataPos(-1, -1);

    // Calculate offsets around terrain bounds that still qualify as inside terrain.
    // They are half the size of a rendered height map field.
    float OffsetX=float(SizeXTool)/float(m_TerrainCopy->GetResolution())*0.5f;
    float OffsetY=float(SizeYTool)/float(m_TerrainCopy->GetResolution())*0.5f;

    // Check if mouse lies inside terrain boundaries.
    if (MousePosition.x>PointBLTool.x-OffsetX && MousePosition.x<PointTRTool.x+OffsetX && MousePosition.y<PointBLTool.y+OffsetY && MousePosition.y>PointTRTool.y-OffsetY)
    {
        // Get relative position to terrains top left point.
        wxPoint PosRelative(MousePosition.x-PointBLTool.x, MousePosition.y-PointTRTool.y);

        HeightDataPos.x=Round(float(PosRelative.x)/float(SizeXTool-1)*float(m_TerrainCopy->GetResolution()-1));
        HeightDataPos.y=Round(float(PosRelative.y)/float(SizeYTool-1)*float(m_TerrainCopy->GetResolution()-1));
    }

    SetHeightDataPos(HeightDataPos);

    // Show mouse position in world coordinates in status bar.
    if (m_HeightDataPos.y>-1 && m_HeightDataPos.x>-1)
    {
        const Vector3fT WorldPos   =ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f);
        const int       Height     =m_TerrainCopy->m_HeightData[(m_TerrainCopy->m_Resolution-1-m_HeightDataPos.y)*m_TerrainCopy->m_Resolution+m_HeightDataPos.x];
        const float     HeightWorld=Height/65535.0f*(m_TerrainCopy->m_TerrainBounds.Max.z-m_TerrainCopy->m_TerrainBounds.Min.z)+m_TerrainCopy->m_TerrainBounds.Min.z;

        // Overwrite the entire string (instead of appending the z-coordinate only),
        // because WorldPos is guaranteed to not be snapped to the grid.
        ViewWindow.GetChildFrame()->SetStatusText(
            wxString::Format("x: %.0f, y: %.0f, z: %.0f", WorldPos[ViewWindow.GetAxesInfo().HorzAxis], WorldPos[ViewWindow.GetAxesInfo().VertAxis], HeightWorld), ChildFrameT::SBP_MOUSE_POS);
    }

    DoEdit();
    m_RenderUpdateTool=true;
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    return true;
}


int ToolTerrainEditorT::OnContextMenu2D(ViewWindow2DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu)
{
    enum
    {
        ID_MENU_1TO1_ZOOM=wxID_HIGHEST+1000,
        ID_MENU_COLOR_GREY,
        ID_MENU_COLOR_RAINBOW,
        ID_MENU_COLOR_DEBUG
    };

    wxMenu* SubMenuViewColor=new wxMenu();
    SubMenuViewColor->Append(ID_MENU_COLOR_GREY, "Greyscale");
    SubMenuViewColor->Append(ID_MENU_COLOR_RAINBOW, "Rainbow");
    SubMenuViewColor->Append(ID_MENU_COLOR_DEBUG, "Debug");

    if (Menu.GetMenuItemCount()>0) Menu.AppendSeparator();
    Menu.Append(ID_MENU_1TO1_ZOOM, "Zoom terrain 1:1")->Enable(m_TerrainCopy!=NULL);
    Menu.AppendSubMenu(SubMenuViewColor, "Terrain color scheme");

    const int MenuSelID=ViewWindow.GetPopupMenuSelectionFromUser(Menu);

    switch (MenuSelID)
    {
        case ID_MENU_1TO1_ZOOM:
        {
            if (m_TerrainCopy)
            {
                int TerrainSizeX=m_TerrainCopy->m_TerrainBounds.Max.x-m_TerrainCopy->m_TerrainBounds.Min.x;
                ViewWindow.SetZoom(float(m_TerrainCopy->GetResolution())/float(TerrainSizeX));
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            }
            break;
        }

        case ID_MENU_COLOR_GREY:
        {
            m_CurrentColorGradient=GREY;
            m_RenderUpdateBitmap=true;
            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            break;
        }

        case ID_MENU_COLOR_RAINBOW:
        {
            m_CurrentColorGradient=RAINBOW;
            m_RenderUpdateBitmap=true;
            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            break;
        }

        case ID_MENU_COLOR_DEBUG:
        {
            m_CurrentColorGradient=DEBUG_COLOR;
            m_RenderUpdateBitmap=true;
            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            break;
        }
    }

    return MenuSelID;
}


// This method updates the tools current position by making a hit test on the terrain and then setting the resulting position.
// There are two methods used to make this hit test:
// 1. The hit test is performed by sending a ray through the terrain and then calculating the hit position. This is used if no tool
//    is currently active, to get an exact hit position of the intersection of mouse cursor and terrain.
// 2. The hit test is performed by calculating the hit position on a predefined plane. This plane is defined as soon as the user makes
//    the first click to activate a tool and is set on the current tool position height. This is useful because the tool position won't
//    do unexpected "jumps" if the user move the cursor while working with a tool. Instead the tool position will only move on an even plane.
bool ToolTerrainEditorT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    // Nothing to do if no terrain is attached to the tool.
    if (m_TerrainCopy==NULL) return true;

    if (ME.AltDown()) m_ToolMode=TOOLMODE_EYEDROPPER;

         if (m_ToolMode==TOOLMODE_INACTIVE)
            ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
    else if (m_ToolMode==TOOLMODE_EYEDROPPER)
            ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));
    else
            ViewWindow.SetCursor(wxCURSOR_BLANK);

    wxPoint HeightDataPos(-1, -1);

    if (m_ToolMode==TOOLMODE_INACTIVE || m_ToolMode==TOOLMODE_EYEDROPPER)
    {
        // The tool is currently inactive (no mouse button is down), but the mouse is moved in a 3D view.
        // Compute the tool position from a real ray intersection test with the terrain.

        // Note that our ray does intentionally not start at ViewWindow.GetCamera().Pos,
        // but at the point of intersection with the near clipping plane!
        const Vector3fT RayOrigin=ViewWindow.WindowToWorld(ME.GetPosition());
        const Vector3fT RayDir   =RayOrigin - ViewWindow.GetCamera().Pos;
        const float     RayScale =16384.0f/std::max(length(RayDir), 1.0f);

        // The TraceRay() method needs a maximum range ray, not just a unit direction vector.
        HeightDataPos=m_TerrainCopy->TraceRay(RayOrigin, RayDir*RayScale);
    }
    else
    {
        // The tool is currently active (one of the mouse buttons is down).
        // Therefore, derive the new tool position from the m_EditPlane.
        const MapTerrainT*   Terrain      =m_TerrainCopy;
        const BoundingBox3fT TerrainBounds=Terrain->m_TerrainBounds;

        const float TerrainLengthX=TerrainBounds.Max.x-TerrainBounds.Min.x;
        const float TerrainLengthY=TerrainBounds.Max.y-TerrainBounds.Min.y;

        try
        {
            const Vector3fT HitPos=m_EditPlane.GetIntersection(ViewWindow.GetCamera().Pos, ViewWindow.WindowToWorld(ME.GetPosition()), 0);

            HeightDataPos.x=Round(float(HitPos.x-TerrainBounds.Min.x)/float(TerrainLengthX)*float(Terrain->GetResolution()-1));
            HeightDataPos.y=Round(float(HitPos.y-TerrainBounds.Min.y)/float(TerrainLengthY)*float(Terrain->GetResolution()-1));

            // Flip Y position.
            HeightDataPos.y=Terrain->GetResolution()-1-HeightDataPos.y;
        }
        catch (const DivisionByZeroE&)
        {
            // A DivisionByZeroE is thrown when the ray from the camera position to the clicked pixel is parallel to
            // the dragging plane. Due to our setup, this should never happen, but if it does, we just ignore the case.
        }
    }

    SetHeightDataPos(HeightDataPos);
    DoEdit();
    m_RenderUpdateTool=true;
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);

    return true;
}


bool ToolTerrainEditorT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            // Switch to eyedropper mode if not yet happened.
            if (m_ToolMode!=TOOLMODE_EYEDROPPER)
            {
                m_ToolMode=TOOLMODE_EYEDROPPER;
                ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            }

            return true;

        case WXK_RETURN:
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
            {
                SetEditMode(TOOLMODE_INACTIVE);
                return true;
            }

        case WXK_ESCAPE:
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
            {
                m_EditRoadParts.Clear();
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
                return true;
            }

        case WXK_BACK:
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
            {
                m_EditRoadParts.DeleteBack();
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
                return true;
            }

        default:
            return false;
    }
}


bool ToolTerrainEditorT::OnKeyUp3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            m_ToolMode=TOOLMODE_INACTIVE;
            ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            return true;

        default:
            return false;
    }
}


bool ToolTerrainEditorT::OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
 // if (No restriction here as in or similar to OnLMouseDown2D())
    {
        ArrayT<ViewWindow3DT::HitInfoT> HitElems=ViewWindow.GetElementsAt(ME.GetPosition());

        for (unsigned long HitNr=0; HitNr<HitElems.Size(); HitNr++)
        {
            MapTerrainT* HitTerrain=dynamic_cast<MapTerrainT*>(HitElems[HitNr].Object);

            if (HitTerrain!=NULL && HitTerrain!=m_TerrainOrig)
            {
                wxASSERT(HitTerrain!=m_TerrainCopy);    // Should never happen - m_TerrainCopy is not in the world.

                SetTerrain(HitTerrain);
                return true;    // Selecting another terrain should not immediately entail (the beginning of) an edit operation.
            }
        }
    }

    if (m_TerrainCopy==NULL) return true;

    if (ME.AltDown())
    {
        SetEditMode(TOOLMODE_EYEDROPPER);

        ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));

        PickReHeightValue();
    }
    else
    {
        m_EditPlane=Plane3fT(Vector3fT(0, 0, 1), GetToolHeight_World());

        SetEditMode(TOOLMODE_ACTIVE);

        ViewWindow.SetCursor(wxCURSOR_BLANK);
        ViewWindow.CaptureMouse();
    }

    return true;
}


bool ToolTerrainEditorT::OnLMouseUp3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    if (m_TerrainCopy==NULL) return true;

    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
    {
        m_ToolMode=TOOLMODE_INACTIVE; // Silently set toolmode to inactive (doesn't commit any changes).

        ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
    }
    else if (ME.AltDown())
    {
        SetEditMode(TOOLMODE_EYEDROPPER);

        ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));
    }
    else
    {
        SetEditMode(TOOLMODE_INACTIVE);

        ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
    }

    if (ViewWindow.HasCapture()) ViewWindow.ReleaseMouse();
    return true;
}


bool ToolTerrainEditorT::OnMMouseUp3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
    {
        if (m_EditRoadParts.Size()>0) m_EditRoadParts.DeleteBack();
        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    }
    else if (!m_TerrainEditorDialog->OnMMouseUp())
        PickReHeightValue();

    return true;
}


bool ToolTerrainEditorT::IsHiddenByTool(const MapElementT* Elem) const
{
    // If Elem is our m_TerrainOrig, we want it hidden from normal rendering.
    return Elem==m_TerrainOrig;
}


void ToolTerrainEditorT::RenderTool2D(Renderer2DT& Renderer) const
{
    if (m_TerrainEditorDialog==NULL) return;
    if (m_TerrainCopy        ==NULL) return;

    const ViewWindow2DT& ViewWin=Renderer.GetViewWin2D();

    // Render terrain only in top down view.
    if (ViewWin.GetViewType()!=ViewWindowT::VT_2D_XY) return;

    unsigned long TerrainRes=m_TerrainCopy->GetResolution();

    // Read terrain edges in toolspace and screenspace.
    const wxPoint PointBLTool=ViewWin.WorldToTool(m_TerrainCopy->m_TerrainBounds.Min);
    const wxPoint PointTRTool=ViewWin.WorldToTool(m_TerrainCopy->m_TerrainBounds.Max);
    const wxPoint PointTLTool=wxPoint(PointBLTool.x, PointTRTool.y);
    const wxPoint PointBRTool=wxPoint(PointTRTool.x, PointBLTool.y);

    const wxPoint PointBLScreen=ViewWin.ToolToWindow(PointBLTool);
    const wxPoint PointTRScreen=ViewWin.ToolToWindow(PointTRTool);
    const wxPoint PointTLScreen=ViewWin.ToolToWindow(PointTLTool);
    const wxPoint PointBRScreen=ViewWin.ToolToWindow(PointBRTool);

    // Only recalculate render bitmap if terrain position inside view window has changed.
    if (m_RenderUpdateBitmap || m_LastRenderPosBL!=PointBLScreen || m_LastRenderPosTR!=PointTRScreen)
    {
        const int ClientSizeX=ViewWin.GetClientSize().GetWidth();
        const int ClientSizeY=ViewWin.GetClientSize().GetHeight();

        // Check if terrain rect intersects with screen, if not return and draw nothing.
        const wxRect ClientAreaRect=wxRect(0, 0, ClientSizeX, ClientSizeY);
        if (!ClientAreaRect.Intersects(wxRect(PointTLScreen, PointBRScreen))) return;

        // Determine screenspace offsets (these offsets are also valid for tool space).
        const unsigned int OffsetLeft  =PointBLScreen.x<0 ? -PointBLScreen.x : 0;
        const unsigned int OffsetRight =PointTRScreen.x>ClientSizeX ? PointTRScreen.x-ClientSizeX : 0;
        const unsigned int OffsetTop   =PointTRScreen.y<0 ? -PointTRScreen.y : 0;
        const unsigned int OffsetBottom=PointBLScreen.y>ClientSizeY ? PointBLScreen.y-ClientSizeY : 0;

        // Set new terrain size in tool space.
        m_SizeXTool=PointBRTool.x-PointBLTool.x;
        m_SizeYTool=PointBLTool.y-PointTLTool.y;

        // Translate offsets to height data offsets (These are needed to extract the height data to render from the terrains height data array).
        const float OffsetLeftHD  =float(OffsetLeft)  /float(m_SizeXTool)*(TerrainRes-1);
        const float OffsetRightHD =float(OffsetRight) /float(m_SizeXTool)*(TerrainRes-1);
        const float OffsetTopHD   =float(OffsetTop)   /float(m_SizeYTool)*(TerrainRes-1);
        const float OffsetBottomHD=float(OffsetBottom)/float(m_SizeYTool)*(TerrainRes-1);

        // Calculated offsetted tool positions.
        const wxPoint PointTRToolOff(PointTRTool.x-OffsetRight, PointTRTool.y+OffsetTop);
        const wxPoint PointBRToolOff(PointBRTool.x-OffsetRight, PointBRTool.y-OffsetBottom);

        m_PointTLToolOff=wxPoint(PointTLTool.x+OffsetLeft,  PointTLTool.y+OffsetTop);

        // Set last render position to current position.
        m_LastRenderPosBL=PointBLScreen;
        m_LastRenderPosTR=PointTRScreen;

        m_RenderBitmap=GetScaledBitmap(OffsetLeftHD, OffsetTopHD, TerrainRes-OffsetLeftHD-OffsetRightHD-1.0f, TerrainRes-OffsetTopHD-OffsetBottomHD-1.0f, PointTRToolOff.x-m_PointTLToolOff.x+1, PointBRToolOff.y-PointTRToolOff.y+1);

        m_RenderUpdateBitmap=false;
        m_RenderUpdateTool=true;            // A change in the rendered terrain must always result in a recalculation of the tool position.
    }

    Renderer.DrawBitmap(m_PointTLToolOff.x, m_PointTLToolOff.y, m_RenderBitmap);

    // Render tool position and radius if valid.
    if (m_HeightDataPos.x<0 || m_HeightDataPos.y<0) return;

    // If tool position and/or radius have changed.
    if (m_RenderUpdateTool)
    {
        // Calculate tool position and radius in 2D view space.
        m_ToolRadiusX=float(GetRadius())/float(TerrainRes)*float(m_SizeXTool);
        m_ToolRadiusY=float(GetRadius())/float(TerrainRes)*float(m_SizeYTool);

        m_ToolRenderPosition.x=float(m_HeightDataPos.x)/float(TerrainRes-1)*float(m_SizeXTool);
        m_ToolRenderPosition.y=float(m_HeightDataPos.y)/float(TerrainRes-1)*float(m_SizeYTool);

        // Place tool position in relation to the terrains top left point.
        // Add offset of half the size of a height data field in toolspace.
        m_ToolRenderPosition.x=PointTLTool.x+m_ToolRenderPosition.x;
        m_ToolRenderPosition.y=PointTLTool.y+m_ToolRenderPosition.y;

        m_RenderUpdateTool=false;
    }

    // Render road reference positions if road tool is active and a line between the last point and the current mouse position.
    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD && m_EditRoadParts.Size()>0)
    {
        int Radius=GetRadius();

        int XPos=                                 m_EditRoadParts[0].x+Radius;
        int YPos=m_TerrainCopy->GetResolution()-1-m_EditRoadParts[0].y-Radius;

        wxPoint BeginRenderPos(XPos/float(TerrainRes-1)*float(m_SizeXTool),
                               YPos/float(TerrainRes-1)*float(m_SizeYTool));

        BeginRenderPos.x=PointTLTool.x+BeginRenderPos.x;
        BeginRenderPos.y=PointTLTool.y+BeginRenderPos.y;

        Renderer.SetLineType(wxPENSTYLE_SOLID, 1, *wxGREEN); // Set line type for all of the following renderings.

        for (unsigned long i=1; i<m_EditRoadParts.Size(); i++)
        {
            XPos=                                 m_EditRoadParts[i].x+Radius;
            YPos=m_TerrainCopy->GetResolution()-1-m_EditRoadParts[i].y-Radius;

            wxPoint EndRenderPos(XPos/float(TerrainRes-1)*float(m_SizeXTool),
                                 YPos/float(TerrainRes-1)*float(m_SizeYTool));

            EndRenderPos.x=PointTLTool.x+EndRenderPos.x;
            EndRenderPos.y=PointTLTool.y+EndRenderPos.y;

            Renderer.DrawPoint(BeginRenderPos, 2);
            Renderer.DrawLine(BeginRenderPos, EndRenderPos);

            BeginRenderPos=EndRenderPos;
        }

        Renderer.DrawPoint(BeginRenderPos, 2);

        // Draw line between last point and mouse position.
        Renderer.DrawLine(BeginRenderPos, m_ToolRenderPosition);
    }

    if (m_ToolMode==TOOLMODE_EYEDROPPER)
    {
        Renderer.SetLineType(wxPENSTYLE_SOLID, ViewWin.GetZoom()*5.0f+1, *wxBLUE);
        Renderer.DrawLine(wxPoint(m_ToolRenderPosition.x-20*ViewWin.GetZoom(), m_ToolRenderPosition.y), wxPoint(m_ToolRenderPosition.x+20*ViewWin.GetZoom(), m_ToolRenderPosition.y));
        Renderer.DrawLine(wxPoint(m_ToolRenderPosition.x, m_ToolRenderPosition.y-20*ViewWin.GetZoom()), wxPoint(m_ToolRenderPosition.x, m_ToolRenderPosition.y+20*ViewWin.GetZoom()));
    }
    else
    {
        Renderer.SetLineType(wxPENSTYLE_SOLID, ViewWin.GetZoom()*5.0f+1, *wxRED);
        Renderer.DrawEllipse(m_ToolRenderPosition, m_ToolRadiusX, m_ToolRadiusY, false);
        Renderer.DrawPoint(m_ToolRenderPosition, 1);
    }
}


void ToolTerrainEditorT::RenderTool3D(Renderer3DT& Renderer) const
{
    // Render the working copy of the terrain.
    if (m_TerrainCopy!=NULL)
    {
        m_TerrainCopy->m_RenderEyeDropper=(m_ToolMode==TOOLMODE_EYEDROPPER);
        m_TerrainCopy->Render3D(Renderer);
    }
}


void ToolTerrainEditorT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    if (!IsActiveTool() || m_IsRecSelfNotify) return;

    for (unsigned long PrimNr = 0; PrimNr < Primitives.Size(); PrimNr++)
    {
        if (Primitives[PrimNr] == m_TerrainOrig)
        {
            // The "false" is for *not* having all the observers informed that the visibility of m_TerrainOrig changed.
            SetTerrain(NULL, false);
            break;
        }
    }
}


void ToolTerrainEditorT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (!IsActiveTool() || m_IsRecSelfNotify) return;
    if (Detail!=MEMD_PRIMITIVE_PROPS_CHANGED && Detail!=MEMD_GENERIC) return;
    if (m_TerrainOrig==NULL) return;

    for (unsigned long ElemNr=0; ElemNr<MapElements.Size(); ElemNr++)
    {
        if (MapElements[ElemNr]==m_TerrainOrig)
        {
            // Update (re-set) our local copy of the terrain, discarding all changes made.
            // The "false" is for *not* having all the observers informed that the visibility of m_TerrainOrig changed.
            SetTerrain(m_TerrainOrig, false);
            break;
        }
    }
}


void ToolTerrainEditorT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds)
{
    // Notifications about terrain modifications are always sent by the method above (since the bounding box never changes),
    // so there is nothing to do here.
}


void ToolTerrainEditorT::NotifySubjectDies(SubjectT* dyingSubject)
{
    // We should never get here, since the map document always dies after the tool.
    wxASSERT(false);
}


void ToolTerrainEditorT::SetEditMode(ToolModeE ToolMode)
{
    if (m_TerrainEditorDialog==NULL) return;
    if (m_TerrainCopy==NULL)         return;

    m_ToolMode=ToolMode;

    if (m_ToolMode==TOOLMODE_INACTIVE)
    {
        CommitChanges();
        return;
    }

    if (m_ToolMode==TOOLMODE_EYEDROPPER)
    {
        // Activation of eye dropper mode just updates the views.
        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
        return;
    }

    DoEdit(true);
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
};


void ToolTerrainEditorT::DoEdit(bool Force)
{
    // Only edit the terrain if the tools position has changed since the last edit.
    if (!Force && m_HeightDataPos==m_EditHeigthMapPos) return;

    // Only edit the terrain if an edit mode is set, a terrain is available and the position is valid.
    if (m_ToolMode==TOOLMODE_INACTIVE || m_ToolMode==TOOLMODE_EYEDROPPER || m_TerrainCopy==NULL || m_HeightDataPos.x<0 || m_HeightDataPos.y<0) return;

    m_EditHeigthMapPos=m_HeightDataPos;

    // Set tool effect in relation to terrain height, so tool effect is measured in map units.
    const float          TerrainHeight=m_TerrainCopy->m_TerrainBounds.Max.z-m_TerrainCopy->m_TerrainBounds.Min.z;
    const unsigned short ToolEffect   =(unsigned short)(m_TerrainEditorDialog->GetToolEffect()*65535.0f/TerrainHeight);

    const unsigned short MaxValue=(unsigned short)(-1); // Just a helper.

    wxPoint ToolCenter;

    ToolCenter.x=m_HeightDataPos.x;
    ToolCenter.y=m_TerrainCopy->GetResolution()-1-m_HeightDataPos.y; // Since the terrain itself and the tools position are rendered flipped horizontally we have to flip the y position here to edit the right point in the terrains height data.

    // Calculate quadratic area of effect using tool position and radius.
    // Pos is the top left corner of the area and width are the side lengths.
    int XPos=ToolCenter.x-GetRadius();
    int YPos=ToolCenter.y-GetRadius();
    int XMax=ToolCenter.x+GetRadius()+1;
    int YMax=ToolCenter.y+GetRadius()+1;

    // If the top left point is outside terrain boundaries there no part of the area lies inside the height data array.
    if (XPos>(int)m_TerrainCopy->GetResolution() || YPos>(int)m_TerrainCopy->GetResolution())
    {
        wxASSERT(false);
        return;
    }

    // Adjust top left point and widths so area lies inside height data boundaries.
    if (XMax>(int)m_TerrainCopy->GetResolution()) XMax-=XMax-m_TerrainCopy->GetResolution();
    if (YMax>(int)m_TerrainCopy->GetResolution()) YMax-=YMax-m_TerrainCopy->GetResolution();

    // Calculate area affected by this editing. We need this wxRect later, thats the reason it is created here in a variable instead of only passing it to Union() below.
    wxRect AffectedArea=wxRect(wxPoint(XPos<0 ? 0 : XPos, YPos<0 ? 0 : YPos), wxPoint(XMax-1, YMax-1));

    // Increase editing bounding box by the tools area of effect.
    m_EditBounds.Union(AffectedArea);

    // If the road tool is active, we just have to pick the current position and store it in the tools position array.
    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD)
    {
        m_EditRoadParts.PushBack(wxRect(wxPoint(XPos, YPos), wxPoint(XMax, YMax)));
        return;
    }

    // Calculate resolution of modify array here, so we don't need to do this in every loop iteration.
    int ModifyWeightRes=GetRadius()*2+1;

    ArrayT<float> BlurredHeightData; // Blur and sharpen tool don't change the height data itself in their first step but store the result in this array (which is used in a second step to modify the original height data).

    // Note that at this point YPos and XPos may be below array boundaries. The values aren't corrected before the loop by intention
    // since we need these values in their original form to calculate the right position in the modify weights array.
    for (int y=YPos; y<YMax; y++)
    {
        // We don't want to access the array below its boundaries.
        if (y<0)
        {
            BlurredHeightData.PushBackEmpty(XMax-XPos); // Still need to fill up blurred height data, so it can be accessed correctly in the second step.
            continue;
        }

        for (int x=XPos; x<XMax; x++)
        {
            // We don't want to access the array below its boundaries.
            if (x<0)
            {
                BlurredHeightData.PushBackEmpty(); // Still need to fill up blurred height data, so it can be accessed correctly in the second step.
                continue;
            }

            float ModifyWeight=m_ModifyWeights[(y-YPos)*ModifyWeightRes+x-XPos];

            unsigned short CurrentValue=m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x];

            // If the raise/lower tool is active.
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_RAISE)
            {
                // Calculate increase/decrease strength for this point using the precalculated modififaction matrix.
                unsigned short Strength=ToolEffect*ModifyWeight;

                if (MaxValue-CurrentValue<Strength) CurrentValue=MaxValue;
                else                                CurrentValue+=Strength;

                m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=CurrentValue;
            }

            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_LOWER)
            {
                // Calculate increase/decrease strength for this point using the precalculated modififaction matrix.
                unsigned short Strength=ToolEffect*ModifyWeight;

                if (CurrentValue<Strength) CurrentValue=0;
                else                       CurrentValue-=Strength;

                m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=CurrentValue;
            }

            // If the flatten tool is active.
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_FLATTEN)
            {
                m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=m_ReferenceHeight*ModifyWeight+(1-ModifyWeight)*CurrentValue;
            }

            // If the fill tool is active.
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_FILL)
            {
                if (CurrentValue<m_ReferenceHeight) m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=m_ReferenceHeight*ModifyWeight+(1-ModifyWeight)*CurrentValue;
            }

            // If the ablate tool is active.
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ABLATE)
            {
                if (CurrentValue>m_ReferenceHeight) m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=m_ReferenceHeight*ModifyWeight+(1-ModifyWeight)*CurrentValue;
            }

            // If the blur or sharpen tool is active we calculate a blurred version of this area and store it in the BlurredHeightData array.
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_BLUR || m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_SHARPEN)
            {
                // Only blur inside tool radius.
                if (ModifyWeight>0.0f)
                {
                    unsigned int Sum=0;
                    float        Amount=0;

                    for (int KernelY=0; KernelY<m_GaussWeightsRes; KernelY++)
                    {
                        for (int KernelX=0; KernelX<m_GaussWeightsRes; KernelX++)
                        {
                            int HeightDataYPos=y+KernelY-m_GaussWeightsRes/2;
                            int HeightDataXPos=x+KernelX-m_GaussWeightsRes/2;

                            if (HeightDataYPos<0 || HeightDataXPos<0 || HeightDataYPos>(int)m_TerrainCopy->GetResolution()-1 || HeightDataXPos>(int)m_TerrainCopy->GetResolution()-1)
                            {
                                continue; // Do nothing since we are outside array boundaries (this value is ignored).
                            }
                            else
                            {
                                Sum+=m_TerrainCopy->m_HeightData[HeightDataYPos*m_TerrainCopy->GetResolution()+HeightDataXPos]*m_GaussWeights[KernelY*m_GaussWeightsRes+KernelX];
                                Amount+=m_GaussWeights[KernelY*m_GaussWeightsRes+KernelX];
                            }
                        }
                    }

                    BlurredHeightData.PushBack(Sum/Amount);
                }
                else
                {
                    // Value isn't affected by the tools radius so put the current value into the array of computed values.
                    BlurredHeightData.PushBack(CurrentValue);
                }
            }

            // If the noise tool is active.
            if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_NOISE)
            {
                unsigned short Strength=ToolEffect*ModifyWeight;

                // Only use tool inside tool radius.
                if (ModifyWeight>0.0f)
                {
                    int Result=rand()%100;

                    if (Result>=10) continue;

                    for (int NoiseY=0; NoiseY<m_NoiseWeightsRes; NoiseY++)
                    {
                        for (int NoiseX=0; NoiseX<m_NoiseWeightsRes; NoiseX++)
                        {
                            int HeightDataYPos=y+NoiseY-m_NoiseWeightsRes/2;
                            int HeightDataXPos=x+NoiseX-m_NoiseWeightsRes/2;

                            if (HeightDataYPos<0 || HeightDataXPos<0 || HeightDataYPos>(int)m_TerrainCopy->GetResolution()-1 || HeightDataXPos>(int)m_TerrainCopy->GetResolution()-1)
                            {
                                continue; // Do nothing since we are outside array boundaries (this value is ignored).
                            }
                            else
                            {
                                unsigned short Current=m_TerrainCopy->m_HeightData[HeightDataYPos*m_TerrainCopy->GetResolution()+HeightDataXPos];
                                unsigned short ModVaue=Strength*m_NoiseWeights[NoiseY*m_NoiseWeightsRes+NoiseX];

                                if (Result>4)
                                {
                                    if (MaxValue-Current<ModVaue) Current=MaxValue;
                                    else                          Current+=ModVaue;
                                }
                                else
                                {
                                    if (Current<ModVaue) Current=0;
                                    else                 Current-=ModVaue;
                                }

                                m_TerrainCopy->m_HeightData[HeightDataYPos*m_TerrainCopy->GetResolution()+HeightDataXPos]=Current;
                            }
                        }
                    }
                }
            }
        }
    }

    // If we have worked with the blur or sharpen tool, the result has been written into a second array that we now need to copy into the first one.
    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_BLUR || m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_SHARPEN)
    {
        for (int y=YPos; y<YMax; y++)
        {
            if (y<0) continue; // We don't want to access the array below its boundaries.

            for (int x=XPos; x<XMax; x++)
            {
                if (x<0) continue; // We don't want to access the array below its boundaries.

                unsigned short BlurredValue=BlurredHeightData[(y-YPos)*(XMax-XPos)+(x-XPos)];

                float ModifyWeight=m_ModifyWeights[(y-YPos)*ModifyWeightRes+x-XPos];

                wxASSERT(ModifyWeight>=0.0f && ModifyWeight<=1.0f);

                unsigned short CurrentValue=m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x];

                // If the blur tool is active, we write the blurred value into the real height data array.
                if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_BLUR)
                {
                    CurrentValue-=(CurrentValue-BlurredValue)*ModifyWeight;
                }

                // If the sharpen tool is active we compare both values and increase or decrease the original value to increase contrast.
                if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_SHARPEN)
                {
                    float Strength=ToolEffect*ModifyWeight;

                    // If current value is bigger than the blurred value, increase it.
                    if (CurrentValue>BlurredValue)
                    {
                        if (MaxValue-CurrentValue<Strength) CurrentValue=MaxValue;
                        else                                CurrentValue+=Strength;
                    }

                    // If current value is smaller than the blurred value, decrease it.
                    if (CurrentValue<BlurredValue)
                    {
                        if (CurrentValue<Strength) CurrentValue=0;
                        else                       CurrentValue-=Strength;
                    }
                }

                // Set new value. Note: If new value and old value are the same this has no effect.
                m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=CurrentValue;
            }
        }
    }

    m_TerrainCopy->m_Terrain.UpdateHeights(&(m_TerrainCopy->m_HeightData[0]), AffectedArea.x, AffectedArea.y, AffectedArea.width, AffectedArea.height);
    m_RenderUpdateBitmap=true;
}


void ToolTerrainEditorT::SetTerrain(MapTerrainT* NewTerrain, bool UpdateObs_VisChanged)
{
    ArrayT<MapElementT*> VisChangedList;

    // If NewTerrain!=NULL, it *must* be an element of the map, but m_TerrainCopy never.
    wxASSERT(NewTerrain==NULL || NewTerrain!=m_TerrainCopy);

    // If a terrain is currently attached to the tool, remove it from the tool, delete its copy and make it visible in the map.
    // Note that NewTerrain==m_TerrainOrig is well possible here, but the caller relies on our full re-initialization here,
    // that is, on our *not* shortcutting e.g. by "if (NewTerrain==m_TerrainOrig) return;"
    if (m_TerrainOrig!=NULL)
    {
        wxASSERT(m_TerrainCopy!=NULL);

        // We stop editing m_TerrainOrig now: m_TerrainOrig is assigned NULL below.
        // This also changes the visibility of the terrain referred to by m_TerrainOrig,
        // as it is no longer affected by IsHiddenByTool().
        VisChangedList.PushBack(m_TerrainOrig);

        // Delete the working copy and make the original (possibly modified) terrain visible again.
        delete m_TerrainCopy;
        m_TerrainCopy=NULL;
        m_TerrainOrig=NULL;

        m_TerrainEditorDialog->UpdateResolution(65);
    }

    if (NewTerrain!=NULL)
    {
        // We start editing NewTerrain now: m_TerrainOrig is assigned NewTerrain below.
        // This also changes the visibility of the terrain referred to by m_TerrainOrig,
        // as it is now affected by IsHiddenByTool().
        VisChangedList.PushBack(NewTerrain);

        m_TerrainOrig=NewTerrain;
        m_TerrainCopy=new MapTerrainT(*m_TerrainOrig);

        m_TerrainEditorDialog->UpdateResolution(m_TerrainCopy->GetResolution());

        UpdateModifyWeights();
        UpdateNoiseWeights();
        UpdateGaussWeights();
    }

    m_HeightDataPos=wxPoint(-1, -1);
    UpdateToolInformation();

    m_RenderUpdateBitmap=true;
    m_RenderUpdateTool  =true;

    if (VisChangedList.Size()>0 && UpdateObs_VisChanged)
        m_MapDoc.UpdateAllObservers_Modified(VisChangedList, MEMD_VISIBILITY);
}


static float LineIntersect(const wxPoint& p1, const wxPoint& p2, const wxPoint& q1, const wxPoint& q2)
{
    wxPoint VecU(p2.x-p1.x, p2.y-p1.y);
    wxPoint VecV(q2.x-q1.x, q2.y-q1.y);
    wxPoint VecW(p1.x-q1.x, p1.y-q1.y);

    return float(VecV.y*VecW.x-VecV.x*VecW.y)/float(VecV.x*VecU.y-VecV.y*VecU.x);
}


static inline float RoadFunc(float Radius, float Distance, float Hardness)
{
    float Treshold=Radius*Hardness/100.0f;

    if (Distance<=Treshold) return 1.0f;

    // This is the smoothstep function from the book "The Cg Tutorial" chapter 5.5.2 page 137.
    return 1.0f-(-2.0f*pow((Distance-Treshold)/(Radius-Treshold), 3)+3.0f*pow((Distance-Treshold)/(Radius-Treshold), 2));
}


void ToolTerrainEditorT::CommitChanges()
{
    wxBusyCursor Wait;

    // If the road tool is active, construct the road here and commit the changes.
    if (m_TerrainEditorDialog->GetActiveTool()==TerrainEditorDialogT::ID_TOOL_ROAD && m_EditRoadParts.Size()>0)
    {
        // We need at least 2 road positions.
        if (m_EditRoadParts.Size()<2)
        {
            m_EditRoadParts.Clear();
            return;
        }

#if 1
        const float Hardness  =m_TerrainEditorDialog->GetHardness();
        const float Radius    =GetRadius();
        //const float SegMinDist=0.0f; // The minimum distance between segment start and endpoint.

        const wxPoint Begin(m_EditRoadParts[0].x+Radius,
                            m_EditRoadParts[0].y+Radius);
        const wxPoint End  (m_EditRoadParts[m_EditRoadParts.Size()-1].x+Radius,
                            m_EditRoadParts[m_EditRoadParts.Size()-1].y+Radius);

        const unsigned short BeginHeight=m_TerrainCopy->m_HeightData[Begin.x+Begin.y*m_TerrainCopy->GetResolution()];
        const unsigned short EndHeight  =m_TerrainCopy->m_HeightData[End  .x+End  .y*m_TerrainCopy->GetResolution()];
        const            int HeightDiff =EndHeight-BeginHeight;

        float TotalLength=0;

        unsigned long Interval=1;

        // Include the next two lines to create L form roads for debugging purposes.
#if 0
        Interval=(m_EditRoadParts.Size()-1)/2;      // The -1 is important when m_EditRoadParts.Size() is even.
        if (Interval<1) Interval=1;
#endif

        // Set begin of first segment to the first road reference point.
        {
            wxPoint SegBegin=Begin;

            // Loop over all segments, to get the length of the segment strip (needed to interpolate the height difference of the road).
            for (unsigned long i=Interval; i<m_EditRoadParts.Size(); i+=Interval)
            {
                wxPoint SegEnd(m_EditRoadParts[i].x+Radius, m_EditRoadParts[i].y+Radius);

                const wxPoint DirVec(SegEnd.x-SegBegin.x, SegEnd.y-SegBegin.y);

                float SegLength=sqrt(float(DirVec.x*DirVec.x+DirVec.y*DirVec.y));

                //if (SegLength<SegMinDist) continue;

                TotalLength+=SegLength;
                SegBegin=SegEnd;        // Next segment begins at the end of this segment.
            }
        }

        // Caluclate height value diff per length.
        const float HeightPerLength=float(HeightDiff)/float(TotalLength);

        for (unsigned long y=0; y<m_TerrainCopy->GetResolution(); y++)
        {
            if (int(y)<m_EditBounds.GetTop() || int(y)>m_EditBounds.GetBottom()) continue; // Exclude points that are not part of the edited bounds.

            for (unsigned long x=0; x<m_TerrainCopy->GetResolution(); x++)
            {
                if (int(x)<m_EditBounds.GetLeft() || int(x)>m_EditBounds.GetRight()) continue; // Exclude points that are not part of the edited bounds.

                float SegBeginHeight=BeginHeight; // Reset height value to beginning.

                ArrayT<float> HeightValues;
                ArrayT<float> Distances;
                ArrayT<float> Contribs;   // Contains values between 0 and 1 that define how much this segment value contributes to the overall value of this height data point.

                wxPoint SegBegin=Begin;

                // Loop over all segments.
                for (unsigned long i=Interval; i<m_EditRoadParts.Size(); i+=Interval)
                {
                    wxPoint SegEnd(m_EditRoadParts[i].x+Radius, m_EditRoadParts[i].y+Radius);

                    const wxPoint DirVec(SegEnd.x-SegBegin.x, SegEnd.y-SegBegin.y);

                    float SegLength=sqrt(float(DirVec.x*DirVec.x+DirVec.y*DirVec.y));

                    //if (SegLength<SegMinDist) continue;

                    float SegEndHeight =SegBeginHeight+SegLength*HeightPerLength;
                    float SegHeightDiff=SegEndHeight-SegBeginHeight;

                    float Intersect=LineIntersect(SegBegin, SegEnd, wxPoint(x,y), wxPoint(x,y)+wxPoint(-DirVec.y, DirVec.x));

                    float HeightVal=SegBeginHeight+SegHeightDiff*Intersect;

                    // Make sure height values lies within unsigned short boundaries.
                    if (HeightVal<0.0f)     HeightVal=0.0f;
                    if (HeightVal>65535.0f) HeightVal=65535.0f;

                    float Dist   =0.0f;
                    float Contrib=0.0f;

                    // Caluclate distance to the segments endpoint if point is outside line segment area.
                    if (Intersect<0.0f)
                    {
                        float DistY=SegBegin.y-float(y);
                        float DistX=SegBegin.x-float(x);
                              Dist =sqrt(DistY*DistY+DistX*DistX);

#if 1
                        Contrib=1.0f-Dist/Radius;
#else
                        Contrib=(Radius-DistX)*(Radius-DistY)/(Radius*Radius);
#endif
                    }
                    else if (Intersect>1.0f)
                    {
                        float DistY=SegEnd.y-float(y);
                        float DistX=SegEnd.x-float(x);
                              Dist =sqrt(DistY*DistY+DistX*DistX);

#if 1
                        Contrib=1.0f-Dist/Radius;
#else
                        Contrib=(Radius-DistX)*(Radius-DistY)/(Radius*Radius);
#endif
                    }
                    else
                    {
                        Vector3fT IntersectPoint(float(SegBegin.x)+Intersect*DirVec.x, float(SegBegin.y)+Intersect*DirVec.y, 0.0f);

                        float DistY=IntersectPoint.y-float(y);
                        float DistX=IntersectPoint.x-float(x);
                              Dist =sqrt(DistY*DistY+DistX*DistX);

                        Contrib=1.0f-Dist/Radius;
                    }

                    if (Contrib<0.0001f) Contrib=0.0001f;

                    if (Dist<=Radius)
                    {
                        HeightValues.PushBack(HeightVal);
                        Distances   .PushBack(Dist);
                        Contribs    .PushBack(Contrib);
                    }

                    SegBeginHeight=SegEndHeight;
                    SegBegin      =SegEnd;
                }

                wxASSERT(Distances.Size()==HeightValues.Size() && Distances.Size()==Contribs.Size());

                if (Distances.Size()>0)
                {
                    float Height_Numerator  =0.0f;
                    float Height_Denominator=0.0f;

                    float ShortestDistance=Radius+1.0f;

                    for (unsigned long i=0; i<Distances.Size(); i++)
                    {
                        Height_Numerator  +=HeightValues[i]*Contribs[i];
                        Height_Denominator+=Contribs[i];

                        if (Distances[i]<ShortestDistance) ShortestDistance=Distances[i];
                    }

                    float Weight=RoadFunc(Radius, ShortestDistance, Hardness);

                    wxASSERT(Weight>=0.0f && Weight<=1.0f);

                    // Include this to remove the weight from caluclation.
#if 0
                    Weight=1.0f;
#endif

                    unsigned short CurrentValue=m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x];

                    m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=Height_Numerator/Height_Denominator*Weight+(1.0f-Weight)*CurrentValue;
                }
            }
        }
#else
        const wxPoint Begin(m_EditRoadParts[0].x+GetRadius(),
                            m_EditRoadParts[0].y+GetRadius());
        const wxPoint End  (m_EditRoadParts[m_EditRoadParts.Size()-1].x+GetRadius(),
                            m_EditRoadParts[m_EditRoadParts.Size()-1].y+GetRadius());

        // Get the height values between the road is constructed.
        const unsigned short BeginHeight=m_TerrainCopy->m_HeightData[Begin.x+Begin.y*m_TerrainCopy->GetResolution()];
        const unsigned short EndHeight  =m_TerrainCopy->m_HeightData[End  .x+End  .y*m_TerrainCopy->GetResolution()];

        // Calculate height delta for each road position.
        const float HeightDelta=float(EndHeight-BeginHeight)/float(m_EditRoadParts.Size());

        // Calculate resolution of modify array here, so we don't need to do this in every loop iteration.
        int ModifyWeightRes=GetRadius()*2+1;

        for (unsigned long i=0; i<m_EditRoadParts.Size(); i++)
        {
            // Now use the flatten tool in the area with the according height value.
            const unsigned short HeightVal=BeginHeight+HeightDelta*i;

            for (int y=m_EditRoadParts[i].y; y<m_EditRoadParts[i].GetBottom(); y++)
            {
                if (y<0) continue;

                for (int x=m_EditRoadParts[i].x; x<m_EditRoadParts[i].GetRight(); x++)
                {
                    if (x<0) continue;

                    float ModifyWeight=m_ModifyWeights[(y-m_EditRoadParts[i].y)*ModifyWeightRes+x-m_EditRoadParts[i].x];

                    unsigned short CurrentValue=m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x];

                    m_TerrainCopy->m_HeightData[y*m_TerrainCopy->GetResolution()+x]=HeightVal*ModifyWeight+(1-ModifyWeight)*CurrentValue;
                }
            }
        }
#endif

        m_EditRoadParts.Clear();
        m_RenderUpdateBitmap=true;
        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    }

    // Resolution changes should have been handled in their own map command before this call.
    wxASSERT(m_TerrainOrig->GetResolution()==m_TerrainCopy->GetResolution());

    ArrayT<unsigned short> HeightDataTmp;

    for (int y=m_EditBounds.y; y<m_EditBounds.y+m_EditBounds.height; y++)
    {
        for (int x=m_EditBounds.x; x<m_EditBounds.x+m_EditBounds.width; x++)
        {
            HeightDataTmp.PushBack(m_TerrainCopy->m_HeightData[y*m_TerrainCopy->m_Resolution+x]);
        }
    }

    CommandT* Command=new CommandModifyTerrainT(m_MapDoc, m_TerrainOrig, HeightDataTmp, wxPoint(m_EditBounds.x, m_EditBounds.y), m_EditBounds.width, m_EditBounds.height);

    // Our copy has already been updated, so we don't need to recursively update it again.
    m_IsRecSelfNotify=true;
    m_MapDoc.CompatSubmitCommand(Command);
    m_IsRecSelfNotify=false;

    // Set edit bounds empty since all changes have been commited.
    m_EditBounds=wxRect();

    // All height data editing operations are finished, so terrain needs a complete update.
    m_TerrainCopy->m_NeedsUpdate=true;
}


float ToolTerrainEditorT::GetToolHeight_World() const
{
    const int             FlipY    =m_TerrainCopy->m_Resolution-1-m_HeightDataPos.y;
    const unsigned short  Height16 =m_TerrainCopy->m_HeightData[FlipY*m_TerrainCopy->GetResolution()+m_HeightDataPos.x];
    const BoundingBox3fT& TerrainBB=m_TerrainCopy->m_TerrainBounds;

    return TerrainBB.Min.z+((TerrainBB.Max.z-TerrainBB.Min.z)*float(Height16)/65535.0f);
}


wxBitmap ToolTerrainEditorT::GetScaledBitmap(float x, float y, float width, float height, unsigned int NewSizeX, unsigned int NewSizeY) const
{
    // Set current color gradient.
    const unsigned long* CurrentColorGradient=ColorGradients.Colors[(int)m_CurrentColorGradient];

    wxBitmap                    Bitmap    (NewSizeX, NewSizeY, 24);
    wxNativePixelData           BitmapData(Bitmap);
    wxNativePixelData::Iterator It        (BitmapData);

    const float RatioX=float(width )/float(NewSizeX-1);
    const float RatioY=float(height)/float(NewSizeY-1);

    // We read the terrains height data from bottom to top to flip y data horizontally.
    const float YOffset=float(m_TerrainCopy->GetResolution()-1)-y;

    for (unsigned int ny=0; ny<NewSizeY; ny++)
    {
        const unsigned int oy=(YOffset-float(ny)*RatioY)+0.5f; //+0.5f to round.

        for (unsigned int nx=0; nx<NewSizeX; nx++)
        {
            const unsigned int  ox=(x+float(nx)*RatioX)+0.5f; //+0.5f to round.
            const unsigned long ColorValue=CurrentColorGradient[m_TerrainCopy->m_HeightData[oy*m_TerrainCopy->GetResolution()+ox]>>5];

            It.MoveTo(BitmapData, nx, ny);

            It.Red()  =(unsigned char)(ColorValue>>16);
            It.Green()=(unsigned char)(ColorValue>>8 );
            It.Blue() =(unsigned char)(ColorValue    );
        }
    }

    return Bitmap;
}


void ToolTerrainEditorT::PickReHeightValue()
{
    // Pick reference height if tool position is valid.
    if (m_HeightDataPos.y>=0 && m_HeightDataPos.x>=0)
    {
        // Flip y position.
        int FlipY=m_TerrainCopy->m_Resolution-1-m_HeightDataPos.y;

        // Set reference point of flatten tool to heigth value of the point the user clicked on.
        m_ReferenceHeight=m_TerrainCopy->m_HeightData[FlipY*m_TerrainCopy->m_Resolution+m_HeightDataPos.x];
    }
}


int ToolTerrainEditorT::GetRadius() const
{
    if (m_TerrainCopy==NULL) return 1;

    const float TerrainSizeX=m_TerrainCopy->m_TerrainBounds.Max.x-m_TerrainCopy->m_TerrainBounds.Min.x;

    return float(m_TerrainEditorDialog->GetRadius())/TerrainSizeX*float(m_TerrainCopy->GetResolution());
}


void ToolTerrainEditorT::UpdateToolInformation()
{
    int Height=-1;

    if (m_HeightDataPos.x>-1 && m_HeightDataPos.y>-1)
        Height=m_TerrainCopy->m_HeightData[(m_TerrainCopy->m_Resolution-1-m_HeightDataPos.y)*m_TerrainCopy->m_Resolution+m_HeightDataPos.x];

    m_MapDoc.GetChildFrame()->SetStatusText(wxString::Format("Tool position: %i, %i, %i", m_HeightDataPos.x, m_HeightDataPos.y, Height), ChildFrameT::SBP_SELECTION);
}
