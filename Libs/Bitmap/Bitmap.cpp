/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include <math.h>
#include <stdio.h>

#include "Bitmap.hpp"
#include "FileSys/FileMan.hpp"
#include "FileSys/File.hpp"
#include "png.h"

extern "C"
{
    #include "jpeglib.h"
}

#ifndef _WIN32
#include <string.h>
#define _stricmp strcasecmp
#endif


// This method is defined in ./jdatasrc.cpp
void jpeg_FileSys_src(j_decompress_ptr cinfo, cf::FileSys::InFileI* JpegFile_);


/// Callback function for the png library for reading data from cf::FileSys::InFileT* files.
static void MyFileSysBasedPngReadFunction(png_structp png_ptr, png_bytep data, png_size_t length)
{
    cf::FileSys::InFileI* PngFile  =static_cast<cf::FileSys::InFileI*>(png_get_io_ptr(png_ptr));
    const uint32_t        BytesRead=PngFile->Read((char*)data, uint32_t(length));

    if (BytesRead<length) png_error(png_ptr, "Read Error");
}


static void MyCustomLibpngErrorHandler(png_structp png_ptr, png_const_charp error_msg)
{
    // Display the error message (as is the behaviour of the libjpeg default error handler).
    fprintf(stderr, "libpng error (handled): %s\n", error_msg);

    // No matter whether it's a load or save error, we throw a LoadErrorT object.
    throw BitmapT::LoadErrorT();
}


static void MyCustomLibjpegErrorHandler(j_common_ptr cinfo)
{
    // Display the error message (as is the behaviour of the libjpeg default error handler).
    cinfo->err->output_message(cinfo);

    // No matter whether it's a load or save error, we throw a LoadErrorT object.
    throw BitmapT::LoadErrorT();
}


const char FileNotFoundBitmapData[128*16]=
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
    0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,0,1,1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,
    0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,0,1,1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,0,1,1,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,0,1,1,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,1,1,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


BitmapT::BitmapT() : SizeX(0), SizeY(0)
{
}


static uint32_t ReadTgaPixel(cf::FileSys::InFileI* TgaFile, char ImagePixelSize)
{
    char Blue;  TgaFile->Read(&Blue,  sizeof(Blue ));
    char Green; TgaFile->Read(&Green, sizeof(Green));
    char Red;   TgaFile->Read(&Red,   sizeof(Red  ));
    char Alpha; if (ImagePixelSize==32) TgaFile->Read(&Alpha, sizeof(Alpha)); else Alpha=char(0xFF);

    const uint32_t b=Blue;
    const uint32_t g=Green;
    const uint32_t r=Red;
    const uint32_t a=Alpha;

    return (a << 24)+(b << 16)+(g << 8)+r;
}


BitmapT::BitmapT(const char* FileName) /*throw (LoadErrorT)*/ : SizeX(0), SizeY(0)
{
    if (FileName==NULL) throw LoadErrorT();

    const size_t FileNameLength=strlen(FileName);

    // File names must at least be five characters long, as in "x.bmp".
    if (FileNameLength<5) throw LoadErrorT();

    if (_stricmp(&FileName[FileNameLength-4], ".bmp")==0)
    {
        // It is a BMP (Windows Bitmap) file.
        cf::FileSys::InFileI* BmpFile=cf::FileSys::FileMan->OpenRead(FileName);
        if (BmpFile==NULL) throw LoadErrorT();

        uint16_t Data2;
        uint32_t Data4;

        // Header einlesen
        BmpFile->Read((char*)&Data2, 2); if (Data2!=19778) { cf::FileSys::FileMan->Close(BmpFile); throw LoadErrorT(); }    // 19778==0x4D42=="BM"
        BmpFile->Read((char*)&Data4, 4); if (Data4 < 54+3) { cf::FileSys::FileMan->Close(BmpFile); throw LoadErrorT(); }    // TotalSize==HeaderSize(==sizeof(BitmapHeaderT)==54)+PaletteSize+ImageSize
        BmpFile->Read((char*)&Data2, 2);                                                                                    // Overread first  reserved word
        BmpFile->Read((char*)&Data2, 2);                                                                                    // Overread second reserved word
        BmpFile->Read((char*)&Data4, 4); if (Data4!=   54) { cf::FileSys::FileMan->Close(BmpFile); throw LoadErrorT(); }    // Offset in bytes to ImageData=HeaderSize(=54)+PaletteSize
        BmpFile->Read((char*)&Data4, 4); if (Data4!=   40) { cf::FileSys::FileMan->Close(BmpFile); throw LoadErrorT(); }    // Header-bytes to come
        BmpFile->Read((char*)&SizeX, 4);                                                                                    // SizeX
        BmpFile->Read((char*)&SizeY, 4);                                                                                    // SizeY
        BmpFile->Read((char*)&Data2, 2); if (Data2!=    1) { cf::FileSys::FileMan->Close(BmpFile); throw LoadErrorT(); }    // Planes
        BmpFile->Read((char*)&Data2, 2); if (Data2!=   24) { cf::FileSys::FileMan->Close(BmpFile); throw LoadErrorT(); }    // BPP
        BmpFile->Read((char*)&Data4, 4); if (Data4!=    0) { cf::FileSys::FileMan->Close(BmpFile); throw LoadErrorT(); }    // Compression
        BmpFile->Read((char*)&Data4, 4);                                                                                    // Ignore the image size - often wrong
        BmpFile->Read((char*)&Data4, 4);                                                                                    // Ignore
        BmpFile->Read((char*)&Data4, 4);                                                                                    // Ignore
        BmpFile->Read((char*)&Data4, 4);                                                                                    // Ignore
        BmpFile->Read((char*)&Data4, 4);                                                                                    // Ignore

        // Speicher allokieren
        Data.PushBackEmpty(SizeX*SizeY);

        // BitMap einlesen
        for (unsigned int PosY=0; PosY<SizeY; PosY++)
        {
            for (unsigned int PosX=0; PosX<SizeX; PosX++)
            {
                char Blue;  BmpFile->Read(&Blue , 1);
                char Green; BmpFile->Read(&Green, 1);
                char Red;   BmpFile->Read(&Red  , 1);

                const uint32_t r=Red;
                const uint32_t g=Green;
                const uint32_t b=Blue;
                const uint32_t a=255;

                Data[PosX+(SizeY-PosY-1)*SizeX]=(a << 24)+(b << 16)+(g << 8)+r;
            }

            // Read padding bytes because the next row starts at a multiple of four.
            BmpFile->Read((char*)&Data4, SizeX % 4);
        }

        cf::FileSys::FileMan->Close(BmpFile);
    }
    else if (_stricmp(&FileName[FileNameLength-4], ".tga")==0)
    {
        // It is a TGA (TrueVision Targa) file.
        cf::FileSys::InFileI* TgaFile=cf::FileSys::FileMan->OpenRead(FileName);
        if (TgaFile==NULL) throw LoadErrorT();

        char SizeOfIDField; TgaFile->Read(&SizeOfIDField, sizeof(SizeOfIDField));
        char ColorMapType;  TgaFile->Read(&ColorMapType , sizeof(ColorMapType ));
        char ImageTypeCode; TgaFile->Read(&ImageTypeCode, sizeof(ImageTypeCode));

        if (ColorMapType!=0)
        {
            // printf("%s: ColorMapType==%u, expected 0.\n", FileName, ColorMapType);
            // printf("Sorry - in this version I am unable to proceed. Best is you convert the image!\n");
            cf::FileSys::FileMan->Close(TgaFile);
            throw LoadErrorT();
        }

        if (ImageTypeCode!=2 && ImageTypeCode!=10)
        {
            // printf("%s: ImageTypeCode==%u, expected 2.\n", FileName, ImageTypeCode);
            // printf("Sorry - in this version I am unable to proceed. Best is you convert the image!\n");
            cf::FileSys::FileMan->Close(TgaFile);
            throw LoadErrorT();
        }

        for (unsigned int i=0; i<5; i++) { char Dummy; TgaFile->Read(&Dummy, sizeof(Dummy)); }   // Overread ColorMapSpecification.

        unsigned short OriginX;              TgaFile->Read((char*)&OriginX      , sizeof(OriginX             ));
        unsigned short OriginY;              TgaFile->Read((char*)&OriginY      , sizeof(OriginY             ));
        unsigned short SizeX_;               TgaFile->Read((char*)&SizeX_       , sizeof(SizeX_              ));
        unsigned short SizeY_;               TgaFile->Read((char*)&SizeY_       , sizeof(SizeY_              ));
        char           ImagePixelSize;       TgaFile->Read(&ImagePixelSize      , sizeof(ImagePixelSize      ));
        char           ImageDescriptionByte; TgaFile->Read(&ImageDescriptionByte, sizeof(ImageDescriptionByte));

        SizeX=SizeX_;
        SizeY=SizeY_;

        // printf("%s: OriginX %u, OriginY %u, SizeX %u, SizeY %u, ImgPixelSize %u, ImgDescriptionByte 0x%x\n", FileName, OriginX, OriginY, SizeX, SizeY, ImagePixelSize, ImageDescriptionByte);

        if (ImagePixelSize!=32 && ImagePixelSize!=24)
        {
            // printf("%s: ImagePixelSize==%u, expected 24 or 32.\n", TextureName, ImagePixelSize);
            // printf("Sorry - in this version I am unable to proceed. Best is you convert the image!\n");
            cf::FileSys::FileMan->Close(TgaFile);
            throw LoadErrorT();
        }

        for (int i=0; i<SizeOfIDField; i++) { char Dummy; TgaFile->Read(&Dummy, sizeof(Dummy)); }   // Overread ImageIdentificationField.


        // Speicher allokieren
        Data.PushBackEmpty(SizeX*SizeY);

        // BitMap einlesen
        if (ImageTypeCode==2)
        {
            // Uncompressed true-color image.
            for (unsigned int PosY=0; PosY<SizeY; PosY++)
                for (unsigned int PosX=0; PosX<SizeX; PosX++)
                {
                    const unsigned int DestPosX=(ImageDescriptionByte & 0x10)==0 ? PosX : SizeX-PosX-1;
                    const unsigned int DestPosY=(ImageDescriptionByte & 0x20)==0 ? SizeY-PosY-1 : PosY;

                    Data[DestPosX+DestPosY*SizeX]=ReadTgaPixel(TgaFile, ImagePixelSize);
                }
        }
        else // ImageTypeCode==10
        {
            // Run-length encoded true-color image.
            for (unsigned int PosY=0; PosY<SizeY; PosY++)
                for (unsigned int PosX=0; PosX<SizeX; PosX++)
                {
                    unsigned char RepetitionCount;
                    TgaFile->Read((char*)&RepetitionCount,  sizeof(RepetitionCount));

                    // Is this a run-length packet or a raw packet?
                    const bool IsRunLengthPacket=(RepetitionCount & 0x80)!=0;
                    RepetitionCount=(RepetitionCount & 0x7F)+1;     // Update the repetition count according to the file spec.

                    uint32_t Pixel=0;
                    if (IsRunLengthPacket) Pixel=ReadTgaPixel(TgaFile, ImagePixelSize);

                    for (unsigned long RepNr=0; RepNr<RepetitionCount; RepNr++)
                    {
                        if (!IsRunLengthPacket) Pixel=ReadTgaPixel(TgaFile, ImagePixelSize);

                        unsigned int DestPosX=(ImageDescriptionByte & 0x10)==0 ? PosX : SizeX-PosX-1;
                        unsigned int DestPosY=(ImageDescriptionByte & 0x20)==0 ? SizeY-PosY-1 : PosY;

                        Data[DestPosX+DestPosY*SizeX]=Pixel;

                        PosX++;
                    }

                    // This is necessary or else there is a double-increment for the last pixel of the packet.
                    PosX--;
                }
        }

        cf::FileSys::FileMan->Close(TgaFile);
    }
    else if (_stricmp(&FileName[FileNameLength-4], ".png")==0)
    {
        // It is a PNG (Portable Network Graphics) file.
        cf::FileSys::InFileI* PngFile=cf::FileSys::FileMan->OpenRead(FileName);
        if (PngFile==NULL) throw LoadErrorT();

        const char    NUM_OF_SIGNATURE_BYTES=8;
        unsigned char SignatureBuffer[NUM_OF_SIGNATURE_BYTES];

        PngFile->Read((char*)SignatureBuffer, NUM_OF_SIGNATURE_BYTES);

        if (png_sig_cmp(SignatureBuffer, 0, NUM_OF_SIGNATURE_BYTES))
        {
            // printf("This is not a PNG file!\n");
            cf::FileSys::FileMan->Close(PngFile);
            throw LoadErrorT();
        }

        png_structp png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, MyCustomLibpngErrorHandler, NULL);

        if (!png_ptr)
        {
            cf::FileSys::FileMan->Close(PngFile);
            throw LoadErrorT();
        }

        png_infop info_ptr=png_create_info_struct(png_ptr);

        if (!info_ptr)
        {
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            cf::FileSys::FileMan->Close(PngFile);
            throw LoadErrorT();
        }

        try
        {
         // png_init_io(png_ptr, FilePtr);      // This is used when the file is read via a FILE* with fread().
            png_set_read_fn(png_ptr, PngFile, MyFileSysBasedPngReadFunction);
            png_set_sig_bytes(png_ptr, NUM_OF_SIGNATURE_BYTES);

            // Use the libpng low-level read interface.
            png_read_info(png_ptr, info_ptr);

            // Set up the input transformations such that we always obtain RGBA data.
            switch (png_get_color_type(png_ptr, info_ptr))
            {
                case PNG_COLOR_TYPE_PALETTE:
                    png_set_palette_to_rgb(png_ptr);
                    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
                    break;

                case PNG_COLOR_TYPE_GRAY:
                    if (png_get_bit_depth(png_ptr, info_ptr) < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
                    if (png_get_bit_depth(png_ptr, info_ptr)==16) png_set_strip_16(png_ptr);
                    png_set_gray_to_rgb(png_ptr);
                    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
                    break;

                case PNG_COLOR_TYPE_GRAY_ALPHA:
                    if (png_get_bit_depth(png_ptr, info_ptr) < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
                    if (png_get_bit_depth(png_ptr, info_ptr)==16) png_set_strip_16(png_ptr);
                    png_set_gray_to_rgb(png_ptr);
                    break;

                case PNG_COLOR_TYPE_RGB:
                    if (png_get_bit_depth(png_ptr, info_ptr) < 8) png_set_packing(png_ptr);
                    if (png_get_bit_depth(png_ptr, info_ptr)==16) png_set_strip_16(png_ptr);
                    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
                    break;
            }

            png_read_update_info(png_ptr, info_ptr);

            const unsigned int   Width    =png_get_image_width (png_ptr, info_ptr);
            const unsigned int   Height   =png_get_image_height(png_ptr, info_ptr);
         // const char           Channels =png_get_channels    (png_ptr, info_ptr);
         // const unsigned long  RowBytes =png_get_rowbytes    (png_ptr, info_ptr);
         // const unsigned char* Signature=png_get_signature   (png_ptr, info_ptr);

            if (4*Width!=png_get_rowbytes(png_ptr, info_ptr)) throw LoadErrorT();

            ArrayT<png_bytep> RowPointers;

            Data.PushBackEmpty(Width*Height);
            for (unsigned int y=0; y<Height; y++) RowPointers.PushBack((unsigned char*)&Data[y*Width]);

            png_read_image(png_ptr, &RowPointers[0]);

            SizeX=Width;
            SizeY=Height;

            png_read_end(png_ptr, info_ptr);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            cf::FileSys::FileMan->Close(PngFile);
        }
        catch (const LoadErrorT&)
        {
            fprintf(stderr, "    (A libpng error occurred for file: %s)\n", FileName);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            cf::FileSys::FileMan->Close(PngFile);
            throw LoadErrorT();
        }
    }
    else if (_stricmp(&FileName[FileNameLength-4], ".jpg")==0 || _stricmp(&FileName[FileNameLength-5], ".jpeg")==0)
    {
        // It is a jpg/jpeg file.
        cf::FileSys::InFileI* JpegFile=cf::FileSys::FileMan->OpenRead(FileName);
        if (JpegFile==NULL) throw LoadErrorT();

        jpeg_decompress_struct CompressInfo;
        jpeg_error_mgr         ErrorManager;

        // We set up the normal JPEG error routines, then override error_exit.
        CompressInfo.err=jpeg_std_error(&ErrorManager);
        ErrorManager.error_exit=MyCustomLibjpegErrorHandler;

        try
        {
            jpeg_create_decompress(&CompressInfo);
            jpeg_FileSys_src(&CompressInfo, JpegFile);      // This is not a function of the jpeg library, but my own.
            jpeg_read_header(&CompressInfo, true);
            jpeg_start_decompress(&CompressInfo);

            SizeX=CompressInfo.output_width;
            SizeY=CompressInfo.output_height;
            Data.PushBackEmpty(SizeX*SizeY);

            ArrayT<JSAMPLE> Scanline;
            Scanline.PushBackEmpty(3*SizeX);

            for (unsigned int y=0; y<SizeY; y++)
            {
                JSAMPROW ScanlinePtr=&Scanline[0];

                jpeg_read_scanlines(&CompressInfo, &ScanlinePtr, 1);

                for (unsigned int x=0; x<SizeX; x++)
                    SetPixel(x, y, Scanline[x*3+0], Scanline[x*3+1], Scanline[x*3+2], 255);     // With the 255 make sure that the alpha-channel is not left uninitialized (or else valgrind will complain)!
            }

            jpeg_finish_decompress(&CompressInfo);
            jpeg_destroy_decompress(&CompressInfo);
            cf::FileSys::FileMan->Close(JpegFile);
        }
        catch (const LoadErrorT&)
        {
            jpeg_destroy_decompress(&CompressInfo);
            cf::FileSys::FileMan->Close(JpegFile);
            throw LoadErrorT();
        }
    }
    else throw LoadErrorT();
}


BitmapT::BitmapT(unsigned int Width, unsigned int Height, const uint32_t* Buffer)
    : SizeX(Width),
      SizeY(Height)
{
    const unsigned long int SizeXY=SizeX*SizeY;

    Data.PushBackEmpty(SizeXY);

    if (Buffer!=NULL)
        for (unsigned long int PixelNr=0; PixelNr<SizeXY; PixelNr++) Data[PixelNr]=Buffer[PixelNr];
}


BitmapT BitmapT::GetBuiltInFileNotFoundBitmap()
{
    BitmapT FNF;

    FNF.SizeX=128;
    FNF.SizeY=16;

    for (unsigned int PixelNr=0; PixelNr<128*16; PixelNr++)
        FNF.Data.PushBack(FileNotFoundBitmapData[PixelNr] ? 0xFFFC5454 : 0xFFA80000);

    return FNF;
}


void BitmapT::GetPixel(unsigned int x, unsigned int y, int& r, int& g, int& b) const
{
    const unsigned long RGBA=Data[x+y*SizeX];

    r=(RGBA >>  0) & 0xFF;
    g=(RGBA >>  8) & 0xFF;
    b=(RGBA >> 16) & 0xFF;
}


void BitmapT::SetPixel(unsigned int x, unsigned int y, int r, int g, int b)
{
    if (r<0) r=0;
    if (g<0) g=0;
    if (b<0) b=0;

    if (r>255) r=255;
    if (g>255) g=255;
    if (b>255) b=255;

    const unsigned long r_=r;
    const unsigned long g_=g;
    const unsigned long b_=b;

    // Leave the old alpha value untouched.
    Data[x+y*SizeX]&=0xFF000000;
    Data[x+y*SizeX]|=(b_ << 16)+(g_ << 8)+r_;
}


void BitmapT::GetPixel(unsigned int x, unsigned int y, int& r, int& g, int& b, int& a) const
{
    const uint32_t RGBA=Data[x+y*SizeX];

    r=(RGBA >>  0) & 0xFF;
    g=(RGBA >>  8) & 0xFF;
    b=(RGBA >> 16) & 0xFF;
    a=(RGBA >> 24) & 0xFF;
}


void BitmapT::SetPixel(unsigned int x, unsigned int y, int r, int g, int b, int a)
{
    if (r<0) r=0;
    if (g<0) g=0;
    if (b<0) b=0;
    if (a<0) a=0;

    if (r>255) r=255;
    if (g>255) g=255;
    if (b>255) b=255;
    if (a>255) a=255;

    const uint32_t r_=r;
    const uint32_t g_=g;
    const uint32_t b_=b;
    const uint32_t a_=a;

    Data[x+y*SizeX]=(a_ << 24)+(b_ << 16)+(g_ << 8)+r_;
}


void BitmapT::GetPixel(unsigned int x, unsigned int y, float& r, float& g, float& b) const
{
    const uint32_t RGBA=Data[x+y*SizeX];

    r=((RGBA >>  0) & 0xFF)/255.0f;
    g=((RGBA >>  8) & 0xFF)/255.0f;
    b=((RGBA >> 16) & 0xFF)/255.0f;
}


void BitmapT::SetPixel(unsigned int x, unsigned int y, float r, float g, float b)
{
    if (r<0.0) r=0.0;
    if (g<0.0) g=0.0;
    if (b<0.0) b=0.0;

    if (r>1.0) r=1.0;
    if (g>1.0) g=1.0;
    if (b>1.0) b=1.0;

    const uint32_t r_=(uint32_t)(r*255.0);
    const uint32_t g_=(uint32_t)(g*255.0);
    const uint32_t b_=(uint32_t)(b*255.0);

    // Leave the old alpha value untouched.
    Data[x+y*SizeX]&=0xFF000000;
    Data[x+y*SizeX]|=(b_ << 16)+(g_ << 8)+r_;
}


void BitmapT::GetPixel(unsigned int x, unsigned int y, float& r, float& g, float& b, float& a) const
{
    const uint32_t RGBA=Data[x+y*SizeX];

    r=((RGBA >>  0) & 0xFF)/255.0f;
    g=((RGBA >>  8) & 0xFF)/255.0f;
    b=((RGBA >> 16) & 0xFF)/255.0f;
    a=((RGBA >> 24) & 0xFF)/255.0f;
}


void BitmapT::SetPixel(unsigned int x, unsigned int y, float r, float g, float b, float a)
{
    if (r<0.0) r=0.0;
    if (g<0.0) g=0.0;
    if (b<0.0) b=0.0;
    if (a<0.0) a=0.0;

    if (r>1.0) r=1.0;
    if (g>1.0) g=1.0;
    if (b>1.0) b=1.0;
    if (a>1.0) a=1.0;

    const uint32_t r_=(uint32_t)(r*255.0);
    const uint32_t g_=(uint32_t)(g*255.0);
    const uint32_t b_=(uint32_t)(b*255.0);
    const uint32_t a_=(uint32_t)(a*255.0);

    Data[x+y*SizeX]=(a_ << 24)+(b_ << 16)+(g_ << 8)+r_;
}


void BitmapT::ApplyGamma(float Gamma)
{
    char GammaLookup[256];

    for (unsigned int ValueNr=0; ValueNr<256; ValueNr++)
    {
        float Value=pow(float(ValueNr)/255.0f, 1.0f/Gamma)*255.0f;

        if (Value<  0.0f) Value=  0.0f;
        if (Value>255.0f) Value=255.0f;

        GammaLookup[ValueNr]=char(Value+0.49f);
    }

    for (unsigned long i=0; i<Data.Size(); i++)
    {
        char Alpha=char(Data[i] >> 24);
        char Blue =char(Data[i] >> 16);
        char Green=char(Data[i] >>  8);
        char Red  =char(Data[i] >>  0);

        const uint32_t NewAlpha=Alpha;
        const uint32_t NewBlue =GammaLookup[Blue ];
        const uint32_t NewGreen=GammaLookup[Green];
        const uint32_t NewRed  =GammaLookup[Red  ];

        Data[i]=(NewAlpha << 24)+(NewBlue << 16)+(NewGreen << 8)+NewRed;
    }
}


// This is my very own algorithm. Is this what they usually call "bilinear"?
// Please note that rounding errors in combination with floor() and ceil() can bomb the entire algorithm,
// and this is why I'm computing the floors and ceils with exact(!) integer arithmetic, which solves the problem entirely!
// The non-integer stuff is only needed for the weights, and I guess we could get rid even of those...!
void BitmapT::Scale(unsigned int NewSizeX, unsigned int NewSizeY)
{
    if (NewSizeX>=1 && NewSizeX!=SizeX)
    {
        // Scale in x-axis direction.
        BitmapT NewBitmap;

        NewBitmap.SizeX=NewSizeX;
        NewBitmap.SizeY=SizeY;
        NewBitmap.Data.PushBackEmpty(NewSizeX*SizeY);

        for (unsigned int nx=0; nx<NewSizeX; nx++)
        {
            // The nx-th column in the new bitmap spans in the old bitmap the columns from ox1 to ox2.
            const double       Ratio    =double(SizeX)/double(NewSizeX);
            const double       ox1      =double(nx  )*Ratio;
            const double       ox2      =double(nx+1)*Ratio;
            const unsigned int ox1_floor=  nx   *SizeX              /NewSizeX;     // =floor(ox1);
            const unsigned int ox2_ceil =((nx+1)*SizeX+(NewSizeX-1))/NewSizeX;     // =ceil (ox2);     // The "+(NewSizeX-1)" is to get the ceil!
            const double       ox1_frac =ox1-ox1_floor;
            const double       ox2_frac =ox2-(ox2_ceil-1);

            for (unsigned int oy=0; oy<SizeY; oy++)
            {
                double TotalR=0.0;
                double TotalG=0.0;
                double TotalB=0.0;
                double TotalA=0.0;
             // double TotalW=0.0;

                for (unsigned int ox=ox1_floor; ox<ox2_ceil; ox++)
                {
                    double Weight;

                    if (ox1_floor!=ox2_ceil-1)
                    {
                        // The "new" pixel covers at least two columns.
                             if (ox==ox1_floor ) Weight=(1.0-ox1_frac)/Ratio;   // The first column.
                        else if (ox==ox2_ceil-1) Weight=ox2_frac/Ratio;         // The last column.
                        else                     Weight=1.0/Ratio;              // One of the middle columns.
                    }
                    else Weight=1.0;    // =Ratio/Ratio;    // This is the only iteration of this loop, and the "new" pixel is entirely in the "old" pixel.

                    int r, g, b, a;
                    GetPixel(ox, oy, r, g, b, a);

                    TotalR+=r*Weight;
                    TotalG+=g*Weight;
                    TotalB+=b*Weight;
                    TotalA+=a*Weight;
                 // TotalW+=Weight;
                }

             // if (fabs(TotalW-1.0)>0.0000001) printf("WARNING: TotalW!=1.0: %.15f  %.10f %.10f %.10f %.10f  %lu %lu\n", TotalW, ox1, ox2, ox1_frac, ox2_frac, ox1_floor, ox2_ceil);
                NewBitmap.SetPixel(nx, oy, int(TotalR+0.49), int(TotalG+0.49), int(TotalB+0.49), int(TotalA+0.49));
            }
        }

        // Copy the scaled bitmap into this bitmap.
        SizeX=NewBitmap.SizeX;
        SizeY=NewBitmap.SizeY;
        Data =NewBitmap.Data;
    }

    if (NewSizeY>=1 && NewSizeY!=SizeY)
    {
        // Scale in y-axis direction.
        BitmapT NewBitmap;

        NewBitmap.SizeX=SizeX;
        NewBitmap.SizeY=NewSizeY;
        NewBitmap.Data.PushBackEmpty(SizeX*NewSizeY);

        for (unsigned int ny=0; ny<NewSizeY; ny++)
        {
            // The ny-th row in the new bitmap spans in the old bitmap the rows from oy1 to oy2.
            const double       Ratio    =double(SizeY)/double(NewSizeY);
            const double       oy1      =double(ny  )*Ratio;
            const double       oy2      =double(ny+1)*Ratio;
            const unsigned int oy1_floor=  ny   *SizeY              /NewSizeY;     // =floor(oy1);
            const unsigned int oy2_ceil =((ny+1)*SizeY+(NewSizeY-1))/NewSizeY;     // =ceil (oy2);     // The "+(NewSizeY-1)" is to get the ceil!
            const double       oy1_frac =oy1-oy1_floor;
            const double       oy2_frac =oy2-(oy2_ceil-1);

            for (unsigned int ox=0; ox<SizeX; ox++)
            {
                double TotalR=0.0;
                double TotalG=0.0;
                double TotalB=0.0;
                double TotalA=0.0;
             // double TotalW=0.0;

                for (unsigned int oy=oy1_floor; oy<oy2_ceil; oy++)
                {
                    double Weight;

                    if (oy1_floor!=oy2_ceil-1)
                    {
                        // The "new" pixel covers at least two rows.
                             if (oy==oy1_floor ) Weight=(1.0-oy1_frac)/Ratio;   // The first row.
                        else if (oy==oy2_ceil-1) Weight=oy2_frac/Ratio;         // The last row.
                        else                     Weight=1.0/Ratio;              // One of the middle rows.
                    }
                    else Weight=1.0;    // =Ratio/Ratio;    // This is the only iteration of this loop, and the "new" pixel is entirely in the "old" pixel.

                    int r, g, b, a;
                    GetPixel(ox, oy, r, g, b, a);

                    TotalR+=r*Weight;
                    TotalG+=g*Weight;
                    TotalB+=b*Weight;
                    TotalA+=a*Weight;
                 // TotalW+=Weight;
                }

             // if (fabs(TotalW-1.0)>0.0000001) printf("WARNING: TotalW!=1.0:  %.15f\n", TotalW);
                NewBitmap.SetPixel(ox, ny, int(TotalR+0.49), int(TotalG+0.49), int(TotalB+0.49), int(TotalA+0.49));
            }
        }

        // Copy the scaled bitmap into this bitmap.
        SizeX=NewBitmap.SizeX;
        SizeY=NewBitmap.SizeY;
        Data =NewBitmap.Data;
    }
}


// NeuQuant Neural-Net Quantization Algorithm
// ------------------------------------------
//
// Copyright (c) 1994 Anthony Dekker
//
// NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994.
// See "Kohonen neural networks for optimal colour quantization"
// in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367.
// for a discussion of the algorithm.
// See also  http://members.ozemail.com.au/~dekker/NEUQUANT.HTML
//
// Any party obtaining a copy of these files from the author, directly or
// indirectly, is granted, free of charge, a full and unrestricted irrevocable,
// world-wide, paid up, royalty-free, nonexclusive right and license to deal
// in this software and documentation files (the "Software"), including without
// limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons who receive
// copies from any such party to do so, with the only requirement being
// that this copyright notice remain intact.
char* BitmapT::GetPalettedImage() const
{
/*    const int    ncycles        =100;                  // no. of learning cycles

    const int    netsize        =256;                  // number of colours used
    const int    maxnetpos      =netsize-1;

    const int    initrad        =netsize/8;            // for 256 cols, radius starts at 32
    const int    radiusbiasshift=6;
    const int    radiusbias     =1 << radiusbiasshift;
    const int    initBiasRadius =initrad*radiusbias;
    const int    radiusdec      =30;                   // factor of 1/30 each cycle

    const int    alphabiasshift =10;                   // alpha starts at 1
    const int    initalpha      =1 << alphabiasshift;  // biased by 10 bits

    const double gamma          =1024.0;
    const double beta           =1.0/1024.0;
    const double betagamma      =beta*gamma;

    const int    prime1         =499;                   // four primes near 500 - assume that
    const int    prime2         =491;                   // no image has a length so large that
    const int    prime3         =487;                   // it is divisible by all four primes
    const int    prime4         =503;
    const int    maxprime       =prime4;

    double network [netsize][3];                        // the network itself
    int    colormap[netsize][4];                        // the network itself
    double bias    [netsize];                           // bias array for learning
    double freq    [netsize];                           // freq array for learning


    // Warum können zu kleine Bilder scheitern?
    // Vermutlich gar nicht, höchstens schlechtere Qualität!?
    // if (SizeX*SizeY<maxprime) throw new IOException("Image is too small");

    // Initialize.
    int i;
    for (i=0; i<netsize; i++)
    {
        network[i][0]=(256.0*i)/netsize;
        network[i][1]=(256.0*i)/netsize;
        network[i][2]=(256.0*i)/netsize;

        freq[i]=1.0/netsize;
        bias[i]=0.0;
    }

    // Learning phase.
    const int samplefac   =1;       // 1 bis 30, 1 ist höchste Qualität.
    const int alphadec    =30+((samplefac-1)/3);
    const int lengthcount =Data.Size();
    const int samplepixels=lengthcount/samplefac;
    const int delta       =samplepixels/ncycles;

    int biasRadius  =initBiasRadius;
    int alpha       =initalpha;
    int rad         =biasRadius >> radiusbiasshift; if (rad<=1) rad=0;
    int step        =0;
    int pos         =0;

         if ((lengthcount % prime1)!=0) step=prime1;
    else if ((lengthcount % prime2)!=0) step=prime2;
    else if ((lengthcount % prime3)!=0) step=prime3;
    else                                step=prime4;

    i=0;
    while (i<samplepixels)
    {
        const double r=double((Data[pos] >>  0) & 0xFF);
        const double g=double((Data[pos] >>  8) & 0xFF);
        const double b=double((Data[pos] >> 16) & 0xFF);
        const double a=double(alpha)/initalpha;

        int j;

        // Search for biased BGR values:
        // Finds closest neuron (min dist) and updates freq.
        // Finds best neuron (min dist-bias) and returns position.
        // For frequently chosen neurons, freq[i] is high and bias[i] is negative.
        // bias[i] = gamma*((1/netsize)-freq[i])
        {
            double bestd      =9999999999999.0;
            double bestbiasd  =9999999999999.0;
            int    bestpos    =-1;
            int    bestbiaspos=-1;

            for (int k=0; k<netsize; k++)
            {
                double dist=abs(network[k][0]-b)+abs(network[k][1]-g)+abs(network[k][2]-r);

                if (dist<bestd)
                {
                    bestd  =dist;
                    bestpos=k;
                }

                double biasdist=dist-bias[k];

                if (biasdist<bestbiasd)
                {
                    bestbiasd  =biasdist;
                    bestbiaspos=k;
                }

                freq[k]-=beta     *freq[k];
                bias[k]+=betagamma*freq[k];
            }

            freq[bestpos]+=beta;
            bias[bestpos]-=betagamma;

            j=bestbiaspos;
        }

        // Alter hit neuron: Move neuron j towards biased (b,g,r) by factor alpha.
        network[j][0]-=a*(network[j][0]-b);
        network[j][1]-=a*(network[j][1]-g);
        network[j][2]-=a*(network[j][2]-r);

        if (rad>0)
        {
            // Alter neighbours.
            int lo=j-rad; if (lo<     -1) lo=-1;
            int hi=j+rad; if (hi>netsize) hi=netsize;

            int k=j+1;
            int l=j-1;
            int q=0;

            while (k<hi || l>lo)
            {
                const double a2=(a*(rad*rad-q*q))/(rad*rad);
                q++;

                if (k<hi)
                {
                    network[k][0]-=a2*(network[k][0]-b);
                    network[k][1]-=a2*(network[k][1]-g);
                    network[k][2]-=a2*(network[k][2]-r);
                    k++;
                }

                if (l>lo)
                {
                    network[l][0]-=a2*(network[l][0]-b);
                    network[l][1]-=a2*(network[l][1]-g);
                    network[l][2]-=a2*(network[l][2]-r);
                    l--;
                }
            }
        }

        pos+=step;
        while (pos>=lengthcount) pos-=lengthcount;

        i++;
        if ((i % delta)==0)
        {
            alpha-=alpha/alphadec;
            biasRadius-=biasRadius/radiusdec;
            rad=biasRadius >> radiusbiasshift;
            if (rad<=1) rad=0;
        }
    }

    // Build colormap.
    for (i=0; i<netsize; i++)
    {
        for (int j=0; j<3; j++)
        {
            int x=int(network[i][j]+0.5);

            if (x<  0) x=  0;
            if (x>255) x=255;

            colormap[i][j]=x;
        }

        colormap[i][3]=i;
    }

    // Output colour map.
    char* PaletteImage=new char[256*3 + SizeX*SizeY];   // Space for the resulting output image.

    for (i=2; i>=0; i--)
        for (int j=0; j<netsize; j++)
            PaletteImage[j*3+i]=colormap[j][2-i];

    // Search for BGR values 0..255 (after net is unbiased) and return colour index.
    for (unsigned long PixelNr=0; PixelNr<Data.Size(); PixelNr++)
    {
        int r=(Data[PixelNr] >>  0) & 0xFF;
        int g=(Data[PixelNr] >>  8) & 0xFF;
        int b=(Data[PixelNr] >> 16) & 0xFF;

        // Search for BGR values 0..255 and return colour index.
        int bestd=1000;     // Biggest possible dist is 256*3.
        int best =  -1;

        for (i=0; i<netsize; i++)
        {
            int dist=abs(colormap[i][0]-b)+abs(colormap[i][1]-g)+abs(colormap[i][2]-r);

            if (dist<bestd)
            {
                bestd=dist;
                best =i;
            }
        }

        PaletteImage[3*256+PixelNr]=best;
    }

    return PaletteImage; */


    char* PaletteImage       =new char[256*3 + SizeX*SizeY];   // Space for the resulting output image.

    const int netsize        =256;                             // Number of colours used.
    const int maxnetpos      =netsize-1;
    const int netbiasshift   =4;                               // Bias for colour values.
    const int lengthcount    =3*SizeX*SizeY;
    const int samplefac      =1;                               // Range 1..30.

    // Defs for freq and bias.
    const int intbiasshift   =16;                              // Bias for fractions.
    const int intbias        =((int)1) << intbiasshift;
    const int gammashift     =10;                              // gamma=1024
 // const int gamma          =((int)1)<<gammashift;
    const int betashift      =10;
    const int beta           =intbias >> betashift;            // beta=1/1024
    const int betagamma      =intbias << (gammashift-betashift);

    // Defs for decreasing radius factor.
    const int initrad        =netsize >> 3;                    // For 256 colours, radius starts
    const int radiusbiasshift=6;                               // at 32.0 biased by 6 bits
    const int radiusbias     =((int)1) << radiusbiasshift;
    const int initradius     =initrad*radiusbias;              // and decreases by a
    const int radiusdec      =30;                              // factor of 1/30 each cycle.

    // Defs for decreasing alpha factor.
    const int alphabiasshift =10;                              // Alpha starts at 1.0.
    const int initalpha      =((int)1) << alphabiasshift;

    // radbias and alpharadbias used for radpower calculation.
    const int radbiasshift   =8;
    const int radbias        =((int)1)<<radbiasshift;
    const int alpharadbshift =alphabiasshift+radbiasshift;
    const int alpharadbias   =((int)1)<<alpharadbshift;

    int network [netsize][4];   // The network itself, each entry has components BGRc.
    int netindex[256];          // For network lookup - really 256.
    int bias    [netsize];      // Bias and freq arrays for learning.
    int freq    [netsize];
    int radpower[initrad];      // radpower for precomputation.


    // Initialise network in range (0,0,0) to (255,255,255) and set parameters.
    {
        for (int i=0; i<netsize; i++)
        {
            network[i][0]=(i << (netbiasshift+8))/netsize;
            network[i][1]=(i << (netbiasshift+8))/netsize;
            network[i][2]=(i << (netbiasshift+8))/netsize;

            freq[i]=intbias/netsize;        // 1/netsize
            bias[i]=0;
        }
    }

    // Main Learning Loop.
    {
        const int     alphadec    =30 + ((samplefac-1)/3);              // Biased by 10 bits.
        unsigned long PixelNr     =0;
        int           samplepixels=lengthcount/(3*samplefac);
        int           delta       =samplepixels/100;                    // 100 is the number of learning cycles.
        int           alpha       =initalpha;
        int           radius      =initradius;
        int           rad         =radius >> radiusbiasshift;
        const int     step        =(lengthcount % 499)!=0 ? 499 :       // Four primes near 500 - assume no
                                   (lengthcount % 491)!=0 ? 491 :       // image has a length so large that
                                   (lengthcount % 487)!=0 ? 487 : 503;  // it is divisible by all four primes.

        if (rad<=1) rad=0;
        { for (int i=0; i<rad; i++) radpower[i]=alpha*(((rad*rad-i*i)*radbias)/(rad*rad)); }

        for (int i=0; i<samplepixels;)
        {
            int b=((Data[PixelNr] >> 16) & 0xFF) << netbiasshift;
            int g=((Data[PixelNr] >>  8) & 0xFF) << netbiasshift;
            int r=((Data[PixelNr] >>  0) & 0xFF) << netbiasshift;

            // Search for biased BGR values:
            // Finds closest neuron (min dist) and updates freq.
            // Finds best neuron (min dist-bias) and returns position.
            // For frequently chosen neurons, freq[i] is high and bias[i] is negative.
            // bias[i] = gamma*((1/netsize)-freq[i]).
            int  bestd      =~(((int)1) << 31);
            int  bestbiasd  =bestd;
            int  bestpos    =-1;
            int  bestbiaspos=bestpos;

            {
                int* p          =bias;
                int* f          =freq;

                for (int netnum=0; netnum<netsize; netnum++)
                {
                    int* n       =network[netnum];
                    int  dist    =abs(n[0]-b) + abs(n[1]-g) + abs(n[2]-r);
                    int  biasdist=dist-((*p) >> (intbiasshift-netbiasshift));
                    int  betafreq=*f >> betashift;

                    if (    dist<bestd    ) { bestd    =dist;     bestpos    =netnum; }
                    if (biasdist<bestbiasd) { bestbiasd=biasdist; bestbiaspos=netnum; }

                    *f++ -= betafreq;
                    *p++ += (betafreq<<gammashift);
                }
            }

            freq[bestpos]+=beta;
            bias[bestpos]-=betagamma;

            // Alter hit neuron: Move neuron j towards biased (b,g,r) by factor alpha.
            int* n=network[bestbiaspos];

            *n-=(alpha*(*n-b))/initalpha; n++;
            *n-=(alpha*(*n-g))/initalpha; n++;
            *n-=(alpha*(*n-r))/initalpha;

            // Alter neighbours: Move adjacent neurons by precomputed
            // alpha*(1-((bestbiaspos-j)^2/[r]^2)) in radpower[|bestbiaspos-j|].
            if (rad)
            {
                int  lo=bestbiaspos-rad; if (lo<-1     ) lo=-1;
                int  hi=bestbiaspos+rad; if (hi>netsize) hi=netsize;
                int  j =bestbiaspos+1;
                int  k =bestbiaspos-1;
                int* q =radpower;

                while ((j<hi) || (k>lo))
                {
                    int a=(*(++q));

                    if (j<hi)
                    {
                        int* p=network[j];

                        *p-=(a*(*p-b))/alpharadbias; p++;
                        *p-=(a*(*p-g))/alpharadbias; p++;
                        *p-=(a*(*p-r))/alpharadbias; j++;
                    }

                    if (k>lo)
                    {
                        int* p=network[k];

                        *p-=(a*(*p-b))/alpharadbias; p++;
                        *p-=(a*(*p-g))/alpharadbias; p++;
                        *p-=(a*(*p-r))/alpharadbias; k--;
                    }
                }
            }

            PixelNr+=step;
            while (PixelNr>=SizeX*SizeY) PixelNr-=SizeX*SizeY;

            i++;
            if ((i % delta)==0)
            {
                alpha -= alpha/alphadec;
                radius-=radius/radiusdec;

                rad=radius >> radiusbiasshift;
                if (rad<=1) rad=0;
                for (int j=0; j<rad; j++) radpower[j]=alpha*(((rad*rad-j*j)*radbias)/(rad*rad));
            }
        }
    }

    // Unbias network to give byte values 0..255 and record position i to prepare for sort.
    {
        for (int i=0; i<netsize; i++)
        {
            for (int j=0; j<3; j++)
            {
                // OLD CODE: network[i][j] >>= netbiasshift;
                // Fix based on bug report by Juergen Weigert.
                int temp=(network[i][j] + (1 << (netbiasshift-1))) >> netbiasshift;

                if (temp>255) temp=255;
                network[i][j]=temp;
            }

            // Record colour number.
            network[i][3]=i;
        }
    }

    // Output colour map.
    {
        for (int i=2; i>=0; i--)
            for (int j=0; j<netsize; j++)
                PaletteImage[j*3+i]=network[j][2-i];
    }

    // Insertion sort of network and building of netindex[0..255] (to do after unbias).
    {
        int previouscol=0;
        int startpos   =0;

        for (int i=0; i<netsize; i++)
        {
            int* p       =network[i];
            int  smallpos=i;
            int  smallval=p[1];                // index on g

            // Find smallest in i..netsize-1.
            for (int j=i+1; j<netsize; j++)
                if (network[j][1] < smallval)    // index on g
                {
                    smallpos=j;
                    smallval=network[j][1];      // index on g
                }

            // Swap p (i) and q (smallpos) entries.
            if (i!=smallpos)
            {
                int* q=network[smallpos];
                int  j;

                j=q[0]; q[0]=p[0]; p[0]=j;
                j=q[1]; q[1]=p[1]; p[1]=j;
                j=q[2]; q[2]=p[2]; p[2]=j;
                j=q[3]; q[3]=p[3]; p[3]=j;
            }

            // smallval entry is now in position i.
            if (smallval!=previouscol)
            {
                netindex[previouscol]=(startpos+i) >> 1;
                for (int j=previouscol+1; j<smallval; j++) netindex[j]=i;
                previouscol=smallval;
                startpos=i;
            }
        }

        netindex[previouscol]=(startpos+maxnetpos) >> 1;
        for (int j=previouscol+1; j<256; j++) netindex[j]=maxnetpos;
    }

    // Search for BGR values 0..255 (after net is unbiased) and return colour index.
    for (unsigned long PixelNr=0; PixelNr<Data.Size(); PixelNr++)
    {
        int b=(Data[PixelNr] >> 16) & 0xFF;
        int g=(Data[PixelNr] >>  8) & 0xFF;
        int r=(Data[PixelNr] >>  0) & 0xFF;

        int bestd=1000;                       // Biggest possible dist is 256*3.
        int best =-1;
        int i    =netindex[g];                // Index on g.
        int j    =i-1;                        // Start at netindex[g] and work outwards.

        while ((i<netsize) || (j>=0))
        {
            if (i<netsize)
            {
                int* p   =network[i];
                int  dist=p[1]-g;             // inx key

                if (dist>=bestd) i=netsize;   // stop iter
                else
                {
                    i++;
                    if (dist<0) dist=-dist;
                    dist+=abs(p[0]-b);
                    if (dist<bestd)
                    {
                        dist+=abs(p[2]-r);
                        if (dist<bestd) { bestd=dist; best=p[3]; }
                    }
                }
            }

            if (j>=0)
            {
                int* p   =network[j];
                int  dist=g-p[1];             // inx key - reverse dif

                if (dist>=bestd) j=-1;        // stop iter
                else
                {
                    j--;
                    if (dist<0) dist=-dist;
                    dist+=abs(p[0]-b);
                    if (dist<bestd)
                    {
                        dist+=abs(p[2]-r);
                        if (dist<bestd) { bestd=dist; best=p[3]; }
                    }
                }
            }
        }

        PaletteImage[3*256+PixelNr]=best;
    }

    return PaletteImage;
}


bool BitmapT::SaveToDisk(const char* FileName) const
{
    if (FileName==NULL) return false;

    const size_t FileNameLength=strlen(FileName);

    // File names must at least be five characters long, as in "x.bmp".
    if (FileNameLength<5) return false;

    if (_stricmp(&FileName[FileNameLength-4], ".bmp")==0)
    {
        // It is a BMP (Windows Bitmap) file.
        FILE* FilePtr=fopen(FileName, "wb");
        if (FilePtr==NULL) return false;

        uint16_t Data2;
        uint32_t Data4;

        // Why is fwrite() placed into the dummy if statement?
        // Because glibc/GCC *force* us to check the return value, with no other way to turn off the warning.
        Data2=           19778; if (fwrite(&Data2, sizeof(Data2), 1, FilePtr)==0) { }  // FileType, 19778=0x4D42="BM"
        Data4=SizeX*SizeY*3+54; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // TotalSize, HeaderSize(==54)+PaletteSize+ImageSize
        Data2=               0; if (fwrite(&Data2, sizeof(Data2), 1, FilePtr)==0) { }  // Reserved
        Data2=               0; if (fwrite(&Data2, sizeof(Data2), 1, FilePtr)==0) { }  // Reserved
        Data4=              54; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // ImageOffset, HeaderSize+PaletteSize
        Data4=              40; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // Head1Size, Header-Bytes to come
        Data4=           SizeX; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // Width
        Data4=           SizeY; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // Height
        Data2=               1; if (fwrite(&Data2, sizeof(Data2), 1, FilePtr)==0) { }  // Planes
        Data2=              24; if (fwrite(&Data2, sizeof(Data2), 1, FilePtr)==0) { }  // BPP
        Data4=               0; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // Compression
        Data4=   SizeX*SizeY*3; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // SizeImage, Width*Height*Bytes per Pixel
        Data4=               0; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // XPelsPerMeter
        Data4=               0; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // YPelsPerMeter
        Data4=               0; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // ClrUsed
        Data4=               0; if (fwrite(&Data4, sizeof(Data4), 1, FilePtr)==0) { }  // ClrImportant

        for (unsigned int y=0; y<SizeY; y++)
        {
            for (unsigned int x=0; x<SizeX; x++)
            {
                // Beachte, daß Windows Bitmap Files kopfüber gespeichert werden!
                uint32_t RGBA=Data[(SizeY-y-1)*SizeX+x];

                char r=char(RGBA >>  0);
                char g=char(RGBA >>  8);
                char b=char(RGBA >> 16);

                if (fwrite(&b, sizeof(b), 1, FilePtr)==0) { }  // Blue
                if (fwrite(&g, sizeof(g), 1, FilePtr)==0) { }  // Green
                if (fwrite(&r, sizeof(r), 1, FilePtr)==0) { }  // Red
            }

            // Add padding bytes so that the next row starts at a multiple of four.
            if (fwrite(&Data4, 1, SizeX % 4, FilePtr)==0) { }
        }

        fclose(FilePtr);
    }
    else if (_stricmp(&FileName[FileNameLength-4], ".png")==0)
    {
        // It is a PNG (Portable Network Graphics) file.
        FILE* FilePtr=fopen(FileName, "wb");
        if (FilePtr==NULL) return false;

        png_structp png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, MyCustomLibpngErrorHandler, NULL);

        if (!png_ptr)
        {
            fclose(FilePtr);
            return false;
        }

        png_infop info_ptr=png_create_info_struct(png_ptr);

        if (!info_ptr)
        {
            png_destroy_write_struct(&png_ptr, NULL);
            fclose(FilePtr);
            return false;
        }

        try
        {
            ArrayT<png_bytep> RowPointers;
            for (unsigned int y=0; y<SizeY; y++) RowPointers.PushBack((unsigned char*)&Data[y*SizeX]);

            png_init_io(png_ptr, FilePtr);
            png_set_compression_level(png_ptr, 9);
            png_set_IHDR(png_ptr, info_ptr, SizeX, SizeY, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_write_info(png_ptr, info_ptr);
            png_write_image(png_ptr, &RowPointers[0]);
            png_write_end(png_ptr, info_ptr);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(FilePtr);
        }
        catch (const LoadErrorT&)
        {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(FilePtr);
            return false;
        }
    }
    else if (_stricmp(&FileName[FileNameLength-4], ".jpg")==0 || _stricmp(&FileName[FileNameLength-5], ".jpeg")==0)
    {
        // It is a jpg/jpeg file.
        FILE* FilePtr=fopen(FileName, "wb");
        if (FilePtr==NULL) return false;

        jpeg_compress_struct CompressInfo;
        jpeg_error_mgr       ErrorManager;

        // We set up the normal JPEG error routines, then override error_exit.
        CompressInfo.err=jpeg_std_error(&ErrorManager);
        ErrorManager.error_exit=MyCustomLibjpegErrorHandler;

        try
        {
            jpeg_create_compress(&CompressInfo);
            jpeg_stdio_dest(&CompressInfo, FilePtr);

            CompressInfo.image_width     =SizeX;
            CompressInfo.image_height    =SizeY;
            CompressInfo.input_components=3;        // # of color components/channels per pixel.
            CompressInfo.in_color_space  =JCS_RGB;  // Colorspace of input image.

            jpeg_set_defaults(&CompressInfo);
            jpeg_set_quality (&CompressInfo, 95, true);
            jpeg_start_compress (&CompressInfo, true);

            ArrayT<JSAMPLE> Scanline;
            for (unsigned int y=0; y<SizeY; y++)
            {
                // Move all RGBA quadruplets of this scanline to RGB triples.
                Scanline.Overwrite();

                for (unsigned int x=0; x<SizeX; x++)
                {
                    const uint32_t RGBA=Data[y*SizeX+x];

                    Scanline.PushBack(JSAMPLE(RGBA >>  0));
                    Scanline.PushBack(JSAMPLE(RGBA >>  8));
                    Scanline.PushBack(JSAMPLE(RGBA >> 16));
                }

                JSAMPROW ScanlinePtr=&Scanline[0];
                jpeg_write_scanlines(&CompressInfo, &ScanlinePtr, 1);
            }

            jpeg_finish_compress(&CompressInfo);
            jpeg_destroy_compress(&CompressInfo);
            fclose(FilePtr);
        }
        catch (const LoadErrorT&)
        {
            jpeg_destroy_compress(&CompressInfo);
            fclose(FilePtr);
            return false;
        }
    }
    else return false;

    return true;
}
