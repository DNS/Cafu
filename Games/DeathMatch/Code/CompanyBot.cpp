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

#include "CompanyBot.hpp"
#include "EntityCreateParams.hpp"
#include "HumanPlayer.hpp"
#include "Interpolator.hpp"
#include "PhysicsWorld.hpp"
#include "TypeSys.hpp"
#include "Libs/LookupTables.hpp"
#include "Libs/Physics.hpp"
#include "../../GameWorld.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/AnimPose.hpp"
#include "Models/Model_cmdl.hpp"
#include "Network/State.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntCompanyBotT::GetType() const
{
    return &TypeInfo;
 // return &EntCompanyBotT::TypeInfo;
}

void* EntCompanyBotT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntCompanyBotT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntCompanyBotT::TypeInfo(GetBaseEntTIM(), "EntCompanyBotT", "BaseEntityT", EntCompanyBotT::CreateInstance, NULL /*MethodsList*/);


EntCompanyBotT::EntCompanyBotT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT( 300.0,  300.0,   100.0),
                                 Vector3dT(-300.0, -300.0, -1728.8)),   // 68*25.4 == 1727.2
                  0),
      State(VectorT(),
            0,
            0,
            0,       // ModelIndex
            0,       // ModelSequNr
            0.0,     // ModelFrameNr
            80,      // Health
            0,       // Armor
            0,       // HaveItems
            0,       // HaveWeapons
            0,       // ActiveWeaponSlot
            0,       // ActiveWeaponSequNr
            0.0),    // ActiveWeaponFrameNr
      m_Physics(m_Origin, State.Velocity, m_Dimensions, ClipModel, GameWorld->GetClipWorld()),
      m_CompanyBotModel(Params.GameWorld->GetModel("Games/DeathMatch/Models/Players/Trinity/Trinity.cmdl")),
      m_AnimExpr(),
      m_LastStdAE(),
      m_WeaponModel(Params.GameWorld->GetModel("Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_p.cmdl")),
      m_TimeForLightSource(0.0f)
{
    Register(new InterpolatorT<Vector3dT>(m_Origin));

    m_LastStdAE=m_CompanyBotModel->GetAnimExprPool().GetStandard(State.ModelSequNr, State.ModelFrameNr);
    m_AnimExpr =m_LastStdAE;

    // Wir könnten im Boden stecken oder darüber schweben - korrigiere entsprechend!
    // If multiple solid entities are stacked upon each other, this code might leave gaps between them,
    // depending on their order. Not a serious problem, though - the physics code will correct it.
    // Also note that if m_Dimensions is stuck, this code tries to make it un-stuck, but is not guaranteed to succeed!

    // First, create a BB of dimensions (-300.0, -300.0, -100.0) - (300.0, 300.0, 100.0).
    const BoundingBox3T<double> ClearingBB(VectorT(m_Dimensions.Min.x, m_Dimensions.Min.y, -m_Dimensions.Max.z), m_Dimensions.Max);

    // Move ClearingBB up to a reasonable height (if possible!), such that the *full* BB (that is, m_Dimensions) is clear of (not stuck in) solid.
    cf::ClipSys::TraceResultT Result(1.0);
    GameWorld->GetClipWorld().TraceBoundingBox(ClearingBB, m_Origin, VectorT(0.0, 0.0, 3000.0), MaterialT::Clip_Players, &ClipModel, Result);
    const double AddHeight=3000.0*Result.Fraction;

    // Move ClearingBB down as far as possible.
    GameWorld->GetClipWorld().TraceBoundingBox(ClearingBB, m_Origin+VectorT(0.0, 0.0, AddHeight), VectorT(0.0, 0.0, -999999.0), MaterialT::Clip_Players, &ClipModel, Result);
    const double SubHeight=999999.0*Result.Fraction;

    // Beachte: Hier für Epsilon 1.0 (statt z.B. 1.23456789) zu wählen hebt u.U. GENAU den (0 0 -1) Test in
    // Physics::CategorizePosition() auf! Nicht schlimm, wenn aber auf Client-Seite übers Netz kleine Rundungsfehler
    // vorhanden sind (es werden floats übertragen, nicht doubles!), kommt CategorizePosition() u.U. auf Client- und
    // Server-Seite zu verschiedenen Ergebnissen! Der Effekt spielt sich zwar in einem Intervall der Größe 1.0 ab,
    // kann mit OpenGL aber zu deutlichem Pixel-Flimmern führen!
    m_Origin.z=m_Origin.z+AddHeight-SubHeight+(ClearingBB.Min.z-m_Dimensions.Min.z/*1628.8*/)+1.23456789/*Epsilon (sonst Ruckeln am Anfang!)*/;

    // Old, deprecated code (can get us stuck in non-level ground).
    // const double HeightAboveGround=GameWorld->MapClipLine(m_Origin, VectorT(0.0, 0.0, -1.0), 0.0, 999999.9);
    // m_Origin.z=m_Origin.z-HeightAboveGround-m_Dimensions.Min.z+1.23456789/*Epsilon (sonst Ruckeln am Anfang!)*/;


    if (CollisionModel==NULL)
    {
        // No "normal" collision model has been set for this company bot.
        // Now simply setup a bounding box as the collision model.
        CollisionModel=cf::ClipSys::CollModelMan->GetCM(m_Dimensions, MaterialManager->GetMaterial("Textures/meta/collisionmodel"));
        ClipModel.SetCollisionModel(CollisionModel);
        ClipModel.SetOrigin(m_Origin);
        ClipModel.Register();
    }


    // The /1000 is because our physics world is in meters.
    m_CollisionShape=new btBoxShape(conv((m_Dimensions.Max-m_Dimensions.Min)/2.0/1000.0));  // Should use a btCylinderShapeZ instead of btBoxShape?

    // Our rigid body is of Bullet type "kinematic". That is, we move it ourselves, not the world dynamics.
    m_RigidBody=new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(0, this /*btMotionState for this body*/, m_CollisionShape, btVector3()));

    m_RigidBody->setUserPointer(this);  // This entity is associated to the m_RigidBody.
    m_RigidBody->setCollisionFlags(m_RigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_RigidBody->setActivationState(DISABLE_DEACTIVATION);

    GameWorld->GetPhysicsWorld().AddRigidBody(m_RigidBody);
}


EntCompanyBotT::~EntCompanyBotT()
{
    if (m_RigidBody->isInWorld())
        GameWorld->GetPhysicsWorld().RemoveRigidBody(m_RigidBody);

    delete m_RigidBody;
    delete m_CollisionShape;
}


void EntCompanyBotT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << float(State.Velocity.x);
    Stream << float(State.Velocity.y);
    Stream << float(State.Velocity.z);
    Stream << State.StateOfExistance;
    Stream << State.Flags;
    Stream << State.PlayerName;       // TODO: In the old code, the PlayerName apparently is read/written in *baseline* messages only.
    Stream << State.ModelIndex;
    Stream << State.ModelSequNr;
    Stream << State.ModelFrameNr;
    Stream << State.Health;
    Stream << State.Armor;
    Stream << uint32_t(State.HaveItems);
    Stream << uint32_t(State.HaveWeapons);
    Stream << State.ActiveWeaponSlot;
    Stream << State.ActiveWeaponSequNr;
    Stream << State.ActiveWeaponFrameNr;

    for (unsigned int Nr=0; Nr<16; Nr++) Stream << State.HaveAmmo[Nr];
    for (unsigned int Nr=0; Nr<32; Nr++) Stream << uint32_t(State.HaveAmmoInWeapons[Nr]);
}


void EntCompanyBotT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    float    f =0.0f;
    uint32_t ui=0;

    Stream >> f; State.Velocity.x=f;
    Stream >> f; State.Velocity.y=f;
    Stream >> f; State.Velocity.z=f;
    Stream >> State.StateOfExistance;
    Stream >> State.Flags;
    Stream >> State.PlayerName;     // TODO: In the old code, the PlayerName apparently is read/written in *baseline* messages only.
    Stream >> State.ModelIndex;
    Stream >> State.ModelSequNr;
    Stream >> State.ModelFrameNr;
    Stream >> State.Health;
    Stream >> State.Armor;
    Stream >> ui; State.HaveItems=ui;
    Stream >> ui; State.HaveWeapons=ui;
    Stream >> State.ActiveWeaponSlot;
    Stream >> State.ActiveWeaponSequNr;
    Stream >> State.ActiveWeaponFrameNr;

    for (unsigned int Nr=0; Nr<16; Nr++) Stream >> State.HaveAmmo[Nr];
    for (unsigned int Nr=0; Nr<32; Nr++) { Stream >> ui; State.HaveAmmoInWeapons[Nr]=(unsigned char)ui; }


    const bool IsAlive=(State.ModelSequNr<18 || State.ModelSequNr>24);

    btTransform WorldTrafo;
    getWorldTransform(WorldTrafo);
    m_RigidBody->setWorldTransform(WorldTrafo);

    if (IsAlive)
    {
        ClipModel.SetOrigin(m_Origin);
        ClipModel.Register();

        if (!m_RigidBody->isInWorld())
            GameWorld->GetPhysicsWorld().AddRigidBody(m_RigidBody);
    }
    else
    {
        ClipModel.Unregister();

        if (m_RigidBody->isInWorld())
            GameWorld->GetPhysicsWorld().RemoveRigidBody(m_RigidBody);
    }
}


void EntCompanyBotT::TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir)
{
    // Dead company bots can take no further damage.
    if (State.ModelSequNr>=18 && State.ModelSequNr<=24) return;

    State.Velocity=State.Velocity+scale(VectorT(ImpactDir.x, ImpactDir.y, 0.0), 500.0*Amount);

    if (State.Health<=Amount)
    {
        unsigned short DeltaAngle=Entity->GetHeading()-m_Heading;

        State.Health=0;

             if (DeltaAngle>=57344 || DeltaAngle< 8192) State.ModelSequNr=21;   // 315° ...  45° - die forwards
        else if (DeltaAngle>=8192  && DeltaAngle<16384) State.ModelSequNr=22;   //  45° ...  90° - headshot
        else if (DeltaAngle>=16384 && DeltaAngle<24576) State.ModelSequNr=24;   //  90° ... 135° - gutshot
        else if (DeltaAngle>=24576 && DeltaAngle<32768) State.ModelSequNr=19;   // 135° ... 180° - die backwards1
        else if (DeltaAngle>=32768 && DeltaAngle<40960) State.ModelSequNr=20;   // 180° ... 225° - die backwards
        else if (DeltaAngle>=40960 && DeltaAngle<49152) State.ModelSequNr=18;   // 225° ... 270° - die simple
        else /* (DeltaAngle>=49152&&DeltaAngle<57344)*/ State.ModelSequNr=23;   // 270° ... 315° - die spin

        State.ModelFrameNr=0.0;
        ClipModel.Unregister();     // Dead now, don't clip no more.
        GameWorld->GetPhysicsWorld().RemoveRigidBody(m_RigidBody);

        // Count the frag at the "creator" entity.
        IntrusivePtrT<BaseEntityT> FraggingEntity=Entity;

        while (FraggingEntity->ParentID!=0xFFFFFFFF)
        {
            IntrusivePtrT<BaseEntityT> ParentOfFE=static_pointer_cast<BaseEntityT>(GameWorld->GetGameEntityByID(FraggingEntity->ParentID));

            if (ParentOfFE.IsNull()) break;
            FraggingEntity=ParentOfFE;
        }

        IntrusivePtrT<EntHumanPlayerT> HumanFragEnt = dynamic_pointer_cast<EntHumanPlayerT>(FraggingEntity);

        if (HumanFragEnt!=NULL)
            HumanFragEnt->AddFrag();
    }
    else
    {
        State.Health-=Amount;
    }
}


void EntCompanyBotT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    if (State.ModelSequNr>=18 && State.ModelSequNr<=24)
    {
        bool DummyOldWishJump=false;
        m_Physics.MoveHuman(FrameTime, m_Heading, VectorT(), VectorT(), false, DummyOldWishJump, 0.0);

        AdvanceModelTime(FrameTime, false);

        // As we're in "dead" state, the ClipModel is no longer registered with the clip world,
        // and there is no need to update its position or to re-register.
        // ClipModel.SetOrigin(m_Origin);
        // ClipModel.Register();
        return;
    }


    // Suche als Ziel den ersten HumanPlayer-Entity, der 'alive' ist.
    const ArrayT<unsigned long>& AllEntityIDs=GameWorld->GetAllEntityIDs();
    IntrusivePtrT<BaseEntityT>   TargetEntity=NULL;
    unsigned long                EntityIDNr;

    for (EntityIDNr=0; EntityIDNr<AllEntityIDs.Size(); EntityIDNr++)
    {
        bool IsAlive=false;

        TargetEntity=static_pointer_cast<BaseEntityT>(GameWorld->GetGameEntityByID(AllEntityIDs[EntityIDNr]));
        if (TargetEntity==NULL) continue;

        if (TargetEntity->GetType()!=&EntHumanPlayerT::TypeInfo) continue;

        TargetEntity->ProcessConfigString(&IsAlive, "IsAlive?");
        if (!IsAlive) continue;

        // Gefunden!
        break;
    }

    // Gab es so einen Entity überhaupt?
    if (EntityIDNr>=AllEntityIDs.Size()) return;

    VectorT Dist = TargetEntity->GetOrigin() - m_Origin;
    VectorT WishVelocity;

    // This is really easy thinking
    Dist.z=0;
    if (dot(Dist, Dist)>10000*10000)
    {
        VectorT Dir=normalize(Dist, 0.0);

        WishVelocity=scale(Dir, /*WishSpeed*/7000.0);
        m_Heading=(unsigned short)(acos(Dir.y)/3.1415926*32768.0);
        if (Dir.x<0) m_Heading=/*65536*/-m_Heading;
    }

    VectorT XYVel=State.Velocity;
    XYVel.z=0;
    double OldSpeed=length(XYVel);

    bool DummyOldWishJump=false;
    m_Physics.MoveHuman(FrameTime, m_Heading, WishVelocity, VectorT(), false, DummyOldWishJump, 610.0);
    // m_Origin=m_Origin+scale(WishVelocity, FrameTime); State.Velocity=WishVelocity;
    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();

    XYVel=State.Velocity;
    XYVel.z=0;
    double NewSpeed=length(XYVel);

    AdvanceModelTime(FrameTime, true);

    if (OldSpeed<1000 && NewSpeed>1000) { State.ModelSequNr=3; State.ModelFrameNr=0.0; }
    if (OldSpeed>1000 && NewSpeed<1000) { State.ModelSequNr=1; State.ModelFrameNr=0.0; }


    // For further demonstration, let the CompanyBot have a torch.   OBSOLETE! GetLightSourceInfo() has this stuff now.
    ///if (TorchID==0xFFFFFFFF) TorchID=GameWorld->CreateNewEntity("PointLightSource", ServerFrameNr, VectorT());

    // Update the position of the torch.
    // The torch should probably rather be a true *child* of the CompanyBot...
    ///BaseEntityT* Torch=GameWorld->GetGameEntityByID(TorchID);
    ///if (Torch) Torch->m_Origin=m_Origin+VectorT(0.0, 0.0, -300.0)+VectorT(LookupTables::Angle16ToSin[m_Heading]*500.0, LookupTables::Angle16ToCos[m_Heading]*500.0, 0.0);
}


static ConVarT HasLight("game_BotsHaveLight", true, ConVarT::FLAG_GAMEDLL, "Determines whether the company bot entities all carry a dynamic lightsource.");

bool EntCompanyBotT::GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    if (!HasLight.GetValueBool()) return false;

    if (m_TimeForLightSource<2.0f)
    {
        // 0.0 <= m_TimeForLightSource < 2.0
        const float         Value=1.0f-0.5f*(1.0f+LookupTables::Angle16ToCos[(unsigned short)(m_TimeForLightSource/4.0f*65536.0f)]);
        const unsigned long Red  =char(255.0f*Value);
        const unsigned long Green=char(255.0f*Value*0.9f);

        DiffuseColor =(Green << 8)+Red;
        SpecularColor=0x440000+(Green << 8);
    }
    else
    {
        // 2.0 <= m_TimeForLightSource < 6.0
        const float         Value=0.5f*(1.0f+LookupTables::Angle16ToCos[(unsigned short)((m_TimeForLightSource-2.0f)/4.0f*65536.0f)]);
        const unsigned long Green=char(255.0f*(Value*0.8f+0.1f));

        DiffuseColor =(Green << 8)+0xFF;
        SpecularColor=0x440000+(Green << 8);
    }

    const float   Value=LookupTables::Angle16ToCos[(unsigned short)((m_TimeForLightSource-2.0f)/4.0f*65536.0f)];
    const VectorT RelX =scale(VectorT(LookupTables::Angle16ToCos[m_Heading], LookupTables::Angle16ToSin[m_Heading], 0.0), 80.0*Value);
    const VectorT RelY =scale(VectorT(LookupTables::Angle16ToSin[m_Heading], LookupTables::Angle16ToCos[m_Heading], 0.0), 500.0);
    const VectorT RelZ =VectorT(0.0, 0.0, -300.0+0.5*char(DiffuseColor >> 8));

    Position    =m_Origin+RelX+RelY+RelZ;
    Radius      =10000.0;
    CastsShadows=true;

    return true;
}


void EntCompanyBotT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    MatSys::Renderer->GetCurrentLightSourcePosition()[2]+=32.0f;
    MatSys::Renderer->GetCurrentEyePosition        ()[2]+=32.0f;
    MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, 0.0f, 0.0f, -32.0f);

    AnimPoseT* Pose=m_CompanyBotModel->GetSharedPose(m_AnimExpr);
    Pose->Draw(-1 /*default skin*/, LodDist);

    AnimPoseT* WeaponPose=m_WeaponModel->GetSharedPose(m_WeaponModel->GetAnimExprPool().GetStandard(0, 0.0f));
    WeaponPose->SetSuperPose(Pose);
    WeaponPose->Draw(-1 /*default skin*/, LodDist);
    WeaponPose->SetSuperPose(NULL);
}


void EntCompanyBotT::PostDraw(float FrameTime, bool /*FirstPersonView*/)
{
    // Implicit simple "mini-prediction".
    AdvanceModelTime(FrameTime, State.ModelSequNr<18 || State.ModelSequNr>24);

    // Advance the time for the light source.
    m_TimeForLightSource+=FrameTime;
    if (m_TimeForLightSource>6.0f) m_TimeForLightSource-=4.0f;
}


void EntCompanyBotT::getWorldTransform(btTransform& worldTrans) const
{
    Vector3dT Origin=m_Origin;

    // The box shape of our physics body is equally centered around the origin point,
    // whereas our m_Dimensions box is "non-uniformely displaced".
    // In order to compensate, compute how far the m_Dimensions center is away from the origin.
    Origin.z+=(m_Dimensions.Min.z+m_Dimensions.Max.z)/2.0;

    // Return the current transformation of our rigid body to the physics world.
    worldTrans.setIdentity();
    worldTrans.setOrigin(conv(Origin/1000.0));
}


void EntCompanyBotT::setWorldTransform(const btTransform& /*worldTrans*/)
{
    // Never called for a kinematic rigid body.
    assert(false);
}


#undef min    // See http://stackoverflow.com/questions/5004858/stdmin-gives-error
#undef max


void EntCompanyBotT::AdvanceModelTime(float Time, bool Loop)
{
    if (State.ModelSequNr==m_LastStdAE->GetSequNr())
    {
        m_LastStdAE->SetFrameNr(State.ModelFrameNr);
    }
    else
    {
        const bool IsAlive=(State.ModelSequNr<18 || State.ModelSequNr>24);
        float      BlendTime=0.3f;

        if (!IsAlive)
        {
            BlendTime=0.2f;

            if (State.ModelSequNr>=0 && State.ModelSequNr<int(m_CompanyBotModel->GetAnims().Size()))
            {
                const CafuModelT::AnimT& Anim=m_CompanyBotModel->GetAnims()[State.ModelSequNr];

                if (Anim.Frames.Size() > 0)
                    BlendTime=std::min(BlendTime, (Anim.Frames.Size()-1) * Anim.FPS * 0.5f);
            }
        }

        m_LastStdAE=m_CompanyBotModel->GetAnimExprPool().GetStandard(State.ModelSequNr, 0.0f);
        m_AnimExpr =m_CompanyBotModel->GetAnimExprPool().GetBlend(m_AnimExpr, m_LastStdAE, BlendTime);
    }

    m_LastStdAE->SetForceLoop(Loop);
    m_AnimExpr->AdvanceTime(Time);
    State.ModelFrameNr=m_LastStdAE->GetFrameNr();
}
