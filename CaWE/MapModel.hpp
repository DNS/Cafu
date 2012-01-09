/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _MAP_MODEL_HPP_
#define _MAP_MODEL_HPP_

#include "MapPrimitive.hpp"
#include "Util/Util.hpp"


class MapDocumentT;
class CafuModelT;


class MapModelT : public MapPrimitiveT
{
    public:

    MapModelT(MapDocumentT& MapDoc, const wxString& ModelFileName, const Vector3fT& Position);
    MapModelT(MapDocumentT& MapDoc, const wxString& ModelFileName, const wxString& CollisionModelFileName, const wxString& Label, const Vector3fT& Position, const Vector3fT& Angles, float Scale, int Sequence, float FrameOffset, float FrameTimeScale, bool Animated);

    /// The copy constructor for copying a model.
    /// @param Model   The model to copy-construct this model from.
    MapModelT(const MapModelT& Model);


    // Implementations and overrides for base class methods.
    MapModelT* Clone() const;
    void       Assign(const MapElementT* Elem);


    // MapElementT implementation.
    BoundingBox3fT GetBB() const;

    void Render2D(Renderer2DT& Renderer) const;
    void Render3D(Renderer3DT& Renderer) const;

    bool TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;
    wxString GetDescription() const { return "Model"; }

    // Implement the MapElementT transformation methods.
    void TrafoMove(const Vector3fT& Delta);
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles);
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale);
    void TrafoMirror(unsigned int NormalAxis, float Dist);
    void Transform(const MatrixT& Matrix);

    void Load_cmap(TextParserT& TP, MapDocumentT& MapDoc);
    void Save_cmap(std::ostream& OutFile, unsigned long ModelNr, const MapDocumentT& MapDoc) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    friend class InspDlgPrimitivePropsT;
    friend class CommandModifyModelT;

    wxString           m_ModelFileName;
    const CafuModelT*  m_Model;
    Vector3fT          m_Origin;
    wxString           m_CollModelFileName;
    wxString           m_Label;
    cf::math::AnglesfT m_Angles;
    float              m_Scale;
    int                m_SeqNumber;
    float              m_FrameOffset;
    float              m_FrameTimeScale;
    mutable float      m_FrameNumber;
    bool               m_Animated;
    mutable TimerT     m_Timer;
};

#endif
