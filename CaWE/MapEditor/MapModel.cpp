/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MapModel.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "../Camera.hpp"
#include "../GameConfig.hpp"
#include "../Options.hpp"

#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Models/Model_cmdl.hpp"

#include "wx/wx.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapModelT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapModelT::TypeInfo(GetMapElemTIM(), "MapModelT", "MapPrimitiveT", MapModelT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapModelT::MapModelT(MapDocumentT& MapDoc, const wxString& ModelFileName, const Vector3fT& Position)
    : MapPrimitiveT(wxColour(150 + (rand() % 106), 150 + (rand() % 106), 0)),
      m_ModelFileName(ModelFileName),
      m_Model(MapDoc.GetGameConfig()->GetModel(m_ModelFileName)),
      m_Origin(Position),
      m_CollModelFileName(""),
      m_Label(""),
      m_Angles(),
      m_Scale(1.0f),
      m_AnimExpr(m_Model->GetAnimExprPool().GetStandard(0, 0.0f)),
      m_FrameOffset(0.0f),
      m_FrameTimeScale(1.0f),
      m_Animated(false),
      m_Timer()
{
}


MapModelT::MapModelT(MapDocumentT& MapDoc, const wxString& ModelFileName, const wxString& CollisionModelFileName, const wxString& Label, const Vector3fT& Position, const Vector3fT& Angles, float Scale, int Sequence, float FrameOffset, float FrameTimeScale, bool Animated)
    : MapPrimitiveT(wxColour(150 + (rand() % 106), 150 + (rand() % 106), 0)),
      m_ModelFileName(ModelFileName),
      m_Model(MapDoc.GetGameConfig()->GetModel(m_ModelFileName)),
      m_Origin(Position),
      m_CollModelFileName(CollisionModelFileName),
      m_Label(Label),
      m_Angles(Angles),
      m_Scale(Scale),
      m_AnimExpr(),
      m_FrameOffset(FrameOffset),
      m_FrameTimeScale(FrameTimeScale),
      m_Animated(Animated),
      m_Timer()
{
    m_AnimExpr=m_Model->GetAnimExprPool().GetStandard(Sequence, m_FrameOffset);
}


MapModelT::MapModelT(const MapModelT& Model)
    : MapPrimitiveT(Model),
      m_ModelFileName(Model.m_ModelFileName),
      m_Model(Model.m_Model),
      m_CollModelFileName(Model.m_CollModelFileName),
      m_Label(Model.m_Label),   // Although the value should be unique, we have to copy it anyway, or else copies e.g. for the undo/redo system won't work as expected. Uniqueness must be dealt with and established elsewhere, in a more global scope.
      m_Angles(Model.m_Angles),
      m_Scale(Model.m_Scale),
      m_AnimExpr(dynamic_cast<AnimExprStandardT*>(Model.m_AnimExpr->Clone().get())),
      m_FrameOffset(Model.m_FrameOffset),
      m_FrameTimeScale(Model.m_FrameTimeScale),
      m_Animated(Model.m_Animated),
      m_Timer()
{
}


MapModelT* MapModelT::Clone() const
{
    return new MapModelT(*this);
}


BoundingBox3fT MapModelT::GetBB() const
{
    // TODO: Cache!
    // The 3D bounds are the bounds of the oriented model's first sequence, so that frustum culling works properly in the 3D view.
    Vector3fT VerticesBB[8];
    m_Model->GetSharedPose(m_AnimExpr)->GetBB().GetCornerVertices(VerticesBB);

    // Rotate all eight vertices.
    for (unsigned long VertexNr=0; VertexNr<8; VertexNr++)
        VerticesBB[VertexNr]=VerticesBB[VertexNr].GetRotX(m_Angles[ROLL]).GetRotY(m_Angles[PITCH]).GetRotZ(m_Angles[YAW]);

    // Build a new BB of the rotated BB.
    BoundingBox3fT RotBB(VerticesBB[0]);

    for (unsigned long VertexNr=1; VertexNr<8; VertexNr++)
        RotBB.Insert(VerticesBB[VertexNr]);

    RotBB.Min=RotBB.Min*m_Scale+m_Origin;
    RotBB.Max=RotBB.Max*m_Scale+m_Origin;

    return RotBB;
}


void MapModelT::Render2D(Renderer2DT& Renderer) const
{
    const BoundingBox3fT BB    =GetBB();
    const wxPoint        Point1=Renderer.GetViewWin2D().WorldToTool(BB.Min);
    const wxPoint        Point2=Renderer.GetViewWin2D().WorldToTool(BB.Max);
    const wxPoint        Center=Renderer.GetViewWin2D().WorldToTool(BB.GetCenter());

    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN,
        IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors));

    Renderer.Rectangle(wxRect(Point1, Point2), false);

    // Render the center X handle.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(150, 150, 150));
    Renderer.XHandle(Center);

    // Render the axes of our local system.
    if (IsSelected())
        Renderer.BasisVectors(m_Origin, cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Angles));
}


void MapModelT::Render3D(Renderer3DT& Renderer) const
{
    const Vector3fT ViewPoint=Renderer.GetViewWin3D().GetCamera().Pos;
    const float     ModelDist=length(m_Origin-ViewPoint);
    AnimPoseT*      Pose     =m_Model->GetSharedPose(m_AnimExpr);

    if (Options.view3d.AnimateModels && m_Animated)
    {
        m_AnimExpr->AdvanceTime(float(m_Timer.GetSecondsSinceLastCall())*m_FrameTimeScale);
    }

    if (ModelDist<float(Options.view3d.ModelDistance))
    {
        MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, m_Origin.x, m_Origin.y, m_Origin.z);
        MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD, m_Angles[YAW  ]);
        MatSys::Renderer->RotateY  (MatSys::RendererI::MODEL_TO_WORLD, m_Angles[PITCH]);
        MatSys::Renderer->RotateX  (MatSys::RendererI::MODEL_TO_WORLD, m_Angles[ROLL ]);
        MatSys::Renderer->Scale    (MatSys::RendererI::MODEL_TO_WORLD, m_Scale);

        Pose->Draw(-1 /*default skin*/, ModelDist);

        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        if (IsSelected()) Renderer.RenderBox(GetBB(), Options.colors.Selection, false /* Solid? */);
    }
    else
    {
        // Did not render the real model (the distance was too great), thus render a replacement bounding-box.
        Renderer.RenderBox(GetBB(),
            IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors), true /* Solid? */);
    }

    // Render the axes of our local system.
    if (IsSelected())
        Renderer.BasisVectors(m_Origin, cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Angles));
}


bool MapModelT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    const BoundingBox3fT BB  =GetBB();
    const wxRect         Disc=wxRect(Pixel, Pixel).Inflate(Radius, Radius);
    const wxRect         Rect=wxRect(ViewWin.WorldToWindow(BB.Min), ViewWin.WorldToWindow(BB.Max));

    // Note that the check against the Rect frame (that has a width of 2*Radius) is done in two steps:
    // First by checking if Disc is entirely outside of Rect, then below by checking if Disc is entirely inside Rect.
    if (!Rect.Intersects(Disc)) return false;
    if (Disc.Contains(ViewWin.WorldToWindow(BB.GetCenter()))) return true;
    if (Options.view2d.SelectByHandles) return false;
    return !Rect.Contains(Disc);
}


namespace
{
    class ModelTrafoMementoT : public TrafoMementoT
    {
        public:

        ModelTrafoMementoT(const Vector3fT& Origin, const cf::math::AnglesfT& Angles)
            : m_Origin(Origin),
              m_Angles(Angles)
        {
        }

        const Vector3fT          m_Origin;
        const cf::math::AnglesfT m_Angles;
    };
}


TrafoMementoT* MapModelT::GetTrafoState() const
{
    return new ModelTrafoMementoT(m_Origin, m_Angles);
}


void MapModelT::RestoreTrafoState(const TrafoMementoT* TM)
{
    const ModelTrafoMementoT* ModelTM = dynamic_cast<const ModelTrafoMementoT*>(TM);

    wxASSERT(ModelTM);
    if (!ModelTM) return;

    m_Origin = ModelTM->m_Origin;
    m_Angles = ModelTM->m_Angles;
}


void MapModelT::TrafoMove(const Vector3fT& delta, bool LockTexCoords)
{
    m_Origin+=delta;

    MapPrimitiveT::TrafoMove(delta, LockTexCoords);
}


void MapModelT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords)
{
    // Rotate the origin.
    m_Origin-=RefPoint;

    if (Angles.x!=0.0f) m_Origin=m_Origin.GetRotX( Angles.x);
    if (Angles.y!=0.0f) m_Origin=m_Origin.GetRotY(-Angles.y);
    if (Angles.z!=0.0f) m_Origin=m_Origin.GetRotZ( Angles.z);

    m_Origin+=RefPoint;


    // Convert the existing orientation (expressed in m_Angles) and the additionally to be applied delta rotation
    // (expressed in Angles) to 3x3 rotation matrixes (for backwards-compatibility, both conversions require extra code).
    // Then multiply the matrices in order to obtain the new orientation, and convert that (again bw.-comp.) back to m_Angles.
    const cf::math::Matrix3x3fT OldMatrix=cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Angles);
    const cf::math::Matrix3x3fT RotMatrix=cf::math::Matrix3x3fT::GetFromAngles_COMPAT(cf::math::AnglesfT(-Angles[1], Angles[2], Angles[0]));

    m_Angles=(RotMatrix*OldMatrix).ToAngles_COMPAT();

    // Carefully round and normalize the angles.
    if (fabs(m_Angles[PITCH])<0.001f) m_Angles[PITCH]=0;
    if (fabs(m_Angles[YAW  ])<0.001f) m_Angles[YAW  ]=0;
    if (m_Angles[YAW]<0) m_Angles[YAW]+=360.0f;
    if (fabs(m_Angles[ROLL ])<0.001f) m_Angles[ROLL ]=0;

    MapPrimitiveT::TrafoRotate(RefPoint, Angles, LockTexCoords);
}


void MapModelT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords)
{
    m_Origin=RefPoint + (m_Origin-RefPoint).GetScaled(Scale);

    MapPrimitiveT::TrafoScale(RefPoint, Scale, LockTexCoords);
}


void MapModelT::TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords)
{
    // Unfortunately, there is no way to mirror the actual mesh of the model...
    m_Origin[NormalAxis]=Dist-(m_Origin[NormalAxis]-Dist);

    MapPrimitiveT::TrafoMirror(NormalAxis, Dist, LockTexCoords);
}


void MapModelT::Transform(const MatrixT& Matrix, bool LockTexCoords)
{
    m_Origin=Matrix.Mul1(m_Origin);

    MapPrimitiveT::Transform(Matrix, LockTexCoords);
}
