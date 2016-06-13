/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAP_MODEL_HPP_INCLUDED
#define CAFU_MAP_MODEL_HPP_INCLUDED

#include "MapPrimitive.hpp"
#include "Models/AnimExpr.hpp"
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
    MapModelT* Clone() const override;


    // MapElementT implementation.
    BoundingBox3fT GetBB() const;

    void Render2D(Renderer2DT& Renderer) const;
    void Render3D(Renderer3DT& Renderer) const;

    bool TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;
    wxString GetDescription() const { return "Model"; }

    // Implement the MapElementT transformation methods.
    TrafoMementoT* GetTrafoState() const override;
    void RestoreTrafoState(const TrafoMementoT* TM) override;
    void TrafoMove(const Vector3fT& Delta, bool LockTexCoords) override;
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords) override;
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords) override;
    void TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords) override;
    void Transform(const Matrix4x4fT& Matrix, bool LockTexCoords) override;

    void Load_cmap(TextParserT& TP, MapDocumentT& MapDoc, bool IgnoreGroups) override;
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
    mutable IntrusivePtrT<AnimExprStandardT> m_AnimExpr;    ///< The current expression used for configuring the pose of the model.
    float              m_FrameOffset;
    float              m_FrameTimeScale;
    bool               m_Animated;
    mutable TimerT     m_Timer;
};

#endif
