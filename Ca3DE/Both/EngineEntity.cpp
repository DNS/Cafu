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

/***************************/
/*** EngineEntity (Code) ***/
/***************************/

#include "EngineEntity.hpp"
#include "../NetConst.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Network/Network.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "../../Games/Game.hpp"
#include "Ca3DEWorld.hpp"   // Only for EngineEntityT::Draw().


extern ConVarT GlobalTime;


/******************/
/*** Both Sides ***/
/******************/


EngineEntityT::~EngineEntityT()
{
    for (unsigned long OldStateNr=0; OldStateNr<OldStates.Size(); OldStateNr++) delete OldStates[OldStateNr];

    // Wir können nicht einfach 'delete Entity;' schreiben, denn 'Entity' wurde in der GameDLL allokiert,
    // und muß dort auch wieder freigegeben werden! (The "new/delete cannot cross EXEs/DLLs" problem.)
    cf::GameSys::Game->FreeBaseEntity(Entity);
}


BaseEntityT* EngineEntityT::GetBaseEntity() const
{
    return Entity;
}


void EngineEntityT::ProcessConfigString(const void* ConfigData, const char* ConfigString)
{
    Entity->ProcessConfigString(ConfigData, ConfigString);
}


/*******************/
/*** Server Side ***/
/*******************/


EngineEntityT::EngineEntityT(BaseEntityT* Entity_, unsigned long CreationFrameNr)
    : Entity(Entity_),
      EntityStateFrameNr(CreationFrameNr),
      BaseLine(Entity->State),
      BaseLineFrameNr(CreationFrameNr),
   // OldStates,
   // PlayerCommands,
      PredictedState(Entity->State),
      OldEvents(0),
      InterpolateState0(NULL),
      InterpolateTime0(0),
      InterpolateTime1(0)
{
    for (unsigned long OldStateNr=0; OldStateNr<16 /*MUST be a power of 2*/; OldStateNr++)
        OldStates.PushBack(new EntityStateT(Entity_->State));

    // The array of PlayerCommands remains empty for server purposes.
}


void EngineEntityT::PreThink(unsigned long ServerFrameNr)
{
    // 1. Ein Entity, der für dieses zu erstellende Frame 'ServerFrameNr' erst neu erzeugt wurde, soll nicht gleich denken!
    //    Ein einfacher Vergleich '==' wäre ausreichend, '>=' nur zur Sicherheit.
    //    Diese Zeile ist nur wg. "extern" erzeugten Entities (new-joined clients) hier.
    if (BaseLineFrameNr>=ServerFrameNr) return;

    // 2. Alten 'Entity->State' des vorherigen (aber noch aktuellen!) Server-Frames erstmal speichern.
    *OldStates[(ServerFrameNr-1) & (OldStates.Size()-1)]=Entity->State;
}


void EngineEntityT::Think(float FrameTime, unsigned long ServerFrameNr)
{
    // x. Ein Entity, der für dieses zu erstellende Frame 'ServerFrameNr' erst neu erzeugt wurde, soll nicht gleich denken!
    //    Ein einfacher Vergleich '==' wäre ausreichend, '>=' nur zur Sicherheit.
    if (BaseLineFrameNr>=ServerFrameNr) return;

    // 3. Jetzt neuen 'Entity->State' ausdenken.
    Entity->Think(FrameTime, ServerFrameNr);
    EntityStateFrameNr=ServerFrameNr;
}


void EngineEntityT::WriteNewBaseLine(unsigned long SentClientBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const
{
    // Nur dann etwas tun, wenn unsere 'BaseLineFrameNr' größer (d.h. jünger) als 'SentClientBaseLineFrameNr' ist,
    // d.h. unsere 'BaseLineFrameNr' noch nie / noch nicht an den Client gesendet wurde.
    if (SentClientBaseLineFrameNr>=BaseLineFrameNr) return;


    /***************************************************************************************************************/
    /*** EntityStateT field changes FORCE protocol changes in ALL places that are marked *like this* (use grep)! ***/
    /***************************************************************************************************************/

    unsigned long  FieldMaskBaseLine1=0;    // general fields
    unsigned short FieldMaskBaseLine2=0;    // HaveAmmo fields
    unsigned long  FieldMaskBaseLine3=0;    // HaveAmmoInWeapons fields
    char           Nr;

    for (Nr=0; Nr<16; Nr++) if (BaseLine.HaveAmmo         [Nr]!=0) FieldMaskBaseLine2|=(unsigned short)(1L << Nr);
    for (Nr=0; Nr<32; Nr++) if (BaseLine.HaveAmmoInWeapons[Nr]!=0) FieldMaskBaseLine3|=(unsigned long )(1L << Nr);

    if (BaseLine.Origin.x           !=0) FieldMaskBaseLine1|=0x00000001;
    if (BaseLine.Origin.y           !=0) FieldMaskBaseLine1|=0x00000002;
    if (BaseLine.Origin.z           !=0) FieldMaskBaseLine1|=0x00000004;
    if (BaseLine.Velocity.x         !=0) FieldMaskBaseLine1|=0x00000008;
    if (BaseLine.Velocity.y         !=0) FieldMaskBaseLine1|=0x00000010;
    if (BaseLine.Velocity.z         !=0) FieldMaskBaseLine1|=0x00000020;
    if (BaseLine.Dimensions.Min.z   !=0) FieldMaskBaseLine1|=0x00000040;
    if (BaseLine.Dimensions.Max.z   !=0) FieldMaskBaseLine1|=0x00000080;
    if (BaseLine.Heading            !=0) FieldMaskBaseLine1|=0x00000100;
    if (BaseLine.Pitch              !=0) FieldMaskBaseLine1|=0x00000200;
    if (BaseLine.Bank               !=0) FieldMaskBaseLine1|=0x00000400;
    if (BaseLine.StateOfExistance   !=0) FieldMaskBaseLine1|=0x00000800;
    if (BaseLine.Flags              !=0) FieldMaskBaseLine1|=0x00001000;
    if (BaseLine.PlayerName[0]      !=0) FieldMaskBaseLine1|=0x00002000;
    if (BaseLine.ModelIndex         !=0) FieldMaskBaseLine1|=0x00004000;
    if (BaseLine.ModelSequNr        !=0) FieldMaskBaseLine1|=0x00008000;
    if (BaseLine.ModelFrameNr       !=0) FieldMaskBaseLine1|=0x00010000;
    if (BaseLine.Health             !=0) FieldMaskBaseLine1|=0x00020000;
    if (BaseLine.Armor              !=0) FieldMaskBaseLine1|=0x00040000;
    if (BaseLine.HaveItems          !=0) FieldMaskBaseLine1|=0x00080000;
    if (BaseLine.HaveWeapons        !=0) FieldMaskBaseLine1|=0x00100000;
    if (BaseLine.ActiveWeaponSlot   !=0) FieldMaskBaseLine1|=0x00200000;
    if (BaseLine.ActiveWeaponSequNr !=0) FieldMaskBaseLine1|=0x00400000;
    if (BaseLine.ActiveWeaponFrameNr!=0) FieldMaskBaseLine1|=0x00800000;
    if (BaseLine.Events             !=0) FieldMaskBaseLine1|=0x01000000;
    if (FieldMaskBaseLine2          !=0) FieldMaskBaseLine1|=0x02000000;
    if (FieldMaskBaseLine3          !=0) FieldMaskBaseLine1|=0x04000000;
    // Remember that bit 0x80000000 of FieldMaskBaseLine1 is used as "remove me!" bit in SC1_EntityUpdate messages.
    // Thus, we should never use this bit for other encoding, not even in SC1_EntityBaseLine messages.
    // TODO: Get rid of that special bit by introducing a SC1_EntityRemove message!


    NetDataT NewBaseLineMsg;

    NewBaseLineMsg.WriteByte(SC1_EntityBaseLine);
    NewBaseLineMsg.WriteLong(Entity->ID);
    NewBaseLineMsg.WriteLong(Entity->GetTypeNr());
    NewBaseLineMsg.WriteLong(Entity->WorldFileIndex);

    NewBaseLineMsg.WriteLong(FieldMaskBaseLine1);
    if (FieldMaskBaseLine1 & 0x02000000) NewBaseLineMsg.WriteWord  (FieldMaskBaseLine2);
    if (FieldMaskBaseLine1 & 0x04000000) NewBaseLineMsg.WriteLong  (FieldMaskBaseLine3);

    if (FieldMaskBaseLine1 & 0x00000001) NewBaseLineMsg.WriteFloat (float(BaseLine.Origin.x));
    if (FieldMaskBaseLine1 & 0x00000002) NewBaseLineMsg.WriteFloat (float(BaseLine.Origin.y));
    if (FieldMaskBaseLine1 & 0x00000004) NewBaseLineMsg.WriteFloat (float(BaseLine.Origin.z));
    if (FieldMaskBaseLine1 & 0x00000008) NewBaseLineMsg.WriteFloat (float(BaseLine.Velocity.x));
    if (FieldMaskBaseLine1 & 0x00000010) NewBaseLineMsg.WriteFloat (float(BaseLine.Velocity.y));
    if (FieldMaskBaseLine1 & 0x00000020) NewBaseLineMsg.WriteFloat (float(BaseLine.Velocity.z));
    if (FieldMaskBaseLine1 & 0x00000040) NewBaseLineMsg.WriteFloat (float(BaseLine.Dimensions.Min.z));
    if (FieldMaskBaseLine1 & 0x00000080) NewBaseLineMsg.WriteFloat (float(BaseLine.Dimensions.Max.z));
    if (FieldMaskBaseLine1 & 0x00000100) NewBaseLineMsg.WriteWord  (BaseLine.Heading);
    if (FieldMaskBaseLine1 & 0x00000200) NewBaseLineMsg.WriteWord  (BaseLine.Pitch);
    if (FieldMaskBaseLine1 & 0x00000400) NewBaseLineMsg.WriteWord  (BaseLine.Bank);
    if (FieldMaskBaseLine1 & 0x00000800) NewBaseLineMsg.WriteByte  (BaseLine.StateOfExistance);
    if (FieldMaskBaseLine1 & 0x00001000) NewBaseLineMsg.WriteByte  (BaseLine.Flags);
    if (FieldMaskBaseLine1 & 0x00002000) NewBaseLineMsg.WriteString(BaseLine.PlayerName);
    if (FieldMaskBaseLine1 & 0x00004000) NewBaseLineMsg.WriteByte  (BaseLine.ModelIndex);
    if (FieldMaskBaseLine1 & 0x00008000) NewBaseLineMsg.WriteByte  (BaseLine.ModelSequNr);
    if (FieldMaskBaseLine1 & 0x00010000) NewBaseLineMsg.WriteFloat (BaseLine.ModelFrameNr);
    if (FieldMaskBaseLine1 & 0x00020000) NewBaseLineMsg.WriteByte  (BaseLine.Health);
    if (FieldMaskBaseLine1 & 0x00040000) NewBaseLineMsg.WriteByte  (BaseLine.Armor);
    if (FieldMaskBaseLine1 & 0x00080000) NewBaseLineMsg.WriteLong  (BaseLine.HaveItems);
    if (FieldMaskBaseLine1 & 0x00100000) NewBaseLineMsg.WriteLong  (BaseLine.HaveWeapons);
    if (FieldMaskBaseLine1 & 0x00200000) NewBaseLineMsg.WriteByte  (BaseLine.ActiveWeaponSlot);
    if (FieldMaskBaseLine1 & 0x00400000) NewBaseLineMsg.WriteByte  (BaseLine.ActiveWeaponSequNr);
    if (FieldMaskBaseLine1 & 0x00800000) NewBaseLineMsg.WriteFloat (BaseLine.ActiveWeaponFrameNr);
    if (FieldMaskBaseLine1 & 0x01000000) NewBaseLineMsg.WriteLong  (BaseLine.Events);

    for (Nr=0; Nr<16; Nr++) if (FieldMaskBaseLine2 & (1L << Nr)) NewBaseLineMsg.WriteWord(BaseLine.HaveAmmo         [Nr]);
    for (Nr=0; Nr<32; Nr++) if (FieldMaskBaseLine3 & (1L << Nr)) NewBaseLineMsg.WriteByte(BaseLine.HaveAmmoInWeapons[Nr]);

    OutDatas.PushBack(NewBaseLineMsg.Data);
}


bool EngineEntityT::WriteDeltaEntity(bool SendFromBaseLine, unsigned long ClientFrameNr, NetDataT& OutData, bool ForceInfo) const
{
    // Prüfe, ob die Voraussetzungen für die Parameter (insb. 'ClientFrameNr') eingehalten werden.
    if (!SendFromBaseLine)
    {
        // EntityStateFrameNr wird in Think() gesetzt und ist gleich der ServerFrameNr!
        // Beachte: OldStates speichert die alten Zustände von ServerFrameNr-1 bis ServerFrameNr-16.
        const unsigned long FrameDiff=EntityStateFrameNr-ClientFrameNr;

        if (FrameDiff<1 || FrameDiff>OldStates.Size()) return false;
    }


    /***************************************************************************************************************/
    /*** EntityStateT field changes FORCE protocol changes in ALL places that are marked *like this* (use grep)! ***/
    /***************************************************************************************************************/

    const EntityStateT& FromState =SendFromBaseLine ? BaseLine : *OldStates[ClientFrameNr & (OldStates.Size()-1)];
    unsigned long       FieldMask1=0;
    unsigned short      FieldMask2=0;
    unsigned long       FieldMask3=0;
    char                Nr;

    for (Nr=0; Nr<16; Nr++) if (Entity->State.HaveAmmo         [Nr]!=FromState.HaveAmmo         [Nr]) FieldMask2|=(unsigned short)(1L << Nr);
    for (Nr=0; Nr<32; Nr++) if (Entity->State.HaveAmmoInWeapons[Nr]!=FromState.HaveAmmoInWeapons[Nr]) FieldMask3|=(unsigned long )(1L << Nr);

    if (Entity->State.Origin.x           !=FromState.Origin.x           ) FieldMask1|=0x00000001;
    if (Entity->State.Origin.y           !=FromState.Origin.y           ) FieldMask1|=0x00000002;
    if (Entity->State.Origin.z           !=FromState.Origin.z           ) FieldMask1|=0x00000004;
    if (Entity->State.Velocity.x         !=FromState.Velocity.x         ) FieldMask1|=0x00000008;
    if (Entity->State.Velocity.y         !=FromState.Velocity.y         ) FieldMask1|=0x00000010;
    if (Entity->State.Velocity.z         !=FromState.Velocity.z         ) FieldMask1|=0x00000020;
    if (Entity->State.Dimensions.Min.z   !=FromState.Dimensions.Min.z   ) FieldMask1|=0x00000040;
    if (Entity->State.Dimensions.Max.z   !=FromState.Dimensions.Max.z   ) FieldMask1|=0x00000080;
    if (Entity->State.Heading            !=FromState.Heading            ) FieldMask1|=0x00000100;
    if (Entity->State.Pitch              !=FromState.Pitch              ) FieldMask1|=0x00000200;
    if (Entity->State.Bank               !=FromState.Bank               ) FieldMask1|=0x00000400;
    if (Entity->State.StateOfExistance   !=FromState.StateOfExistance   ) FieldMask1|=0x00000800;
    if (Entity->State.Flags              !=FromState.Flags              ) FieldMask1|=0x00001000;
    // PlayerName
    if (Entity->State.ModelIndex         !=FromState.ModelIndex         ) FieldMask1|=0x00004000;
    if (Entity->State.ModelSequNr        !=FromState.ModelSequNr        ) FieldMask1|=0x00008000;
    if (Entity->State.ModelFrameNr       !=FromState.ModelFrameNr       ) FieldMask1|=0x00010000;
    if (Entity->State.Health             !=FromState.Health             ) FieldMask1|=0x00020000;
    if (Entity->State.Armor              !=FromState.Armor              ) FieldMask1|=0x00040000;
    if (Entity->State.HaveItems          !=FromState.HaveItems          ) FieldMask1|=0x00080000;
    if (Entity->State.HaveWeapons        !=FromState.HaveWeapons        ) FieldMask1|=0x00100000;
    if (Entity->State.ActiveWeaponSlot   !=FromState.ActiveWeaponSlot   ) FieldMask1|=0x00200000;
    if (Entity->State.ActiveWeaponSequNr !=FromState.ActiveWeaponSequNr ) FieldMask1|=0x00400000;
    if (Entity->State.ActiveWeaponFrameNr!=FromState.ActiveWeaponFrameNr) FieldMask1|=0x00800000;
    if (Entity->State.Events             !=FromState.Events             ) FieldMask1|=0x01000000;
    if (FieldMask2                       !=0                            ) FieldMask1|=0x02000000;
    if (FieldMask3                       !=0                            ) FieldMask1|=0x04000000;
    // Remember that bit 0x80000000 of FieldMaskBaseLine1 is used as "remove me!" bit in SC1_EntityUpdate messages.
    // Thus, we should never use this bit for other encoding.
    // TODO: Get rid of that special bit by introducing a SC1_EntityRemove message!

    if (FieldMask1==0 && !ForceInfo) return true;


    // Write the SC1_EntityUpdate message
    OutData.WriteByte(SC1_EntityUpdate);
    OutData.WriteLong(Entity->ID);

    OutData.WriteLong(FieldMask1);
    if (FieldMask1 & 0x02000000) OutData.WriteWord (FieldMask2);
    if (FieldMask1 & 0x04000000) OutData.WriteLong (FieldMask3);

    if (FieldMask1 & 0x00000001) OutData.WriteFloat(float(Entity->State.Origin.x));
    if (FieldMask1 & 0x00000002) OutData.WriteFloat(float(Entity->State.Origin.y));
    if (FieldMask1 & 0x00000004) OutData.WriteFloat(float(Entity->State.Origin.z));
    if (FieldMask1 & 0x00000008) OutData.WriteFloat(float(Entity->State.Velocity.x));
    if (FieldMask1 & 0x00000010) OutData.WriteFloat(float(Entity->State.Velocity.y));
    if (FieldMask1 & 0x00000020) OutData.WriteFloat(float(Entity->State.Velocity.z));
    if (FieldMask1 & 0x00000040) OutData.WriteFloat(float(Entity->State.Dimensions.Min.z));
    if (FieldMask1 & 0x00000080) OutData.WriteFloat(float(Entity->State.Dimensions.Max.z));
    if (FieldMask1 & 0x00000100) OutData.WriteWord (Entity->State.Heading);
    if (FieldMask1 & 0x00000200) OutData.WriteWord (Entity->State.Pitch);
    if (FieldMask1 & 0x00000400) OutData.WriteWord (Entity->State.Bank);
    if (FieldMask1 & 0x00000800) OutData.WriteByte (Entity->State.StateOfExistance);
    if (FieldMask1 & 0x00001000) OutData.WriteByte (Entity->State.Flags);
    // PlayerName
    if (FieldMask1 & 0x00004000) OutData.WriteByte (Entity->State.ModelIndex);
    if (FieldMask1 & 0x00008000) OutData.WriteByte (Entity->State.ModelSequNr);
    if (FieldMask1 & 0x00010000) OutData.WriteFloat(Entity->State.ModelFrameNr);
    if (FieldMask1 & 0x00020000) OutData.WriteByte (Entity->State.Health);
    if (FieldMask1 & 0x00040000) OutData.WriteByte (Entity->State.Armor);
    if (FieldMask1 & 0x00080000) OutData.WriteLong (Entity->State.HaveItems);
    if (FieldMask1 & 0x00100000) OutData.WriteLong (Entity->State.HaveWeapons);
    if (FieldMask1 & 0x00200000) OutData.WriteByte (Entity->State.ActiveWeaponSlot);
    if (FieldMask1 & 0x00400000) OutData.WriteByte (Entity->State.ActiveWeaponSequNr);
    if (FieldMask1 & 0x00800000) OutData.WriteFloat(Entity->State.ActiveWeaponFrameNr);
    if (FieldMask1 & 0x01000000) OutData.WriteLong (Entity->State.Events);

    for (Nr=0; Nr<16; Nr++) if (FieldMask2 & (1L << Nr)) OutData.WriteWord(Entity->State.HaveAmmo         [Nr]);
    for (Nr=0; Nr<32; Nr++) if (FieldMask3 & (1L << Nr)) OutData.WriteByte(Entity->State.HaveAmmoInWeapons[Nr]);

    return true;
}


#if !DEDICATED

/*******************/
/*** Client Side ***/
/*******************/


EngineEntityT::EngineEntityT(BaseEntityT* Entity_, NetDataT& InData)
    : Entity(Entity_),
      EntityStateFrameNr(0),
      BaseLine(Entity->State),          // Unnötig, aber leider unvermeidlich (kein Default-Konstruktor vorhanden)!
      BaseLineFrameNr(1234),
   // OldStates,
   // PlayerCommands,
      PredictedState(Entity->State),    // Unnötig, aber leider unvermeidlich (kein Default-Konstruktor vorhanden)!
      OldEvents(0),
      InterpolateState0(NULL),
      InterpolateTime0(0),
      InterpolateTime1(0)
{
    /***************************************************************************************************************/
    /*** EntityStateT field changes FORCE protocol changes in ALL places that are marked *like this* (use grep)! ***/
    /***************************************************************************************************************/

    unsigned long  FieldMaskBaseLine1=InData.ReadLong();
    unsigned short FieldMaskBaseLine2=(FieldMaskBaseLine1 & 0x02000000) ? InData.ReadWord () : 0;
    unsigned long  FieldMaskBaseLine3=(FieldMaskBaseLine1 & 0x04000000) ? InData.ReadLong () : 0;
    Entity->State.Origin.x           =(FieldMaskBaseLine1 & 0x00000001) ? InData.ReadFloat() : 0;
    Entity->State.Origin.y           =(FieldMaskBaseLine1 & 0x00000002) ? InData.ReadFloat() : 0;
    Entity->State.Origin.z           =(FieldMaskBaseLine1 & 0x00000004) ? InData.ReadFloat() : 0;
    Entity->State.Velocity.x         =(FieldMaskBaseLine1 & 0x00000008) ? InData.ReadFloat() : 0;
    Entity->State.Velocity.y         =(FieldMaskBaseLine1 & 0x00000010) ? InData.ReadFloat() : 0;
    Entity->State.Velocity.z         =(FieldMaskBaseLine1 & 0x00000020) ? InData.ReadFloat() : 0;
    Entity->State.Dimensions.Min.z   =(FieldMaskBaseLine1 & 0x00000040) ? InData.ReadFloat() : 0;
    Entity->State.Dimensions.Max.z   =(FieldMaskBaseLine1 & 0x00000080) ? InData.ReadFloat() : 0;
    Entity->State.Heading            =(FieldMaskBaseLine1 & 0x00000100) ? InData.ReadWord () : 0;
    Entity->State.Pitch              =(FieldMaskBaseLine1 & 0x00000200) ? InData.ReadWord () : 0;
    Entity->State.Bank               =(FieldMaskBaseLine1 & 0x00000400) ? InData.ReadWord () : 0;
    Entity->State.StateOfExistance   =(FieldMaskBaseLine1 & 0x00000800) ? InData.ReadByte () : 0;
    Entity->State.Flags              =(FieldMaskBaseLine1 & 0x00001000) ? InData.ReadByte () : 0;
    if (FieldMaskBaseLine1 & 0x00002000) Entity->ProcessConfigString(InData.ReadString(), "PlayerName");
    Entity->State.ModelIndex         =(FieldMaskBaseLine1 & 0x00004000) ? InData.ReadByte () : 0;
    Entity->State.ModelSequNr        =(FieldMaskBaseLine1 & 0x00008000) ? InData.ReadByte () : 0;
    Entity->State.ModelFrameNr       =(FieldMaskBaseLine1 & 0x00010000) ? InData.ReadFloat() : 0;
    Entity->State.Health             =(FieldMaskBaseLine1 & 0x00020000) ? InData.ReadByte () : 0;
    Entity->State.Armor              =(FieldMaskBaseLine1 & 0x00040000) ? InData.ReadByte () : 0;
    Entity->State.HaveItems          =(FieldMaskBaseLine1 & 0x00080000) ? InData.ReadLong () : 0;
    Entity->State.HaveWeapons        =(FieldMaskBaseLine1 & 0x00100000) ? InData.ReadLong () : 0;
    Entity->State.ActiveWeaponSlot   =(FieldMaskBaseLine1 & 0x00200000) ? InData.ReadByte () : 0;
    Entity->State.ActiveWeaponSequNr =(FieldMaskBaseLine1 & 0x00400000) ? InData.ReadByte () : 0;
    Entity->State.ActiveWeaponFrameNr=(FieldMaskBaseLine1 & 0x00800000) ? InData.ReadFloat() : 0;
    Entity->State.Events             =(FieldMaskBaseLine1 & 0x01000000) ? InData.ReadLong () : 0;

    char Nr;

    for (Nr=0; Nr<16; Nr++) Entity->State.HaveAmmo         [Nr]=(FieldMaskBaseLine2 & (1L << Nr)) ? InData.ReadWord() : 0;
    for (Nr=0; Nr<32; Nr++) Entity->State.HaveAmmoInWeapons[Nr]=(FieldMaskBaseLine3 & (1L << Nr)) ? InData.ReadByte() : 0;

    Entity->Cl_UnserializeFrom();   // A temp. hack to get the entities ClipModel origin updated.


    for (unsigned long OldStateNr=0; OldStateNr<32 /*MUST be a power of 2*/; OldStateNr++)
        OldStates.PushBack(new EntityStateT(Entity->State));

    PlayerCommands.PushBackEmpty(128);  // Achtung! Die Größe MUSS eine 2er-Potenz sein!

    BaseLine      =Entity->State;
    PredictedState=Entity->State;

    InterpolateTime1=GlobalTime.GetValueDouble();
}


bool EngineEntityT::ParseServerDeltaUpdateMessage(unsigned long DeltaFrameNr, unsigned long ServerFrameNr, unsigned long FieldMask1, NetDataT& InData)
{
    /***************************************************************************************************************/
    /*** EntityStateT field changes FORCE protocol changes in ALL places that are marked *like this* (use grep)! ***/
    /***************************************************************************************************************/

    // Es ist wichtig, daß wir zu allererst die 'InData' gemäß der 'FieldMask' zu Ende auslesen.
    // Nur so ist es sinnvoll möglich, z.B. bei Fehlern weiter unten diese Funktion konsistent abzubrechen!
    char Nr;

    unsigned short FieldMask2                =(FieldMask1 & 0x02000000) ? InData.ReadWord () : 0;
    unsigned long  FieldMask3                =(FieldMask1 & 0x04000000) ? InData.ReadLong () : 0;
    float          InData_Origin_x           =(FieldMask1 & 0x00000001) ? InData.ReadFloat() : 0;
    float          InData_Origin_y           =(FieldMask1 & 0x00000002) ? InData.ReadFloat() : 0;
    float          InData_Origin_z           =(FieldMask1 & 0x00000004) ? InData.ReadFloat() : 0;
    float          InData_Velocity_x         =(FieldMask1 & 0x00000008) ? InData.ReadFloat() : 0;
    float          InData_Velocity_y         =(FieldMask1 & 0x00000010) ? InData.ReadFloat() : 0;
    float          InData_Velocity_z         =(FieldMask1 & 0x00000020) ? InData.ReadFloat() : 0;
    float          InData_Dim_Min_z          =(FieldMask1 & 0x00000040) ? InData.ReadFloat() : 0;
    float          InData_Dim_Max_z          =(FieldMask1 & 0x00000080) ? InData.ReadFloat() : 0;
    unsigned short InData_Heading            =(FieldMask1 & 0x00000100) ? InData.ReadWord () : 0;
    unsigned short InData_Pitch              =(FieldMask1 & 0x00000200) ? InData.ReadWord () : 0;
    unsigned short InData_Bank               =(FieldMask1 & 0x00000400) ? InData.ReadWord () : 0;
    char           InData_StateOfExistance   =(FieldMask1 & 0x00000800) ? InData.ReadByte () : 0;
    char           InData_Flags              =(FieldMask1 & 0x00001000) ? InData.ReadByte () : 0;
    // PlayerName
    char           InData_ModelIndex         =(FieldMask1 & 0x00004000) ? InData.ReadByte () : 0;
    char           InData_ModelSequNr        =(FieldMask1 & 0x00008000) ? InData.ReadByte () : 0;
    float          InData_ModelFrameNr       =(FieldMask1 & 0x00010000) ? InData.ReadFloat() : 0;
    char           InData_Health             =(FieldMask1 & 0x00020000) ? InData.ReadByte () : 0;
    char           InData_Armor              =(FieldMask1 & 0x00040000) ? InData.ReadByte () : 0;
    unsigned long  InData_HaveItems          =(FieldMask1 & 0x00080000) ? InData.ReadLong () : 0;
    unsigned long  InData_HaveWeapons        =(FieldMask1 & 0x00100000) ? InData.ReadLong () : 0;
    char           InData_ActiveWeaponSlot   =(FieldMask1 & 0x00200000) ? InData.ReadByte () : 0;
    char           InData_ActiveWeaponSequNr =(FieldMask1 & 0x00400000) ? InData.ReadByte () : 0;
    float          InData_ActiveWeaponFrameNr=(FieldMask1 & 0x00800000) ? InData.ReadFloat() : 0;
    unsigned long  InData_Events             =(FieldMask1 & 0x01000000) ? InData.ReadLong () : 0;

    unsigned short InData_HaveAmmo         [16]; for (Nr=0; Nr<16; Nr++) InData_HaveAmmo         [Nr]=(FieldMask2 & (1L << Nr)) ? InData.ReadWord() : 0;
    char           InData_HaveAmmoInWeapons[32]; for (Nr=0; Nr<32; Nr++) InData_HaveAmmoInWeapons[Nr]=(FieldMask3 & (1L << Nr)) ? InData.ReadByte() : 0;


    // Sanity-Check: Wir wollen, daß 'DeltaFrameNr<=EntityStateFrameNr<ServerFrameNr' gilt.
    // Wäre 'DeltaFrameNr>EntityStateFrameNr', so sollten wir gegen einen State dekomprimieren, der in der Zukunft liegt.
    // Wäre 'EntityStateFrameNr>=ServerFrameNr', so sollten wir uns in einen State begeben, der schon Vergangenheit ist.
    // Dies hält auch für den Spezialfall 'DeltaFrameNr==0' (Delta-Dekompression gegen die BaseLine).
    // Im Normalfall 'DeltaFrameNr>0' müssen wir unten außerdem noch sicherstellen, daß der DeltaState nicht zu weit in der Vergangenheit liegt.
    //
    // ONE possible reason for DeltaFrameNr>EntityStateFrameNr is related to the way how baselines are sent,
    // see EntityManager.cpp, EntityManagerT::ParseServerDeltaUpdateMessage() for a description, which is essentially repeated here:
    // When a client joins a level, there can be a LOT of entities. Usually, not all baselines of all entities fit into a single
    // realiable message at once, and thus the server sends them in batches, contained in subsequent realiable messages.
    // Between realiable messages however, the server sends also SC1_EntityUpdate messages.
    // These messages can already refer to entities that the client knows nothing about, because it has not yet seen the (reliable)
    // introductory baseline message.
    // Then, the entities that the client already knows about normally receive and process delta updates here in this function,
    // the others don't (because their non-presence is already detected in EntityManagerT::ParseServerDeltaUpdateMessage()).
    // However, the frame counters increase normally, as if all entities were present. When finally the remaining entities
    // arrive (because their baseline got finally through), these entities are likely to have DeltaFrameNr>EntityStateFrameNr.
    // I turn the "WARNING" into an "INFO", so that ordinary users get a better impression. ;)
    if (DeltaFrameNr>EntityStateFrameNr)   { EnqueueString("CLIENT INFO: %s, L %u: DeltaFrameNr>EntityStateFrameNr (%lu>%lu)\n"  , __FILE__, __LINE__, DeltaFrameNr, EntityStateFrameNr); return false; }
    if (EntityStateFrameNr>=ServerFrameNr) { EnqueueString("CLIENT WARNING: %s, L %u: EntityStateFrameNr>=ServerFrameNr (%lu>%lu)\n", __FILE__, __LINE__, EntityStateFrameNr, ServerFrameNr); return false; }


    const EntityStateT* DeltaStatePtr=NULL;

    if (DeltaFrameNr>0)
    {
        // Normalfall: Delta-Dekomprimiere NICHT gegen die BaseLine, sondern gegen einen 'OldState'.
        if (DeltaFrameNr==EntityStateFrameNr)
        {
            // Der Delta-State ist der gegenwärtige Entity->State
            DeltaStatePtr=&Entity->State;
        }
        else
        {
            // Der oben angekündigte Test, ob der DeltaState nicht schon zu weit in der Vergangenheit liegt.
            // Einen gültigen State können wir dann nicht mehr produzieren, und dem Calling-Code muß klar sein oder klar werden,
            // daß er gegen die BaseLines komprimierte Messages anfordern muß.
            if (EntityStateFrameNr-DeltaFrameNr>OldStates.Size())
            {
                EnqueueString("CLIENT WARNING: %s, L %u: Delta state too old!\n", __FILE__, __LINE__);
                return false;
            }

            // Ordentliche Delta-Dekompression gegen einen OldState.
            DeltaStatePtr=OldStates[DeltaFrameNr & (OldStates.Size()-1)];
        }
    }
    else DeltaStatePtr=&BaseLine;   // Delta-Dekomprimiere gegen die BaseLine

    // Wir müssen den DeltaState zuerst aus den OldStates kopieren, denn es könnte ansonsten passieren,
    // daß im folgenden gerade dieser OldState überschrieben wird, bevor wir damit fertig sind!
    EntityStateT DeltaState(*DeltaStatePtr);

    // Trage den bisher aktuellen Entity->State in die OldStates ein, und ersetze Entity->State durch den DeltaState.
    *OldStates[EntityStateFrameNr & (OldStates.Size()-1)]=Entity->State;
    Entity->State=DeltaState;

    InterpolateState0=OldStates[EntityStateFrameNr & (OldStates.Size()-1)];
    InterpolateTime0 =InterpolateTime1;
    InterpolateTime1 =GlobalTime.GetValueDouble();

    if (length(Entity->State.Origin - InterpolateState0->Origin)/(InterpolateTime1-InterpolateTime0) > 50.0*1000.0)
    {
        // Don't interpolate if the theoretical speed is larger than 50 m/s, or 180 km/h.
        InterpolateState0=NULL;
    }


    // Beginne die eigentliche Delta-Dekomprimierung
    if (FieldMask1 & 0x00000001) Entity->State.Origin.x           =InData_Origin_x;
    if (FieldMask1 & 0x00000002) Entity->State.Origin.y           =InData_Origin_y;
    if (FieldMask1 & 0x00000004) Entity->State.Origin.z           =InData_Origin_z;
    if (FieldMask1 & 0x00000008) Entity->State.Velocity.x         =InData_Velocity_x;
    if (FieldMask1 & 0x00000010) Entity->State.Velocity.y         =InData_Velocity_y;
    if (FieldMask1 & 0x00000020) Entity->State.Velocity.z         =InData_Velocity_z;
    if (FieldMask1 & 0x00000040) Entity->State.Dimensions.Min.z   =InData_Dim_Min_z;
    if (FieldMask1 & 0x00000080) Entity->State.Dimensions.Max.z   =InData_Dim_Max_z;
    if (FieldMask1 & 0x00000100) Entity->State.Heading            =InData_Heading;
    if (FieldMask1 & 0x00000200) Entity->State.Pitch              =InData_Pitch;
    if (FieldMask1 & 0x00000400) Entity->State.Bank               =InData_Bank;
    if (FieldMask1 & 0x00000800) Entity->State.StateOfExistance   =InData_StateOfExistance;
    if (FieldMask1 & 0x00001000) Entity->State.Flags              =InData_Flags;
    // PlayerName
    if (FieldMask1 & 0x00004000) Entity->State.ModelIndex         =InData_ModelIndex;
    if (FieldMask1 & 0x00008000) Entity->State.ModelSequNr        =InData_ModelSequNr;
    if (FieldMask1 & 0x00010000) Entity->State.ModelFrameNr       =InData_ModelFrameNr;
    if (FieldMask1 & 0x00020000) Entity->State.Health             =InData_Health;
    if (FieldMask1 & 0x00040000) Entity->State.Armor              =InData_Armor;
    if (FieldMask1 & 0x00080000) Entity->State.HaveItems          =InData_HaveItems;
    if (FieldMask1 & 0x00100000) Entity->State.HaveWeapons        =InData_HaveWeapons;
    if (FieldMask1 & 0x00200000) Entity->State.ActiveWeaponSlot   =InData_ActiveWeaponSlot;
    if (FieldMask1 & 0x00400000) Entity->State.ActiveWeaponSequNr =InData_ActiveWeaponSequNr;
    if (FieldMask1 & 0x00800000) Entity->State.ActiveWeaponFrameNr=InData_ActiveWeaponFrameNr;
    if (FieldMask1 & 0x01000000) Entity->State.Events             =InData_Events;

    for (Nr=0; Nr<16; Nr++) if (FieldMask2 & (1L << Nr)) Entity->State.HaveAmmo         [Nr]=InData_HaveAmmo         [Nr];
    for (Nr=0; Nr<32; Nr++) if (FieldMask3 & (1L << Nr)) Entity->State.HaveAmmoInWeapons[Nr]=InData_HaveAmmoInWeapons[Nr];

    Entity->Cl_UnserializeFrom();   // A temp. hack to get the entities ClipModel origin updated.
    EntityStateFrameNr=ServerFrameNr;
    return true;
}


bool EngineEntityT::Repredict(unsigned long RemoteLastIncomingSequenceNr, unsigned long LastOutgoingSequenceNr)
{
    if (LastOutgoingSequenceNr-RemoteLastIncomingSequenceNr>PlayerCommands.Size())
    {
        EnqueueString("WARNING - Prediction impossible: Last ack'ed PlayerCommand is too old (%u, %u)!\n", RemoteLastIncomingSequenceNr, LastOutgoingSequenceNr);
        return false;
    }

    const EntityStateT BackupState=Entity->State;

    // Unseren Entity über alle relevanten (d.h. noch nicht bestätigten) PlayerCommands unterrichten.
    // Wenn wir auf dem selben Host laufen wie der Server (z.B. Single-Player Spiel oder lokaler Client bei non-dedicated-Server Spiel),
    // werden die Netzwerk-Nachrichten in Nullzeit (im Idealfall über Memory-Buffer) versandt.
    // Falls dann auch noch der Server mit full-speed läuft, sollte daher immer RemoteLastIncomingSequenceNr==LastOutgoingSequenceNr sein,
    // was impliziert, daß dann keine Prediction stattfindet (da nicht notwendig!).
    for (unsigned long SequenceNr=RemoteLastIncomingSequenceNr+1; SequenceNr<=LastOutgoingSequenceNr; SequenceNr++)
        Entity->ProcessConfigString(&PlayerCommands[SequenceNr & (PlayerCommands.Size()-1)], "PlayerCommand");
    Entity->Think(-2.0, 0);

    PredictedState=Entity->State;
    Entity->State=BackupState;
    Entity->Cl_UnserializeFrom();   // A temp. hack to get the entities ClipModel origin updated.

    return true;
}


void EngineEntityT::Predict(const PlayerCommandT& PlayerCommand, unsigned long OutgoingSequenceNr)
{
    // PlayerCommand für die Reprediction speichern
    PlayerCommands[OutgoingSequenceNr & (PlayerCommands.Size()-1)]=PlayerCommand;

    const EntityStateT BackupState=Entity->State;
    Entity->State=PredictedState;
    Entity->Cl_UnserializeFrom();   // A temp. hack to get the entities ClipModel origin updated.

    Entity->ProcessConfigString(&PlayerCommand, "PlayerCommand");
    Entity->Think(-1.0, 0);

    PredictedState=Entity->State;
    Entity->State=BackupState;
    Entity->Cl_UnserializeFrom();   // A temp. hack to get the entities ClipModel origin updated.
}


const EntityStateT* EngineEntityT::GetPredictedState()
{
    return &PredictedState;
}


static ConVarT interpolateNPCs("interpolateNPCs", true, ConVarT::FLAG_MAIN_EXE, "Toggles whether the origin of NPCs is interpolated for rendering.");


bool EngineEntityT::GetLightSourceInfo(bool UsePredictedState, unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    if (UsePredictedState)
    {
        // It's the local predicted human player entity.
        #if 0
            const EntityStateT BackupState(Entity->State);
            Entity->State=PredictedState;
        #else
            // Be lazy and only correct for the predicted origin, not everything else in the state, too.
            const Vector3dT BackupOrigin(Entity->State.Origin);
            Entity->State.Origin=PredictedState.Origin;
        #endif

        const bool Result=Entity->GetLightSourceInfo(DiffuseColor, SpecularColor, Position, Radius, CastsShadows);

        #if 0
            Entity->State=BackupState;
        #else
            Entity->State.Origin=BackupOrigin;
        #endif

        return Result;
    }
    else
    {
        // It's a non-predicted NPC entity.
        const Vector3dT BackupOrigin(Entity->State.Origin);

        if (InterpolateState0!=NULL && interpolateNPCs.GetValueBool() && Entity->DrawInterpolated())
        {
            const double dt=InterpolateTime1-InterpolateTime0;
            const double f =(dt>0) ? (GlobalTime.GetValueDouble()-InterpolateTime1)/dt : 1.0;

            Entity->State.Origin=InterpolateState0->Origin*(1.0-f) + Entity->State.Origin*f;
        }

        const bool Result=Entity->GetLightSourceInfo(DiffuseColor, SpecularColor, Position, Radius, CastsShadows);

        Entity->State.Origin=BackupOrigin;

        return Result;
    }
}


void EngineEntityT::Draw(bool FirstPersonView, bool UsePredictedState, const VectorT& ViewerPos) const
{
    const EntityStateT BackupState(Entity->State);

    if (UsePredictedState)
    {
        Entity->State=PredictedState;
    }
    else
    {
        if (InterpolateState0!=NULL && interpolateNPCs.GetValueBool() && Entity->DrawInterpolated())
        {
            const double dt=InterpolateTime1-InterpolateTime0;
            const double f =(dt>0) ? (GlobalTime.GetValueDouble()-InterpolateTime1)/dt : 1.0;

            // if (Entity->State.Origin.z>10000)   // Only print information for the eagle in BpRockB.
            // {
            //     std::cout << "Values: t0: " << InterpolateTime0 << " t1: " << InterpolateTime1 << " globalt: " << GlobalTime.GetValueDouble() << " f: " << f << " ";
            //     std::cout << "ohne Interp: " << Entity->State.Origin << "   mit Interp: " << InterpolateState0->Origin*(1.0-f) + Entity->State.Origin*f << "\n";
            // }

            Entity->State.Origin=InterpolateState0->Origin*(1.0-f) + Entity->State.Origin*f;
        }
    }


    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushLightingParameters();

    // Get the currently set lighting parameters.
    const float* PosL=MatSys::Renderer->GetCurrentLightSourcePosition();
    VectorT      LightSourcePos   =VectorT(PosL[0], PosL[1], PosL[2]);
    float        LightSourceRadius=MatSys::Renderer->GetCurrentLightSourceRadius();

    const float* PosE=MatSys::Renderer->GetCurrentEyePosition();
    VectorT      EyePos=VectorT(PosE[0], PosE[1], PosE[2]);


    // Starting from world space, compute the position of the light source in model space.
    // IMPORTANT NOTE:
    // The lighting info we get here may or may not be from the local players entity.
    // However, we currently always get the *UNPREDICTED* position of the light source,
    // no matter whether it comes from the local predicted entity or not.
    // (And it probably doesn't make much sense to try to get the predicted position instead.)
    // Consequently, we ALWAYS have to compute the relative light source position (i.e. the model space position)
    // wrt. the UNPREDICTED entity, even if we're actually drawing the predicted entity of the local player.
    LightSourcePos=LightSourcePos-Entity->State.Origin;         // Convert into unrotated model space.
    LightSourcePos=LightSourcePos.GetRotZ(-90.0+float(Entity->State.Heading)/8192.0*45.0);
    LightSourcePos=scale(LightSourcePos, 1.0/25.4);

    // Don't forget to scale the radius of the light source appropriately down (into model space), too.
    LightSourceRadius/=25.4f;


    // Do the same for the eye: Starting from world space, compute the position of the eye in model space.
    // IMPORTANT NOTE:
    // Contrary to the above, the eye position *IS* predicted.
    // I have not *fully* thought through all ramifications, but I think in general it is fine to compute this relative to the unpredicted positions.
    // The ONLY BAD CASE probably occurs if this entity is "our" entity: The local "view" weapon model might show the typical stepping behaviour
    // in specular highlights that occurs when predicted and non-predicted positions meet.
    // In this case should EyePos=PredictedOrig-PredictedOrig=(0, 0, 0).
    // Idea: Instead of detecting *here* if this entity is "our" entity and then replace Entity->State.Origin with the predicted Origin,
    // we might also detect this in EntityManager.cpp, and hand-in the unpredicted(!) position if this is "our" entity.
    // Then we had the same result: EyePos=UnPredictedOrig-UnPredictedOrig=(0, 0, 0).
    // On the other hand: We have UsePredictedState conveniently passed as parameter already, so why not make use of it?!
    EyePos=EyePos-Entity->State.Origin;         // Convert into unrotated model space.
    EyePos=EyePos.GetRotZ(-90.0+float(Entity->State.Heading)/8192.0*45.0);
    EyePos=scale(EyePos, 1.0/25.4);


    // Set the modified (now in model space) lighting parameters.
    MatSys::Renderer->SetCurrentLightSourcePosition(float(LightSourcePos.x), float(LightSourcePos.y), float(LightSourcePos.z));
    MatSys::Renderer->SetCurrentLightSourceRadius(LightSourceRadius);
    MatSys::Renderer->SetCurrentEyePosition(float(EyePos.x), float(EyePos.y), float(EyePos.z));


    // Set the ambient light color for this entity.
    // Paradoxically, this is not a global, but rather a per-entity value that is derived from the lightmaps that are close to that entity.
    const Vector3fT AmbientEntityLight=Entity->GameWorld->GetAmbientLightColorFromBB(Entity->State.Dimensions, Entity->State.Origin);
    MatSys::Renderer->SetCurrentAmbientLightColor(AmbientEntityLight.x, AmbientEntityLight.y, AmbientEntityLight.z);


    MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, float(Entity->State.Origin.x), float(Entity->State.Origin.y), float(Entity->State.Origin.z));
    MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD, 90.0f-float(Entity->State.Heading)/8192.0f*45.0f);
    MatSys::Renderer->Scale    (MatSys::RendererI::MODEL_TO_WORLD, 25.4f);

    Entity->Draw(FirstPersonView, (float)length(ViewerPos-Entity->State.Origin));


    MatSys::Renderer->PopLightingParameters();
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);


#if 1
    Entity->State=BackupState;
#else
    if (UsePredictedState)
    {
        Entity->State=BackupState;
    }
    else
    {
        if (InterpolateState0!=NULL && interpolateNPCs.GetValueBool() && Entity->DrawInterpolated())
        {
            Entity->State.Origin=BackupState.Origin;
        }
    }
#endif
}


void EngineEntityT::PostDraw(float FrameTime, bool FirstPersonView, bool UsePredictedState)
{
    if (UsePredictedState)
    {
        const EntityStateT BackupState(Entity->State);
        Entity->State=PredictedState;

        // Code duplicated below!
        // TODO: Event processing works fine and conveniently here,
        // but should probably be moved into some 'PreDraw()' or 'ProcessEvents()' method...
        unsigned long Events=Entity->State.Events ^ OldEvents;

        for (char b=0; Events!=0; Events >>= 1, b++)
            if (Events & 1) Entity->ProcessEvent(b);

        OldEvents=Entity->State.Events;

        Entity->PostDraw(FrameTime, FirstPersonView);

        Entity->State=BackupState;
    }
    else
    {
        // Code duplicated above!
        // TODO: Event processing works fine and conveniently here,
        // but should probably be moved into some 'PreDraw()' or 'ProcessEvents()' method...
        unsigned long Events=Entity->State.Events ^ OldEvents;

        for (char b=0; Events!=0; Events >>= 1, b++)
            if (Events & 1) Entity->ProcessEvent(b);

        OldEvents=Entity->State.Events;

        Entity->PostDraw(FrameTime, FirstPersonView);
    }
}

#endif   /* !DEDICATED */
