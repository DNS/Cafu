/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ConVar.hpp"
#include "ConsoleInterpreter.hpp"

#include <cassert>
#include <fstream>


// This functions returns a static list where all ConVarT objects that are instantiated before the call to ConVarT::RegisterStaticList()
// register themselves (this is done by their ctors), waiting to be registered with the ConsoleInterpreter.
// Usually only globally declared ConVarTs get into this list.
//
// Note that we can *NOT* simply have a   static ArrayT<ConVarT*> StaticList;   here (without the function around it), because the StaticList
// is then just another global variable (as the ConVarT objects), and it seems that when a ctor of a global ConVarT object is run, it can
// happen that the ctor of the StaticList has NOT YET BEEN RUN, so this is a (nontrivial) instance of the "static initialization order problem",
// fixed by the wrapper function. Note that the deinitialization order is not a problem here.
static ArrayT<ConVarT*>& GetStaticList()
{
    static ArrayT<ConVarT*> StaticList;

    return StaticList;
}


void ConVarT::RegisterStaticList()
{
    assert(ConsoleInterpreter!=NULL);

    // Well, under Linux it seems that it is possible that this method is called multiple times per loaded (game) DLL...
    // This is probably because Windows DLLs get their reference count increased and their global data *duplicated* when being loaded multiply,
    // whereas under Linux, loading a DLL is more like statically linking a ".a" library - there is only *one* instance of the global data.
 // assert(!(GetStaticList().Size()>0 && GetStaticList()[0]==NULL));
    if (GetStaticList().Size()>0 && GetStaticList()[0]==NULL) return;

    for (unsigned long ConVarNr=0; ConVarNr<GetStaticList().Size(); ConVarNr++)
        ConsoleInterpreter->Register(GetStaticList()[ConVarNr]);

    // The sole purpose of the list is fulfilled, mark it appropriately.
    GetStaticList().Clear();
    GetStaticList().PushBack(NULL);
}


// This is just a helper method for the constructor below.
static ArrayT<std::string> ConvertStringList(const char** Strings)
{
    ArrayT<std::string> List;

    if (Strings==NULL) return List;

    for (unsigned long Nr=0; Strings[Nr]!=NULL; Nr++)
        List.PushBack(Strings[Nr]);

    return List;
}


ConVarT::ConVarT(const std::string& Name_, const std::string& Value_, const int Flags_, const std::string& Description_, const char** AllowedValues_)
    : Name(Name_),
      Description(Description_),
      Flags(Flags_ | FLAG_BY_CODE),
      Type(String),
      ValueString(Value_),
      ValueInt(0),
      ValueDouble(0.0),
      MinValue(0.0),
      MaxValue(0.0),
      AllowedValues(ConvertStringList(AllowedValues_)),
      IsModified(false)
{
    if (GetStaticList().Size()>0 && GetStaticList()[0]==NULL)
    {
        // The StaticList is already in the state that indicates that the ConsoleInterpreter is available now
        // and its members have already been registered with it. So register this ConFuncT directly, too.
        assert(ConsoleInterpreter!=NULL);
        ConsoleInterpreter->Register(this);
    }
    else
    {
        // RegisterStaticList() has not yet been called, so add this ConFuncT to the waiting list for later registration.
        GetStaticList().PushBack(this);
    }
}


// This ctor is identical to the one above, except for the type of the Value_ parameter.
ConVarT::ConVarT(const std::string& Name_, const char* Value_, const int Flags_, const std::string& Description_, const char** AllowedValues_)
    : Name(Name_),
      Description(Description_),
      Flags(Flags_ | FLAG_BY_CODE),
      Type(String),
      ValueString(Value_),
      ValueInt(0),
      ValueDouble(0.0),
      MinValue(0.0),
      MaxValue(0.0),
      AllowedValues(ConvertStringList(AllowedValues_)),
      IsModified(false)
{
    if (GetStaticList().Size()>0 && GetStaticList()[0]==NULL)
    {
        // The StaticList is already in the state that indicates that the ConsoleInterpreter is available now
        // and its members have already been registered with it. So register this ConFuncT directly, too.
        assert(ConsoleInterpreter!=NULL);
        ConsoleInterpreter->Register(this);
    }
    else
    {
        // RegisterStaticList() has not yet been called, so add this ConFuncT to the waiting list for later registration.
        GetStaticList().PushBack(this);
    }
}


ConVarT::ConVarT(const std::string& Name_, const int Value_, const int Flags_, const std::string& Description_, const int MinValue_, const int MaxValue_)
    : Name(Name_),
      Description(Description_),
      Flags(Flags_ | FLAG_BY_CODE),
      Type(Integer),
      ValueString(""),
      ValueInt(Value_),
      ValueDouble(0.0),
      MinValue(MinValue_),
      MaxValue(MaxValue_),
      AllowedValues(),
      IsModified(false)
{
    if (GetStaticList().Size()>0 && GetStaticList()[0]==NULL)
    {
        // The StaticList is already in the state that indicates that the ConsoleInterpreter is available now
        // and its members have already been registered with it. So register this ConFuncT directly, too.
        assert(ConsoleInterpreter!=NULL);
        ConsoleInterpreter->Register(this);
    }
    else
    {
        // RegisterStaticList() has not yet been called, so add this ConFuncT to the waiting list for later registration.
        GetStaticList().PushBack(this);
    }
}


ConVarT::ConVarT(const std::string& Name_, const bool Value_, const int Flags_, const std::string& Description_)
    : Name(Name_),
      Description(Description_),
      Flags(Flags_ | FLAG_BY_CODE),
      Type(Bool),
      ValueString(""),
      ValueInt(Value_),
      ValueDouble(0.0),
      MinValue(0.0),
      MaxValue(0.0),
      AllowedValues(),
      IsModified(false)
{
    if (GetStaticList().Size()>0 && GetStaticList()[0]==NULL)
    {
        // The StaticList is already in the state that indicates that the ConsoleInterpreter is available now
        // and its members have already been registered with it. So register this ConFuncT directly, too.
        assert(ConsoleInterpreter!=NULL);
        ConsoleInterpreter->Register(this);
    }
    else
    {
        // RegisterStaticList() has not yet been called, so add this ConFuncT to the waiting list for later registration.
        GetStaticList().PushBack(this);
    }
}


ConVarT::ConVarT(const std::string& Name_, const double Value_, const int Flags_, const std::string& Description_, const double MinValue_, const double MaxValue_)
    : Name(Name_),
      Description(Description_),
      Flags(Flags_ | FLAG_BY_CODE),
      Type(Double),
      ValueString(""),
      ValueInt(0),
      ValueDouble(Value_),
      MinValue(MinValue_),
      MaxValue(MaxValue_),
      AllowedValues(),
      IsModified(false)
{
    if (GetStaticList().Size()>0 && GetStaticList()[0]==NULL)
    {
        // The StaticList is already in the state that indicates that the ConsoleInterpreter is available now
        // and its members have already been registered with it. So register this ConFuncT directly, too.
        assert(ConsoleInterpreter!=NULL);
        ConsoleInterpreter->Register(this);
    }
    else
    {
        // RegisterStaticList() has not yet been called, so add this ConFuncT to the waiting list for later registration.
        GetStaticList().PushBack(this);
    }
}


ConVarT::~ConVarT()
{
    if (Flags & FLAG_PERSISTENT)
    {
        std::ofstream CfgFile("config_p.lua", std::ios::app);

        if (!CfgFile.bad())
        {
            CfgFile << Name << " = ";

            switch (Type)
            {
                case String:  CfgFile << "\"" << ValueString << "\""; break;
                case Integer: CfgFile << ValueInt; break;
                case Bool:    CfgFile << ((ValueInt!=0) ? "true" : "false"); break;
                case Double:  CfgFile << ValueDouble; break;
            }

            CfgFile << "\n";
        }
    }

    // Notify the ConsoleInterpreter that we're going down, and e.g. the completion callback is thus not longer available.
    // Note that it is possible that ConsoleInterpreter==NULL here, because when it as well as this ConVarT was a static variable,
    // it usually is unclear who is destructed before whom.
    if (ConsoleInterpreter!=NULL) ConsoleInterpreter->Unregister(this);
}


void ConVarT::SetValue(const std::string& Value_)
{
    assert(Type==String);

    if (AllowedValues.Size()>0)
    {
        unsigned long Nr;

        for (Nr=0; Nr<AllowedValues.Size(); Nr++)
            if (Value_==AllowedValues[Nr]) break;

        if (Nr>=AllowedValues.Size()) return;
    }

    // If the value remains the same, return (don't set the IsModified flag).
    if (ValueString==Value_) return;

    ValueString=Value_;
    IsModified=true;
}


void ConVarT::SetValue(const char* Value_)
{
    const std::string Str=Value_ ? Value_ : "NULL";

    SetValue(Str);
}


void ConVarT::SetValue(const int Value_)
{
    assert(Type==Integer);

    if (MinValue<=MaxValue)
    {
        // If we have a valid min-max range, and the value is outside it, return.
        if (double(Value_)<MinValue) return;
        if (double(Value_)>MaxValue) return;
    }

    // If the value remains the same, return (don't set the IsModified flag).
    if (ValueInt==Value_) return;

    ValueInt=Value_;
    IsModified=true;
}


void ConVarT::SetValue(const bool Value_)
{
    assert(Type==Bool);

    // If the value remains the same, return (don't set the IsModified flag).
    if ((ValueInt!=0)==Value_) return;

    ValueInt=Value_;
    IsModified=true;
}


void ConVarT::SetValue(const double Value_)
{
    assert(Type==Double);

    if (MinValue<=MaxValue)
    {
        // If we have a valid min-max range, and the value is outside it, return.
        if (Value_<MinValue) return;
        if (Value_>MaxValue) return;
    }

    // If the value remains the same, return (don't set the IsModified flag).
    if (ValueDouble==Value_) return;

    ValueDouble=Value_;
    IsModified=true;
}
