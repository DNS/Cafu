/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MapComposition.hpp"
#include "Bitmap/Bitmap.hpp"
#include "ConsoleCommands/Console.hpp"

#include <math.h>


ArrayT<std::string*> MapCompositionT::BaseDirCache;


static std::string* GetBaseDirCachePtr(ArrayT<std::string*>& BaseDirCache, const std::string& BaseDir)
{
    // If BaseDir is already in the cache, return its pointer.
    for (unsigned long Nr=0; Nr<BaseDirCache.Size(); Nr++)
        if (*BaseDirCache[Nr]==BaseDir)
            return BaseDirCache[Nr];

    // Otherwise add BaseDir to the cache.
    BaseDirCache.PushBack(new std::string(BaseDir));
    return BaseDirCache[BaseDirCache.Size()-1];
}


MapCompositionT::MapCompositionT(MinMagFiltersT MinFilter_, MinMagFiltersT MagFilter_, WrapModesT WrapS_, WrapModesT WrapT_, bool NoScaleDown_, bool NoCompression_)
    : Type         (Empty),
      MinFilter    (MinFilter_),
      MagFilter    (MagFilter_),
      WrapS        (WrapS_),
      WrapT        (WrapT_),
      NoScaleDown  (NoScaleDown_),
      NoCompression(NoCompression_),
      FileName     (""),
      BaseDir      (NULL),
      HeightScale  (1.0),
      Child1       (NULL),
      Child2       (NULL)
{
}


MapCompositionT::MapCompositionT(const std::string& s, const std::string& BaseDir_) /*throw (TextParserT::ParseError)*/
    : Type         (Empty),
      MinFilter    (Linear_MipMap_Linear),
      MagFilter    (Linear),
      WrapS        (Repeat),
      WrapT        (Repeat),
      NoScaleDown  (false),
      NoCompression(false),
      FileName     (""),
      BaseDir      (GetBaseDirCachePtr(BaseDirCache, BaseDir_)),
      HeightScale  (1.0),
      Child1       (NULL),
      Child2       (NULL)
{
    TextParserT TP(s.c_str(), "({[]}),", false);

    Init(TP, 0);
}


MapCompositionT::MapCompositionT(TextParserT& TP, const std::string& BaseDir_, bool NoCompression_, const unsigned long RecursionCount) /*throw (TextParserT::ParseError)*/
    : Type         (Empty),
      MinFilter    (Linear_MipMap_Linear),
      MagFilter    (Linear),
      WrapS        (Repeat),
      WrapT        (Repeat),
      NoScaleDown  (false),
      NoCompression(NoCompression_),
      FileName     (""),
      BaseDir      (GetBaseDirCachePtr(BaseDirCache, BaseDir_)),
      HeightScale  (1.0),
      Child1       (NULL),
      Child2       (NULL)
{
    Init(TP, RecursionCount);
}


void MapCompositionT::Init(TextParserT& TP, const unsigned long RecursionCount)
{
    if (TP.IsAtEOF()) return;
    std::string Token=TP.GetNextToken();

    if (Token=="add")
    {
        Type=Add;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        Child2=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="mul")
    {
        Type=Mul;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        Child2=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="combineNMs")
    {
        Type=CombineNormals;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        Child2=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="hm2nm")
    {
        Type=HeightMapToNormalMap;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        HeightScale=float(atof(TP.GetNextToken().c_str()));
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="flipNMyAxis")
    {
        Type=FlipNormalMapYAxis;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="renormalize")
    {
        Type=ReNormalize;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="blue2alpha")
    {
        Type=BlueToAlpha;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new MapCompositionT(TP, *BaseDir, NoCompression, RecursionCount+1);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else
    {
        if (Token!="")
        {
            Type=Map;
            FileName=Token;

            // Note that the following makes no sense - using "$lightmap" implies that the *code* will supply a texture for the MapComposition,
            // (including filter and wrapping specs); the MapCompositionT is never "instantiated" for deriving a texture from it,
            // as is normally the case.
            // This even makes sense: The LightMaps are created without knowing into which materials they will eventually be combined...
            // // For MapCompositionTs that only consist of "$lightmap", reset the default minification filter to linear (not mip-mapped).
            // // This saves us from writing "$lightmap, minFilter bilinear" for each occurrance of "$lightmap" in the material scripts.
            // if (FileName=="$lightmap" && RecursionCount==0) MinFilter=Linear;
        }
    }


    // Read options.
    while (!TP.IsAtEOF())
    {
        // Only read options for the root MapCompositionT, never for the children.
        if (RecursionCount!=0) break;

        Token=TP.GetNextToken();

        if (Token!=",")
        {
            // Tried to get a comma but got something else, so put it back and be done.
            TP.PutBack(Token);
            break;
        }

        Token=TP.GetNextToken();

        if (Token=="minFilter")
        {
            Token=TP.GetNextToken();

                 if (Token=="nearest"                || Token=="point"    ) MinFilter=Nearest;
            else if (Token=="linear"                 || Token=="bilinear" ) MinFilter=Linear;
            else if (Token=="nearest_mipmap_nearest"                      ) MinFilter=Nearest_MipMap_Nearest;
            else if (Token=="nearest_mipmap_linear"                       ) MinFilter=Nearest_MipMap_Linear;
            else if (Token=="linear_mipmap_nearest"                       ) MinFilter=Linear_MipMap_Nearest;
            else if (Token=="linear_mipmap_linear"   || Token=="trilinear") MinFilter=Linear_MipMap_Linear;
            else throw TextParserT::ParseError();
        }
        else if (Token=="magFilter")
        {
            Token=TP.GetNextToken();

                 if (Token=="nearest" || Token=="point"   ) MagFilter=Nearest;
            else if (Token=="linear"  || Token=="bilinear") MagFilter=Linear;
            else throw TextParserT::ParseError();
        }
        else if (Token=="wrapS")
        {
            Token=TP.GetNextToken();

                 if (Token=="repeat"     ) WrapS=Repeat;
            else if (Token=="clamp"      ) WrapS=Clamp;
            else if (Token=="clampToEdge") WrapS=ClampToEdge;
            else throw TextParserT::ParseError();
        }
        else if (Token=="wrapT")
        {
            Token=TP.GetNextToken();

                 if (Token=="repeat"     ) WrapT=Repeat;
            else if (Token=="clamp"      ) WrapT=Clamp;
            else if (Token=="clampToEdge") WrapT=ClampToEdge;
            else throw TextParserT::ParseError();
        }
        else if (Token=="noScaleDown")
        {
            NoScaleDown=true;
        }
        else if (Token=="noCompression")
        {
            NoCompression=true;
        }
        else if (Token=="useCompression")
        {
            // Normal-maps default to employ no compression,
            // so this is a means to override the default and turn compression on again.
            NoCompression=false;
        }
        else throw TextParserT::ParseError();
    }
}


// Copy Constructor (Law of the Big Three).
MapCompositionT::MapCompositionT(const MapCompositionT& Source)
    : Type         (Source.Type),
      MinFilter    (Source.MinFilter),
      MagFilter    (Source.MagFilter),
      WrapS        (Source.WrapS),
      WrapT        (Source.WrapT),
      NoScaleDown  (Source.NoScaleDown),
      NoCompression(Source.NoCompression),
      FileName     (Source.FileName),
      BaseDir      (Source.BaseDir),
      HeightScale  (Source.HeightScale),
      Child1       (Source.Child1 ? new MapCompositionT(*Source.Child1) : NULL),
      Child2       (Source.Child2 ? new MapCompositionT(*Source.Child2) : NULL)
{
}


// Destructor (Law of the Big Three).
MapCompositionT::~MapCompositionT()
{
    delete Child1;
    delete Child2;
}


// Assignment Operator (Law of the Big Three).
MapCompositionT& MapCompositionT::operator = (const MapCompositionT& Source)
{
    // Make sure that self-assignment is handled properly.
    if (this==&Source) return *this;

    Type         =Source.Type;
    MinFilter    =Source.MinFilter;
    MagFilter    =Source.MagFilter;
    WrapS        =Source.WrapS;
    WrapT        =Source.WrapT;
    NoScaleDown  =Source.NoScaleDown;
    NoCompression=Source.NoCompression;
    FileName     =Source.FileName;
    BaseDir      =Source.BaseDir;
    HeightScale  =Source.HeightScale;

    // This is not the most clever way of doing things, but self-assignment is
    // explicitly checked for above, and a failing new operator is fatal anyway.
    delete Child1; Child1=Source.Child1 ? new MapCompositionT(*Source.Child1) : NULL;
    delete Child2; Child2=Source.Child2 ? new MapCompositionT(*Source.Child2) : NULL;

    return *this;
}


// Equal Operator.
bool MapCompositionT::operator == (const MapCompositionT& rhs) const
{
    if (Type         !=rhs.Type         ) return false;
    if (MinFilter    !=rhs.MinFilter    ) return false;
    if (MagFilter    !=rhs.MagFilter    ) return false;
    if (WrapS        !=rhs.WrapS        ) return false;
    if (WrapT        !=rhs.WrapT        ) return false;
    if (NoScaleDown  !=rhs.NoScaleDown  ) return false;
    if (NoCompression!=rhs.NoCompression) return false;

    switch (Type)
    {
        case Empty:
            return true;

        case Map:
            return FileName==rhs.FileName && BaseDir==rhs.BaseDir;

        case Add:
        case Mul:
        case CombineNormals:
            // With "+" or f() being an arbitrary commutative operation (like +/Add, */Mul, CombineNormals):
            // a+b==c+d  or  f(a,b)==f(c,d)   <==>   (c==a && d==b) || (c==b && d==a)
            return ((*Child1)==(*rhs.Child1) && (*Child2)==(*rhs.Child2)) ||
                   ((*Child2)==(*rhs.Child1) && (*Child1)==(*rhs.Child2));

        case HeightMapToNormalMap:
            return (*Child1)==(*rhs.Child1) && rhs.HeightScale==HeightScale;

        case FlipNormalMapYAxis:
        case ReNormalize:
        case BlueToAlpha:
            return (*Child1)==(*rhs.Child1);
    }

    return false;
}


// This function loads all image source files from disk (at BaseDir+FileName), and combines them (according to this MapCompositionT)
// into a single resulting BitmapT. On any error with the participating bitmaps (ie. file not found, file unreadable, ...),
// a default texture is substituted for the missing participant, and a warning is printed out. Thus, the function never fails.
BitmapT* MapCompositionT::GetBitmap() const
{
    switch (Type)
    {
        case Empty:
            // Handled below as the default (file-not-found) case.
            break;

        case Map:
        {
            try
            {
                return new BitmapT((*BaseDir+FileName).c_str());
            }
            catch (const BitmapT::LoadErrorT&)
            {
                Console->Warning(std::string("Could not load the bitmap at \"")+(*BaseDir)+FileName+"\".\n");
                return new BitmapT(BitmapT::GetBuiltInFileNotFoundBitmap());
            }
        }

        case Add:
        {
            BitmapT* B1=Child1->GetBitmap();
            BitmapT* B2=Child2->GetBitmap();

            // Make sure that B2 has the same size as B1.
            B2->Scale(B1->SizeX, B1->SizeY);

            // Add the two bitmaps.
            for (unsigned int y=0; y<B1->SizeY; y++)
                for (unsigned int x=0; x<B1->SizeX; x++)
                {
                    int r1, g1, b1, a1;
                    int r2, g2, b2, a2;

                    B1->GetPixel(x, y, r1, g1, b1, a1);
                    B2->GetPixel(x, y, r2, g2, b2, a2);

                    B1->SetPixel(x, y, r1+r2, g1+g2, b1+b2, a1+a2);
                }

            delete B2;
            return B1;
        }

        case Mul:
        {
            BitmapT* B1=Child1->GetBitmap();
            BitmapT* B2=Child2->GetBitmap();

            // Make sure that B2 has the same size as B1.
            B2->Scale(B1->SizeX, B1->SizeY);

            // Add the two bitmaps.
            for (unsigned int y=0; y<B1->SizeY; y++)
                for (unsigned int x=0; x<B1->SizeX; x++)
                {
                    float r1, g1, b1, a1;
                    float r2, g2, b2, a2;

                    B1->GetPixel(x, y, r1, g1, b1, a1);
                    B2->GetPixel(x, y, r2, g2, b2, a2);

                    B1->SetPixel(x, y, r1*r2, g1*g2, b1*b2, a1*a2);
                }

            delete B2;
            return B1;
        }

        case CombineNormals:
        {
            BitmapT* NormalMap1=Child1->GetBitmap();
            BitmapT* NormalMap2=Child2->GetBitmap();

            // Make sure that NormalMap2 has the same size as NormalMap1.
            NormalMap2->Scale(NormalMap1->SizeX, NormalMap1->SizeY);

            // Initially create the combined normal-map from NormalMap1, in order to get the dimensions right.
            // The actual contents is overwritten below.
            BitmapT* CombinedNormalMap=new BitmapT(*NormalMap1);

            // Combine the two normal maps.
            for (unsigned int y=0; y<NormalMap1->SizeY; y++)
                for (unsigned int x=0; x<NormalMap1->SizeX; x++)
                {
                    float x1, y1, z1;
                    float x2, y2, z2;

                    NormalMap1->GetPixel(x, y, x1, y1, z1);
                    NormalMap2->GetPixel(x, y, x2, y2, z2);

                    x1=x1*2.0f-1.0f;
                    y1=y1*2.0f-1.0f;
                    z1=z1*2.0f-1.0f;

                    x2=x2*2.0f-1.0f;
                    y2=y2*2.0f-1.0f;
                    z2=z2*2.0f-1.0f;

                    // See the Cafu Tech Archive ("Adding two normal-maps properly", 2004-06-19) for a detailed explanation!
                    float x_=x1*z2+x2*z1;
                    float y_=y1*z2+y2*z1;
                    float z_=z1*z2;

                    // Renormalize.
                    const float Length=float(sqrt(x_*x_ + y_*y_ + z_*z_));

                    if (Length!=0.0)
                    {
                        x_/=Length;
                        y_/=Length;
                        z_/=Length;
                    }

                    CombinedNormalMap->SetPixel(x, y, (x_+1.0f)/2.0f, (y_+1.0f)/2.0f, (z_+1.0f)/2.0f);
                }

            delete NormalMap1;
            delete NormalMap2;

            return CombinedNormalMap;
        }

        case HeightMapToNormalMap:
        {
            BitmapT* HeightMap=Child1->GetBitmap();

            // Initially create the normal-map from HeightMap, in order to get the dimensions right.
            // The actual contents is overwritten below.
            BitmapT* NormalMap=new BitmapT(*HeightMap);

            // Turn HeightMap into NormalMap.
            // Combine the two normal maps.
            for (unsigned int y=0; y<HeightMap->SizeY; y++)
                for (unsigned int x=0; x<HeightMap->SizeX; x++)
                {
                    float h00, h01, h10, g, b, a;

                    HeightMap->GetPixel( x                      ,  y                      , h00, g, b, a);
                    HeightMap->GetPixel( x                      , (y+1) % HeightMap->SizeY, h01, g, b, a);
                    HeightMap->GetPixel((x+1) % HeightMap->SizeX,  y                      , h10, g, b, a);

                    const float VecX[3]={ 1.0f, 0.0f, (h10-h00)*HeightScale };
                    const float VecY[3]={ 0.0f, 1.0f, (h01-h00)*HeightScale };

                    // Normal=VectorCross(VecX, VecY);
                    float Normal[3]={ VecX[1]*VecY[2]-VecX[2]*VecY[1], VecX[2]*VecY[0]-VecX[0]*VecY[2], VecX[0]*VecY[1]-VecX[1]*VecY[0] };

                    // Normalize Normal.
                    float Length=float(sqrt(Normal[0]*Normal[0] + Normal[1]*Normal[1] + Normal[2]*Normal[2]));

                    if (Length!=0.0)
                    {
                        Normal[0]/=Length;
                        Normal[1]/=Length;
                        Normal[2]/=Length;
                    }

                    NormalMap->SetPixel(x, y, (Normal[0]+1.0f)/2.0f, (Normal[1]+1.0f)/2.0f, (Normal[2]+1.0f)/2.0f);
                }

            delete HeightMap;
            return NormalMap;
        }

        case FlipNormalMapYAxis:
        {
            BitmapT* NormalMap=Child1->GetBitmap();

            for (unsigned int y=0; y<NormalMap->SizeY; y++)
                for (unsigned int x=0; x<NormalMap->SizeX; x++)
                {
                    int nx, ny, nz;

                    NormalMap->GetPixel(x, y, nx,     ny, nz);
                    NormalMap->SetPixel(x, y, nx, 255-ny, nz);
                }

            return NormalMap;
        }

        case ReNormalize:
        {
            BitmapT* NormalMap=Child1->GetBitmap();

            for (unsigned int y=0; y<NormalMap->SizeY; y++)
                for (unsigned int x=0; x<NormalMap->SizeX; x++)
                {
                    float nx, ny, nz;

                    NormalMap->GetPixel(x, y, nx, ny, nz);

                    nx=nx*2.0f-1.0f;
                    ny=ny*2.0f-1.0f;
                    nz=nz*2.0f-1.0f;

                    // Renormalize.
                    const float Length=float(sqrt(nx*nx + ny*ny + nz*nz));

                    if (Length!=0.0)
                    {
                        nx/=Length;
                        ny/=Length;
                        nz/=Length;
                    }

                    NormalMap->SetPixel(x, y, (nx+1.0f)/2.0f, (ny+1.0f)/2.0f, (nz+1.0f)/2.0f);
                }

            return NormalMap;
        }

        case BlueToAlpha:
        {
            BitmapT*      Bitmap  =Child1->GetBitmap();
            const BitmapT Original=*Bitmap;

            for (unsigned int y=0; y<Original.SizeY; y++)
                for (unsigned int x=0; x<Original.SizeX; x++)
                {
                    int r, g, b;

                    Original.GetPixel(x, y, r, g, b);

                    if (r!=0 || g!=0 || b!=255)
                    {
                        // No pure blue, keep the color and make sure alpha is at 255.
                        Bitmap->SetPixel(x, y, r, g, b, 255);
                    }
                    else
                    {
                        // Pure blue, replace with a good average color.
                        int AvgRed  =0;
                        int AvgGreen=0;
                        int AvgBlue =0;
                        int AvgCount=0;

                        for (int ry=-1; ry<=1; ry++)
                            for (int rx=-1; rx<=1; rx++)
                            {
                                const int x_=int(x)+rx;
                                const int y_=int(y)+ry;

                                if (x_<0) continue;
                                if (x_>=int(Original.SizeX)) continue;
                                if (y_<0) continue;
                                if (y_>=int(Original.SizeY)) continue;

                                int r_, g_, b_;

                                Original.GetPixel(x_, y_, r_, g_, b_);
                                if (r_==0 && g_==0 && b_==255) continue;

                                AvgRed  +=r_;
                                AvgGreen+=g_;
                                AvgBlue +=b_;
                                AvgCount+=1;
                            }

                        if (AvgCount>0)
                        {
                            AvgRed  /=AvgCount;
                            AvgGreen/=AvgCount;
                            AvgBlue /=AvgCount;
                        }

                        Bitmap->SetPixel(x, y, AvgRed, AvgGreen, AvgBlue, 0);
                    }
                }

            return Bitmap;


            /* BitmapT* Bitmap=Child1->GetBitmap();

            unsigned long AvgRed  =0;
            unsigned long AvgGreen=0;
            unsigned long AvgBlue =0;
            unsigned long AvgCount=0;

            // Compute the average color.
            for (unsigned long y=0; y<Bitmap->SizeY; y++)
                for (unsigned long x=0; x<Bitmap->SizeX; x++)
                {
                    int r, g, b;

                    Bitmap->GetPixel(x, y, r, g, b);

                    if (r!=0 || g!=0 || b!=255)
                    {
                        // No pure blue, so take this color into account for the average color.
                        AvgRed  +=r;
                        AvgGreen+=g;
                        AvgBlue +=b;
                        AvgCount+=1;
                    }
                }

            if (AvgCount>0)
            {
                AvgRed  /=AvgCount;
                AvgGreen/=AvgCount;
                AvgBlue /=AvgCount;
            }

            // Fill-in the proper alpha and color values.
            for (unsigned long y=0; y<Bitmap->SizeY; y++)
                for (unsigned long x=0; x<Bitmap->SizeX; x++)
                {
                    int r, g, b;

                    Bitmap->GetPixel(x, y, r, g, b);

                    if (r!=0 || g!=0 || b!=255) Bitmap->SetPixel(x, y,      r,        g,       b, 255); // No pure blue, keep color.
                                           else Bitmap->SetPixel(x, y, AvgRed, AvgGreen, AvgBlue,   0); // Pure blue, set avg color.
                }

            return Bitmap; */
        }
    }

    return new BitmapT(BitmapT::GetBuiltInFileNotFoundBitmap());
}


std::string MapCompositionT::GetString() const
{
    switch (Type)
    {
        case Empty:
            // Handled below as the default case.
            break;

        case Map:
            return FileName;

        case Add:
            return std::string("add(")+Child1->GetString()+", "+Child2->GetString()+")";

        case Mul:
            return std::string("mul(")+Child1->GetString()+", "+Child2->GetString()+")";

        case CombineNormals:
            return std::string("combineNMs(")+Child1->GetString()+", "+Child2->GetString()+")";

        case HeightMapToNormalMap:
            return std::string("hm2nm(")+Child1->GetString()+", "+cf::va("%f", HeightScale)+")";

        case FlipNormalMapYAxis:
            return std::string("flipNMyAxis(")+Child1->GetString()+")";

        case ReNormalize:
            return std::string("renormalize(")+Child1->GetString()+")";

        case BlueToAlpha:
            return std::string("blue2alpha(")+Child1->GetString()+")";
    }

    return "";
}


std::string MapCompositionT::GetStringWithOptions(bool NoCompressionDefault) const
{
    const static MapCompositionT RefMapComp;    // Use a reference object here, just in case we ever change the defaults.
    const std::string            BaseString=GetString();
    std::string                  OptionsString="";

    if (BaseString=="")
    {
        // If the base string is empty, don't attempt to attach any options.
        return "";
    }

    if (MinFilter!=RefMapComp.MinFilter)
    {
        OptionsString+=", minFilter ";

        switch (MinFilter)
        {
            case Nearest:                OptionsString+="nearest";                break;
            case Linear:                 OptionsString+="linear";                 break;
            case Nearest_MipMap_Nearest: OptionsString+="nearest_mipmap_nearest"; break;
            case Nearest_MipMap_Linear:  OptionsString+="nearest_mipmap_linear";  break;
            case Linear_MipMap_Nearest:  OptionsString+="linear_mipmap_nearest";  break;
            case Linear_MipMap_Linear:   OptionsString+="linear_mipmap_linear";   break;
        }
    }

    if (MagFilter!=RefMapComp.MagFilter)
    {
        OptionsString+=", magFilter ";

        switch (MagFilter)
        {
            case Nearest: OptionsString+="nearest"; break;
            case Linear:  OptionsString+="linear";  break;
            default:      OptionsString+="INVALID"; break;
        }
    }

    if (WrapS!=RefMapComp.WrapS)
    {
        OptionsString+=", wrapS ";

        switch (WrapS)
        {
            case Repeat:      OptionsString+="repeat";      break;
            case Clamp:       OptionsString+="clamp";       break;
            case ClampToEdge: OptionsString+="clampToEdge"; break;
        }
    }

    if (WrapT!=RefMapComp.WrapT)
    {
        OptionsString+=", wrapT ";

        switch (WrapT)
        {
            case Repeat:      OptionsString+="repeat";      break;
            case Clamp:       OptionsString+="clamp";       break;
            case ClampToEdge: OptionsString+="clampToEdge"; break;
        }
    }

    if (NoScaleDown)
    {
        OptionsString+=", noScaleDown";
    }

    if (NoCompression && !NoCompressionDefault)
    {
        OptionsString+=", noCompression";
    }
    else if (!NoCompression && NoCompressionDefault)
    {
        OptionsString+=", useCompression";
    }

    return BaseString + OptionsString;
}


std::string MapCompositionT::GetBaseDir() const
{
    return (BaseDir==NULL) ? "" : *BaseDir;
}
