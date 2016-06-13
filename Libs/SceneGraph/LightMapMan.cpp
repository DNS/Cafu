/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "LightMapMan.hpp"
#include "Bitmap/Bitmap.hpp"
#include "MaterialSystem/MapComposition.hpp"
#include "MaterialSystem/TextureMap.hpp"

using namespace cf::SceneGraph;


const unsigned int cf::SceneGraph::LightMapManT::SIZE_S     =256;
const unsigned int cf::SceneGraph::LightMapManT::SIZE_T     =256;
const unsigned int cf::SceneGraph::LightMapManT::INIT_COLOR1=0xFF000000;    // Black with 100% alpha.
const unsigned int cf::SceneGraph::LightMapManT::INIT_COLOR2=0xFF808080;    // 50% gray (==zero-vector) with 100% alpha (which is the implicit factor contained in the RGB lightmap color of Bitmap1 - see e.g. FaceNodeT::BackToLightMap() for details).


LightMapManT::LightMapManT()
{
    for (unsigned int s=0; s<SIZE_S; s++)
        BitmapAllocated.PushBack(SIZE_T);
}


LightMapManT::~LightMapManT()
{
    for (unsigned long BitmapNr=0; BitmapNr<Bitmaps.Size(); BitmapNr++)
    {
        delete Bitmaps [BitmapNr];
        delete Bitmaps2[BitmapNr];
    }


    if (MatSys::TextureMapManager!=NULL)
    {
        for (unsigned long TextureNr=0; TextureNr<Textures.Size(); TextureNr++)
        {
            MatSys::TextureMapManager->FreeTextureMap(Textures[TextureNr]);
            Textures[TextureNr]=NULL;

            MatSys::TextureMapManager->FreeTextureMap(Textures2[TextureNr]);
            Textures2[TextureNr]=NULL;
        }
    }
}


bool LightMapManT::Allocate(unsigned int SizeS, unsigned int SizeT, unsigned long& BitmapNr, unsigned int& PosS, unsigned int& PosT)
{
    if (SizeS>SIZE_S) return false;
    if (SizeT>SIZE_T) return false;

    BitmapNr=Bitmaps.Size()-1;
    if (AllocateHelper(SizeS, SizeT, PosS, PosT)) return true;

    BitmapT* Bitmap1=new BitmapT(SIZE_S, SIZE_T);
    BitmapT* Bitmap2=new BitmapT(SIZE_S, SIZE_T);

    for (unsigned long i=0; i<Bitmap1->Data.Size(); i++) Bitmap1->Data[i]=INIT_COLOR1;
    for (unsigned long i=0; i<Bitmap2->Data.Size(); i++) Bitmap2->Data[i]=INIT_COLOR2;

    Bitmaps .PushBack(Bitmap1);
    Bitmaps2.PushBack(Bitmap2);

    Textures .PushBack(NULL);
    Textures2.PushBack(NULL);

    for (unsigned int s=0; s<SIZE_S; s++)
        BitmapAllocated[s]=0;

    BitmapNr=Bitmaps.Size()-1;
    return AllocateHelper(SizeS, SizeT, PosS, PosT);
}


void LightMapManT::InitTextures(const float Gamma, const int AmbientR, const int AmbientG, const int AmbientB)
{
    for (unsigned long LightMapNr=0; LightMapNr<Bitmaps.Size(); LightMapNr++)
    {
        BitmapT GammaBitmap(*Bitmaps[LightMapNr]);

        if (Gamma!=1.0f) GammaBitmap.ApplyGamma(Gamma);

        if (AmbientR>0 || AmbientG>0 || AmbientB>0)
        {
            for (unsigned int y=0; y<GammaBitmap.SizeY; y++)
                for (unsigned int x=0; x<GammaBitmap.SizeX; x++)
                {
                    int r, g, b;
                    GammaBitmap.GetPixel(x, y, r, g, b);

                    if (r<AmbientR) r=AmbientR;
                    if (g<AmbientG) g=AmbientG;
                    if (b<AmbientB) b=AmbientB;

                    GammaBitmap.SetPixel(x, y, r, g, b);
                }
        }

        Textures[LightMapNr]=MatSys::TextureMapManager->GetTextureMap2D(
            &GammaBitmap,
            true,
            // The wrap mode (clamp, repeat, etc.) doesn't play a role for LightMaps,
            // but NoScaleDown should be set, because many small LightMaps may share a single bitmap.
            // The same is true for NoCompression, it too mixes up pixels that should not be mixed.
            MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear, MapCompositionT::Repeat, MapCompositionT::Repeat, true, true));

        Textures2[LightMapNr]=MatSys::TextureMapManager->GetTextureMap2D(
            Bitmaps2[LightMapNr],
            true,
            // The wrap mode (clamp, repeat, etc.) doesn't play a role for LightMaps,
            // but NoScaleDown should be set, because many small LightMaps may share a single bitmap.
            // The same is true for NoCompression, it too mixes up pixels that should not be mixed.
            MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear, MapCompositionT::Repeat, MapCompositionT::Repeat, true, true));
    }
}


bool LightMapManT::AllocateHelper(unsigned int SizeS, unsigned int SizeT, unsigned int& PosS, unsigned int& PosT)
{
    unsigned int Best=SIZE_T;

    for (unsigned int s=0; s<=SIZE_S-SizeS; s++)
    {
        unsigned int Best2=0;
        unsigned int s2;

        for (s2=0; s2<SizeS; s2++)
        {
            if (BitmapAllocated[s+s2]>=Best) break;
            if (BitmapAllocated[s+s2]>Best2) Best2=BitmapAllocated[s+s2];
        }

        if (s2==SizeS)
        {
            // Gültige Position gefunden
            PosS=s;
            PosT=Best=Best2;
        }
    }

    if (Best+SizeT>SIZE_T) return false;

    for (unsigned int s=0; s<SizeS; s++) BitmapAllocated[PosS+s]=Best+SizeT;
    return true;
}
