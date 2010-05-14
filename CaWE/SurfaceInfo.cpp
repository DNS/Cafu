/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "SurfaceInfo.hpp"

#include "TextParser/TextParser.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Math3D/Misc.hpp"

#include <ostream>


SurfaceInfoT::SurfaceInfoT()
    : TexCoordGenMode(Custom),
      Rotate(0.0f),
      UAxis(),
      VAxis(),
      LightmapScale(16.0f)
{
    Scale[0]=1.0f;
    Scale[1]=1.0f;

    Trans[0]=0.0f;
    Trans[1]=0.0f;
}


SurfaceInfoT::SurfaceInfoT(const Plane3fT& Plane, bool FaceAligned)
    : TexCoordGenMode(PlaneProj),
      Rotate(0.0f),
      UAxis(),
      VAxis(),
      LightmapScale(16.0f)
{
    Scale[0]=1.0f;
    Scale[1]=1.0f;

    Trans[0]=0.0f;
    Trans[1]=0.0f;

    ResetUVAxes(Plane, FaceAligned);
}


SurfaceInfoT SurfaceInfoT::Create_cmap(TextParserT& TP)
{
    SurfaceInfoT SI;

    TP.AssertAndSkipToken("(");
    SI.TexCoordGenMode=TexCoordGenModeT(TP.GetNextTokenAsInt());

    if (SI.TexCoordGenMode==MatFit)
    {
        SI.Trans[0]=TP.GetNextTokenAsFloat();
        SI.Trans[1]=TP.GetNextTokenAsFloat();
        SI.Scale[0]=TP.GetNextTokenAsFloat();
        SI.Scale[1]=TP.GetNextTokenAsFloat();
        SI.Rotate  =TP.GetNextTokenAsFloat();
    }

    if (SI.TexCoordGenMode==PlaneProj)
    {
        SI.Trans[0]=TP.GetNextTokenAsFloat();
        SI.Trans[1]=TP.GetNextTokenAsFloat();
        SI.Rotate  =TP.GetNextTokenAsFloat();

        TP.AssertAndSkipToken("(");
        SI.UAxis.x=TP.GetNextTokenAsFloat();
        SI.UAxis.y=TP.GetNextTokenAsFloat();
        SI.UAxis.z=TP.GetNextTokenAsFloat();
        TP.AssertAndSkipToken(")");

        TP.AssertAndSkipToken("(");
        SI.VAxis.x=TP.GetNextTokenAsFloat();
        SI.VAxis.y=TP.GetNextTokenAsFloat();
        SI.VAxis.z=TP.GetNextTokenAsFloat();
        TP.AssertAndSkipToken(")");

        // Restore the scale values and renormalize the axes.
        const float LenU=length(SI.UAxis);
        const float LenV=length(SI.VAxis);

        SI.UAxis/=LenU;
        SI.VAxis/=LenV;

        SI.Scale[0]=1.0f/LenU;
        SI.Scale[1]=1.0f/LenV;
    }

    TP.AssertAndSkipToken(")");
    return SI;
}


void SurfaceInfoT::Save_cmap(std::ostream& OutFile) const
{
    // Temporarily reduce the precision from 9 to 6 decimal digits.
    // Initially we only used this for the six SaveU and SaveV values below, see svn log -r 155 for the details
    // (we do math with the these numbes between loading and saving, introducing subtle rounding errors).
    // However, using the implied rounding of the 6 digits precision also with the other numbers (Trans, Scale
    // and Rotate) with their typically well-known, limited range of values seems to be a good idea as well.
    const std::streamsize OldPrecision=OutFile.precision(std::numeric_limits<float>::digits10);

    OutFile << "( " << TexCoordGenMode;

    if (TexCoordGenMode==MatFit)
    {
        OutFile << " " << Trans[0] << " " << Trans[1]
                << " " << Scale[0] << " " << Scale[1] << " " << Rotate;
    }
    else if (TexCoordGenMode==PlaneProj)
    {
        OutFile << " " << Trans[0] << " " << Trans[1] << " " << Rotate;

        // Include the scale factors into their axes.
        Vector3fT SaveU=UAxis/Scale[0];
        Vector3fT SaveV=VAxis/Scale[1];

        // Turn a vector like (0 -4.89843e-016 8) into (0 0 8).
        // Even though a certain kind of rounding is implied by limiting the output precision of floats
        // to 6 decimal digits below, it is also worthwhile to implement a "fixed" rounding near zero here.
        if (fabs(SaveU.x)<0.000001f) SaveU.x=0.0f;
        if (fabs(SaveU.y)<0.000001f) SaveU.y=0.0f;
        if (fabs(SaveU.z)<0.000001f) SaveU.z=0.0f;
        if (fabs(SaveV.x)<0.000001f) SaveV.x=0.0f;
        if (fabs(SaveV.y)<0.000001f) SaveV.y=0.0f;
        if (fabs(SaveV.z)<0.000001f) SaveV.z=0.0f;

        OutFile << " ( " << SaveU.x << " " << SaveU.y  << " " << SaveU.z << " )";
        OutFile << " ( " << SaveV.x << " " << SaveV.y  << " " << SaveV.z << " )";
    }

    OutFile << " )\n";

    // Restore original precision mode.
    OutFile.precision(OldPrecision);
}


static unsigned int GetMainAxis(const Vector3fT& PlaneNormal)
{
    unsigned int MainAxis=0;
    float        MaxVal  =fabs(PlaneNormal[0]);

    for (unsigned int AxisNr=1; AxisNr<3; AxisNr++)
    {
        if (fabs(PlaneNormal[AxisNr]) > MaxVal)
        {
            MaxVal  =fabs(PlaneNormal[AxisNr]);
            MainAxis=AxisNr;
        }
    }

    return MainAxis;
}


void SurfaceInfoT::ResetUVAxes(const Plane3fT& Plane, bool FaceAligned)
{
    assert(TexCoordGenMode==PlaneProj);

    // Always initialize the axes world-aligned first.
    const unsigned int MainAxis=GetMainAxis(Plane.Normal);

    UAxis=Vector3fT(1, 0,  0);
    VAxis=Vector3fT(0, 0, -1);

    if (MainAxis==0) UAxis=Vector3fT(0,  1, 0);
    if (MainAxis==2) VAxis=Vector3fT(0, -1, 0);

    // Now "modify" the world-aligned axes so that they become face-aligned.
    if (FaceAligned)
    {
        UAxis=normalizeOr0(cross(Plane.Normal, VAxis));
        VAxis=normalizeOr0(cross(UAxis, Plane.Normal));
    }

    RotateUVAxes(Rotate);
}


void SurfaceInfoT::WrapTranslations()
{
    assert(TexCoordGenMode==PlaneProj);

    Trans[0]=fmod(Trans[0], 1.0f);
    Trans[1]=fmod(Trans[1], 1.0f);
}


void SurfaceInfoT::RotateUVAxes(float Angle)
{
    if (TexCoordGenMode!=PlaneProj) return;
    if (Angle==0.0f) return;

    const cf::math::Matrix3x3T<float> Matrix=cf::math::Matrix3x3T<float>::GetRotateMatrix(Angle, cross(VAxis, UAxis));

    UAxis=Matrix*UAxis;
    VAxis=Matrix*VAxis;
}


void SurfaceInfoT::AlignMaterial(const char* AlignKey, const ArrayT<Vector3fT>& Vertices)
{
    assert(TexCoordGenMode==PlaneProj);

    TexCoordT TopLeft;
    TexCoordT BottomRight;

    // Project each vertex into the texture plane in order to determine the "texture-space bounding-box"
    // of the vertices (that is, the 2D bounding-box over all texture coordinates).
    // Note that the projection includes scale, but not translation (which is computed below) or rotation.
    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        TexCoordT Test;

        Test[0]=dot(Vertices[VertexNr], UAxis)*Scale[0];
        Test[1]=dot(Vertices[VertexNr], VAxis)*Scale[1];

        if (VertexNr==0)
        {
            TopLeft    =Test;
            BottomRight=Test;
        }
        else
        {
            if (Test[0]<    TopLeft[0])     TopLeft[0]=Test[0];
            if (Test[1]<    TopLeft[1])     TopLeft[1]=Test[1];
            if (Test[0]>BottomRight[0]) BottomRight[0]=Test[0];
            if (Test[1]>BottomRight[1]) BottomRight[1]=Test[1];
        }
    }

    switch (AlignKey[0])
    {
        case 't':   // Top.
            Trans[1]=-TopLeft[1];
            break;

        case 'b':   // Bottom.
            Trans[1]=-BottomRight[1] + 1.0f;
            break;

        case 'l':   // Left.
            Trans[0]=-TopLeft[0];
            break;

        case 'r':   // Right.
            Trans[0]=-BottomRight[0] + 1.0f;
            break;

        case 'c':   // Center.
            Trans[0]=-(TopLeft[0]+BottomRight[0])/2.0f + 0.5f;
            Trans[1]=-(TopLeft[1]+BottomRight[1])/2.0f + 0.5f;
            break;

        case 'f':   // Fit.
        {
            if (TopLeft[0]!=BottomRight[0]) Scale[0]/=BottomRight[0] - TopLeft[0];
            if (TopLeft[1]!=BottomRight[1]) Scale[1]/=BottomRight[1] - TopLeft[1];

            AlignMaterial("top",  Vertices);
            AlignMaterial("left", Vertices);
            break;
        }
    }

    WrapTranslations();
}
