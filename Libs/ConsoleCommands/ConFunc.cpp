/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ConFunc.hpp"
#include "ConsoleInterpreter.hpp"

#include <cassert>


// This functions returns a static list where all ConFuncT objects that are instantiated before the call to ConFuncT::RegisterStaticList()
// register themselves (this is done by their ctors), waiting to be registered with the ConsoleInterpreter.
// Usually only globally declared ConFuncTs get into this list.
//
// Note that we can *NOT* simply have a   static ArrayT<ConFuncT*> StaticList;   here (without the function around it), because the StaticList
// is then just another global variable (as the ConFuncT objects), and it seems that when a ctor of a global ConFuncT object is run, it can
// happen that the ctor of the StaticList has NOT YET BEEN RUN, so this is a (nontrivial) instance of the "static initialization order problem",
// fixed by the wrapper function. Note that the deinitialization order is not a problem here.
static ArrayT<ConFuncT*>& GetStaticList()
{
    static ArrayT<ConFuncT*> StaticList;

    return StaticList;
}


void ConFuncT::RegisterStaticList()
{
    assert(ConsoleInterpreter!=NULL);
    assert(!(GetStaticList().Size()>0 && GetStaticList()[0]==NULL));

    for (unsigned long FuncNr=0; FuncNr<GetStaticList().Size(); FuncNr++)
        ConsoleInterpreter->Register(GetStaticList()[FuncNr]);

    // The sole purpose of the list is fulfilled, mark it appropriately.
    GetStaticList().Clear();
    GetStaticList().PushBack(NULL);
}


ConFuncT::ConFuncT(const std::string& Name_, lua_CFunction LuaCFunction_, const unsigned long Flags_, const std::string& Description_)
    : Name(Name_),
      Description(Description_),
      Flags(Flags_),
      LuaCFunction(LuaCFunction_)
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


ConFuncT::~ConFuncT()
{
    // Notify the ConsoleInterpreter that we're going down, and the acutal LuaCFunction is thus not longer available.
    // Note that it is possible that ConsoleInterpreter==NULL here, because when it as well as this ConFuncT was a static variable,
    // it usually is unclear who is destructed before whom.
    if (ConsoleInterpreter!=NULL) ConsoleInterpreter->Unregister(this);
}
