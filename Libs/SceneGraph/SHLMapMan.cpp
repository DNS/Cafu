/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SHLMapMan.hpp"
#include "MaterialSystem/MapComposition.hpp"
#include "MaterialSystem/TextureMap.hpp"

using namespace cf::SceneGraph;


const unsigned long cf::SceneGraph::SHLMapManT::SIZE_S=256;
const unsigned long cf::SceneGraph::SHLMapManT::SIZE_T=256;

char         cf::SceneGraph::SHLMapManT::NrOfBands =0;      // Default value.
unsigned int cf::SceneGraph::SHLMapManT::NrOfRepres=0;      // Default value (no representatives - no compression).


SHLMapManT::SHLMapT::SHLMapT()
{
    const unsigned long NR_OF_SH_COEFFS=NrOfBands * NrOfBands;

    if (NrOfRepres>0)
    {
        // Compressed, use the indices.
        while (Indices.Size()<(unsigned long)SIZE_S*SIZE_T)
            Indices.PushBack(0);
    }
    else
    {
        // Not compressed, use direct coeff storage.
        while (Coeffs.Size()<SIZE_S*SIZE_T*NR_OF_SH_COEFFS)
            Coeffs.PushBack(0.0);
    }
}


SHLMapManT::SHLMapManT()
    : SHLRepresentativesTexture(NULL),
      SHLRepresentativesTexWidth(0)
{
    for (unsigned long s=0; s<SIZE_S; s++)
        BitmapAllocated.PushBack(SIZE_T);
}


SHLMapManT::~SHLMapManT()
{
    for (unsigned long BitmapNr=0; BitmapNr<SHLMaps.Size(); BitmapNr++)
        delete SHLMaps[BitmapNr];


    if (MatSys::TextureMapManager!=NULL)
    {
        for (unsigned long SHLMapNr=0; SHLMapNr<SHLMapTextures.Size(); SHLMapNr++)
            for (unsigned long TexNr=0; TexNr<SHLMapTextures[SHLMapNr].Size(); TexNr++)
            {
                MatSys::TextureMapManager->FreeTextureMap(SHLMapTextures[SHLMapNr][TexNr]);
                SHLMapTextures[SHLMapNr][TexNr]=NULL;
            }

        MatSys::TextureMapManager->FreeTextureMap(SHLRepresentativesTexture);
        SHLRepresentativesTexture=NULL;
    }
}


void SHLMapManT::ReadSHLCoeffsTable(std::istream& InFile)
{
    for (unsigned long RepresNr=0; RepresNr<NrOfRepres; RepresNr++)
        for (unsigned long CoeffNr=0; CoeffNr<(unsigned long)NrOfBands * NrOfBands; CoeffNr++)
        {
            float ReadCoeff;
            InFile.read((char*)&ReadCoeff, sizeof(ReadCoeff));

            SHLCoeffsTable.PushBack(ReadCoeff);
        }
}


void SHLMapManT::WriteSHLCoeffsTable(std::ostream& OutFile) const
{
    for (unsigned long CoeffNr=0; CoeffNr<SHLCoeffsTable.Size(); CoeffNr++)
        OutFile.write((char*)&SHLCoeffsTable[CoeffNr], sizeof(float));
}


bool SHLMapManT::Allocate(unsigned long SizeS, unsigned long SizeT, unsigned long& BitmapNr, unsigned long& PosS, unsigned long& PosT)
{
    if (SizeS>SIZE_S) return false;
    if (SizeT>SIZE_T) return false;

    BitmapNr=SHLMaps.Size()-1;
    if (AllocateHelper(SizeS, SizeT, PosS, PosT)) return true;

    SHLMaps.PushBack(new SHLMapT);
    SHLMapTextures.PushBackEmpty();

    for (unsigned long s=0; s<SIZE_S; s++)
        BitmapAllocated[s]=0;

    BitmapNr=SHLMaps.Size()-1;
    return AllocateHelper(SizeS, SizeT, PosS, PosT);
}


void SHLMapManT::InitTextures()
{
    // Allocate SHLMapTextures in full depth.
    if (NrOfRepres>0)
    {
        // Compressed SHL coeffs: For each SHLMap, only store the 16-bit indices, requiring only a single texture (R and G components).
        for (unsigned long SHLMapNr=0; SHLMapNr<SHLMapTextures.Size(); SHLMapNr++)
        {
            ArrayT<unsigned long> TexData;

            // Hm. Why does g++ promote SIZE_S*SIZE_T to type "int"???
            // (Which is signed, instead of unsigned!?!)
            for (unsigned long ElementNr=0; ElementNr<(unsigned long)(SIZE_S)*SIZE_T; ElementNr++)
            {
                // This puts the indices low byte into the red component, and the high byte into the green component of the RGBA pixel.
                TexData.PushBack(SHLMaps[SHLMapNr]->Indices[ElementNr]);
            }

            SHLMapTextures[SHLMapNr].PushBack(MatSys::TextureMapManager->GetTextureMap2D(
                (char*)&TexData[0],
                SIZE_S, SIZE_T,
                4, true,
                // This is an index-texture, thus we have to use Nearest filtering! The wrap mode (clamp, repeat, etc.)
                // doesn't play a role for SHLMaps, but setting NoScaleDown and NoCompression are mandatory (same as filtering).
                MapCompositionT(MapCompositionT::Nearest, MapCompositionT::Nearest, MapCompositionT::Repeat, MapCompositionT::Repeat, true, true)));
        }


        // Also upload the SHLRepresentatives texture.
        ArrayT<unsigned long> TexData;

        // n Bands yield n*n SHL coeffs, which in turn require NrOfPixels=ceil(n*n/4) pixels per SHL vector.
        // The SHLCoeffTable contains R (# of representatives) SHL vectors,
        // which are stored in columns of 256 rows each, yielding NrOfColumns=ceil(R/256) columns.
        const unsigned long NR_OF_SH_COEFFS    =NrOfBands * NrOfBands;
        const unsigned long NrOfColumns        =(NrOfRepres+255)/256;   // =ceil(double(NrOfRepres)/256);
        const unsigned long NrOfPixelsPerVector=(NR_OF_SH_COEFFS+3)/4;

        unsigned long Width=1; while (Width<NrOfColumns*NrOfPixelsPerVector) Width*=2;
        SHLRepresentativesTexWidth=Width;

        // TexData.PushBackEmpty(Width*256);
        while (TexData.Size()<Width*256) TexData.PushBack(0);

        for (unsigned long ColumnNr=0; ColumnNr<NrOfColumns; ColumnNr++)
        {
            for (unsigned long RowNr=0; RowNr<256; RowNr++)
            {
                if (ColumnNr*256+RowNr>=NrOfRepres) break;

                for (unsigned long PixelNr=0; PixelNr<NrOfPixelsPerVector; PixelNr++)
                {
                    const float*  SHLVector=&SHLCoeffsTable[(ColumnNr*256+RowNr)*NR_OF_SH_COEFFS];
                    unsigned long d[4];

                    for (unsigned long i=0; i<4; i++)
                    {
                        float c=PixelNr*4+i<NR_OF_SH_COEFFS ? SHLVector[PixelNr*4+i] : 0.0f;

                        if (c<-2.0) c=-2.0;
                        if (c> 2.0) c= 2.0;

                        d[i]=char((c+2.0)/4.0*255.0+0.49);
                    }

                    TexData[RowNr*Width + ColumnNr*NrOfPixelsPerVector+PixelNr]=(d[3] << 24) + (d[2] << 16) + (d[1] << 8) + (d[0] << 0);
                }
            }
        }

        // BitmapT test(Width, 256, &TexData[0]);
        // test.SaveToDisk("Debug.bmp");
        // EnqueueString("There are %lu representatives with %lu SH coeffs each.", NrOfRepres, NR_OF_SH_COEFFS);
        // EnqueueString("They are stored in %lu columns (%lu pixels per column).", NrOfColumns, NrOfPixelsPerVector);
        // EnqueueString("The look-up texture thus has dimensions %lu x 256.", Width);

        SHLRepresentativesTexture=MatSys::TextureMapManager->GetTextureMap2D(
            (char*)&TexData[0],
            Width, 256,
            4, true,
            // This is a lookup-texture, thus we have to use Nearest! The wrap mode (clamp, repeat, etc.)
            // doesn't play a role for SHL Representatives textures, but setting NoScaleDown and NoCompression are mandatory (same as filtering).
            MapCompositionT(MapCompositionT::Nearest, MapCompositionT::Nearest, MapCompositionT::Repeat, MapCompositionT::Repeat, true, true));
    }
    else
    {
        // Uncompressed SHL coeffs: For each SHLMap, allocate enough textures to store all SHL coeffs "immediately".
        // Each texture holds 4 SHL coeffs in a RGBA pixel, thus NrOfSHLCoeffs/4 textures are required.
        float Min=0;
        float Max=0;

        for (unsigned long SHLMapNr=0; SHLMapNr<SHLMapTextures.Size(); SHLMapNr++)
        {
            const unsigned long NR_OF_SH_COEFFS=NrOfBands * NrOfBands;

            // For non-compressed SHL coeff storage, the number of texture objects per SHLMap is
            // (NR_OF_SH_COEFFS+3)/4 == ceil(NR_OF_SH_COEFFS/4), each with four coefficients (RGBA) per texture.
            for (unsigned long TexObjNr=0; TexObjNr<(NR_OF_SH_COEFFS+3)/4; TexObjNr++)
            {
                ArrayT<char> TexData;
                TexData.PushBackEmpty(SIZE_S*SIZE_T*4);

                // Hm. Why does g++ promote SIZE_S*SIZE_T to type "int"???
                // (Which is signed, instead of unsigned!?!)
                for (unsigned long ElementNr=0; ElementNr<(unsigned long)(SIZE_S)*SIZE_T; ElementNr++)
                    for (unsigned long CoeffNr=TexObjNr*4; CoeffNr<TexObjNr*4+4 && CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                    {
                        float Value=SHLMaps[SHLMapNr]->Coeffs[ElementNr*NR_OF_SH_COEFFS+CoeffNr];

                        if (Value<Min) Min=Value;
                        if (Value>Max) Max=Value;

                        // In theory, Value is a float in range [-4*Pi, 4*Pi].
                        // In practise, however, the range does not exceed [-2, 2].
                        // Therefore, clamp all values to this range, and compress it into an unsigned char.
                        if (Value<-2.0) Value=-2.0;
                        if (Value> 2.0) Value= 2.0;

                        TexData[ElementNr*4+(CoeffNr-TexObjNr*4)]=char((Value+2.0)/4.0*255.0+0.49);
                    }

                SHLMapTextures[SHLMapNr].PushBack(MatSys::TextureMapManager->GetTextureMap2D(
                    &TexData[0],
                    SIZE_S, SIZE_T,
                    4, true,
                    // The wrap mode (clamp, repeat, etc.) doesn't play a role for SHLMaps,
                    // but NoScaleDown and NoCompression should be set, because many small SHLMaps may share a single bitmap.
                    MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear, MapCompositionT::Repeat, MapCompositionT::Repeat, true, true)));
            }
        }

        // EnqueueString("SHL Results: Min %f   Max %f", Min, Max);
    }
}


bool SHLMapManT::AllocateHelper(unsigned long SizeS, unsigned long SizeT, unsigned long& PosS, unsigned long& PosT)
{
    unsigned long Best=SIZE_T;

    for (unsigned long s=0; s<=SIZE_S-SizeS; s++)
    {
        unsigned long Best2=0;
        unsigned long s2;

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

    for (unsigned long s=0; s<SizeS; s++) BitmapAllocated[PosS+s]=Best+SizeT;
    return true;
}
