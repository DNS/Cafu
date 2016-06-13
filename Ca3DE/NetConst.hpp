/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*****************************/
/*** Net Message Constants ***/
/*****************************/


// Dieses Header-File definiert die Konstanten für die Kommunikation zwischen Server und Client.
// Die 0 wurde absichtlich freigehalten, um sie später evtl. als ErrorFlag verwenden zu können.

// 'connection-less' Message-Types vom Client zum Server
const char CS0_NoOperation         =1;
const char CS0_Ping                =2;
const char CS0_Connect             =3;
const char CS0_Info                =4;
const char CS0_RemoteConsoleCommand=5;  ///< A message consisting of a password and a command string that is to be executed by the server console interpreter.

// 'connection-less' Message-Types vom Server zum Client
const char SC0_ACK           =1;
const char SC0_NACK          =2;
const char SC0_RccReply      =3;        ///< String reply to a CS0_RemoteConsoleCommand message.

// 'connection-established' Message-Types vom Client zum Server
const char CS1_PlayerCommand =1;
const char CS1_Disconnect    =2;
const char CS1_SayToAll      =3;
const char CS1_WorldInfoACK  =4;
const char CS1_FrameInfoACK  =5;

// 'connection-established' Message-Types vom Server zum Client
const char SC1_WorldInfo     =1;
const char SC1_EntityBaseLine=2;
const char SC1_FrameInfo     =3;
const char SC1_EntityUpdate  =4;
const char SC1_EntityRemove  =5;    ///< A special case of the SC1_EntityUpdate message: No data follows, remove the entity from the frame instead.
const char SC1_DropClient    =6;
const char SC1_ChatMsg       =7;
