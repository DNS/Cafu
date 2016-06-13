/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "TypeSys.hpp"
#include "ConsoleCommands/Console.hpp"

#include <cassert>
#include <cstring>


using namespace cf::TypeSys;


/*****************/
/*** TypeInfoT ***/
/*****************/

TypeInfoT::TypeInfoT(TypeInfoManT& TIM, const char* ClassName_, const char* BaseClassName_, CreateInstanceT CreateInstance_, const luaL_Reg MethodsList_[],
                     const char* DocClass_, const MethsDocT DocMethods_[], const MethsDocT DocCallbacks_[], const VarsDocT DocVars_[])
    : ClassName(ClassName_),
      BaseClassName(BaseClassName_),
      CreateInstance(CreateInstance_),
      MethodsList(MethodsList_),
      Base(NULL),
      Sibling(NULL),
      Child(NULL),
      TypeNr(0),
      LastChildNr(0),
      DocClass(DocClass_),
      DocMethods(DocMethods_),
      DocCallbacks(DocCallbacks_),
      DocVars(DocVars_)
{
    assert(ClassName_!=NULL);                               // Is not NULL.
    assert(ClassName_[0]!=0);                               // Is not the empty string ("").
    assert(BaseClassName_==NULL || BaseClassName_[0]!=0);   // Is NULL or a "proper" (non-empty) string.
    assert(CreateInstance!=NULL);                           // Is not NULL.

    TIM.Register(this);
}


void TypeInfoT::Print(bool Recurse, unsigned long RecDepth) const
{
    const std::string BaseClassString=(BaseClassName!=NULL) ? std::string(" --> ")+BaseClassName : "";

    Console->Print(std::string(RecDepth, ' ')+cf::va("%2lu ", TypeNr)+ClassName+BaseClassString+cf::va(", %2lu", LastChildNr)+"\n");

    if (!Recurse) return;

    if (Child!=NULL)
    {
        assert(Child->Base==this);      // We are the base class of our children.
        Child->Print(true, RecDepth+4);
    }

    if (Sibling!=NULL)
    {
        assert(Sibling->Base==Base);    // All our siblings have the same base.
        Sibling->Print(true, RecDepth);
    }
}


bool TypeInfoT::HierarchyHas(const TypeInfoT* Other) const
{
    while (Other)
    {
        if (Other==this) return true;
        Other=Other->Base;
    }

    return false;
}


const TypeInfoT* TypeInfoT::GetNext() const
{
    if (Child) return Child;

    // Move up in the hierarchy until we find siblings (or reach the root).
    const TypeInfoT* TI=this;

    while (TI)
    {
        if (TI->Sibling) return TI->Sibling;
        TI=TI->Base;
    }

    return NULL;
}


/********************/
/*** TypeInfoManT ***/
/********************/

TypeInfoManT::TypeInfoManT()
    : IsInited(false)
{
}


void TypeInfoManT::Register(TypeInfoT* TI)
{
    assert(!IsInited);

    // Just add TI to the list, but don't bother to sort it alphabetically right now (this is done later in Init()).
    TypeInfosByName.PushBack(TI);
}


static bool AlphabeticalCompare(const TypeInfoT* const& TI1, const TypeInfoT* const& TI2)
{
    return strcmp(TI1->ClassName, TI2->ClassName)<0;
}


// The implementation of this method uses const_cast<>() pretty frequently, as it is the nature of an Init() method to manipulate things.
// const_cast<>() should not occur anywhere else, because when Init() is finished,
// the type infos should not and cannot change any more, and then are "really" const.
void TypeInfoManT::Init()
{
    // Really cannot call this twice without zeroing many data members first (both of the TypeInfoTs and this TypeInfoManT).
    assert(!IsInited);
    if (IsInited) return;

    // First of all, establish an alphabetical order.
    // This allows us to use the FindTypeInfoByName() method afterwards.
    // This is also the key step that makes our numbering system below "robust", i.e. independent of the actual static initialization order.
    TypeInfosByName.QuickSort(AlphabeticalCompare);


    // Build (re-model) the C++ class hierarchy.
    for (unsigned long TypeNr=0; TypeNr<TypeInfosByName.Size(); TypeNr++)
    {
        const TypeInfoT* TI=TypeInfosByName[TypeNr];

        assert(TI->BaseClassName==NULL || strcmp(TI->ClassName, TI->BaseClassName)!=0);     // assert(ClassName!=BaseClassName);

#ifdef DEBUG
        IsInited=true;  // Have the assert() in FindTypeInfoByName() not trigger.
        const_cast<TypeInfoT*>(TI)->Base=FindTypeInfoByName(TI->BaseClassName);
        IsInited=false;
#else
        const_cast<TypeInfoT*>(TI)->Base=FindTypeInfoByName(TI->BaseClassName);
#endif

        // Assert that if we have a valid BaseClassName, the type info for the base class has been found.
        assert((TI->BaseClassName!=NULL && TI->Base!=NULL) || (TI->BaseClassName==NULL && TI->Base==NULL));

        if (TI->Base)
        {
            // This is not a root node, so add it into the hierarchy.
            const_cast<TypeInfoT*>(TI)->Sibling    =TI->Base->Child;
            const_cast<TypeInfoT*>(TI->Base)->Child=TI;
        }
        else TypeInfoRoots.PushBack(TI);
    }


    // For each node (class), determine the number of children in the entire subhierarchy.
    for (unsigned long TypeNr=0; TypeNr<TypeInfosByName.Size(); TypeNr++)
    {
        for (const TypeInfoT* TI=TypeInfosByName[TypeNr]->Base; TI!=NULL; TI=TI->Base)
            const_cast<TypeInfoT*>(TI)->LastChildNr++;
    }

    // Traverse the class hierarchy in depth-first order, assigning an increasing number to each node in the process.
    // At the same time, adjust the LastChildNr of each class accordingly, so that it is easy to determine the
    // inheritance relationship of two classes: a class D has been derived from another class B, when the expression
    // (B.TypeNr<=D.TypeNr && D.TypeNr<=B.LastChildNr) holds.
    unsigned long CurrentNr=0;

    for (unsigned long RootNr=0; RootNr<TypeInfoRoots.Size(); RootNr++)
    {
        for (const TypeInfoT* TI=TypeInfoRoots[RootNr]; TI!=NULL; TI=TI->GetNext())
        {
            const_cast<TypeInfoT*>(TI)->TypeNr=CurrentNr;
            const_cast<TypeInfoT*>(TI)->LastChildNr+=CurrentNr;

            CurrentNr++;
        }
    }


    // Finally, fill-in the TypeInfosByNr array.
    TypeInfosByNr.PushBackEmpty(TypeInfosByName.Size());

    for (unsigned long TypeNr=0; TypeNr<TypeInfosByName.Size(); TypeNr++)
        TypeInfosByNr[TypeInfosByName[TypeNr]->TypeNr]=TypeInfosByName[TypeNr];


// #ifdef DEBUG
#if 0
    // Debug output.
    Console->Print("\n");
    Console->Print("Types by name:\n");
    for (unsigned long TypeNr=0; TypeNr<TypeInfosByName.Size(); TypeNr++)
    {
        TypeInfosByName[TypeNr]->Print(false);
    }

    Console->Print("Types by type number:\n");
    for (unsigned long TypeNr=0; TypeNr<TypeInfosByNr.Size(); TypeNr++)
    {
        assert(TypeInfosByNr[TypeNr]->TypeNr==TypeNr);
        TypeInfosByNr[TypeNr]->Print(false);
    }

    Console->Print(cf::va("Type (class) hierarchy (%lu root%s):\n", TypeInfoRoots.Size(), TypeInfoRoots.Size()==1 ? "" : "s"));
    for (unsigned long RootNr=0; RootNr<TypeInfoRoots.Size(); RootNr++)
    {
        const TypeInfoT* Root=TypeInfoRoots[RootNr];

        assert(Root->Base   ==NULL);
        assert(Root->Sibling==NULL);

        Root->Print();
    }

    Console->Print("\n");
#endif

    IsInited=true;
}


const TypeInfoT* TypeInfoManT::FindTypeInfoByName(const char* ClassName) const
{
    assert(IsInited);

    if (ClassName==NULL) return NULL;
    if (TypeInfosByName.Size()==0) return NULL;

    // Implement a binary search for the ClassName.
    // Note that we cannot use the "unsigned long" type for Min, Max and Mid, because Max can get -1 if Mid is 0.
    int Min=0;
    int Max=TypeInfosByName.Size()-1;

    while (Min<=Max)
    {
        const int        Mid=(Min+Max)/2;
        const TypeInfoT* TI =TypeInfosByName[Mid];

        const int Result=strcmp(TI->ClassName, ClassName);

             if (Result==0) return TI;
        else if (Result >0) Max=Mid-1;  // Max can get -1 here.
        else                Min=Mid+1;
    }

    return NULL;
}


const TypeInfoT* TypeInfoManT::FindTypeInfoByNr(unsigned long TypeNr) const
{
    assert(IsInited);
    assert(TypeInfosByNr[TypeNr]->TypeNr==TypeNr);

    return TypeInfosByNr[TypeNr];
}
