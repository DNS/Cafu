/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TYPESYS_TYPEINFO_HPP_INCLUDED
#define CAFU_TYPESYS_TYPEINFO_HPP_INCLUDED

#include "Templates/Array.hpp"


struct luaL_Reg;


namespace cf
{
    /// The TypeSys ("type system") namespace provides classes for two related, but entirely independent concepts:
    ///   - The TypeInfoT and TypeInfoManT classes provide meta-information about classes and class hierarchies.
    ///   - The Var*T classes provide meta-information about class members.
    /// The rest of this text talks about the TypeInfo*T classes only: see the documentation of the Var*T classes
    /// for the second point. -- TODO: update/revise this text accordingly!
    ///
    /// The main purpose of the Type System is to make the programmers life easier when he is working with inheritance
    /// hierarchies of C++ classes. In many such cases, "meta" knowledge about the classes is required.
    ///
    /// For example, when binding a hierarchy of C++ classes to Lua it is very helpful during Lua state initialization when
    /// we can iterate over all classes in the hierarchy (specially ordered so that bases classes always occur before their
    /// child classes). Additionally, classes should register themselves automatically in the data-structure for the
    /// iteration, so that adding new classes to the hierarchy is easy and doesn't require any changes elsewhere, such as
    /// to the Lua init code (a very convenient and important feature for MOD and game developers!).
    /// The TypeSys is employed for the game entities hierarchy (BaseEntityT), for which the TypeSys was initially invented,
    /// as well as the windows hierarchy of the GuiSys (cf::GuiSys::WindowT), which supports scripting as well.
    ///
    /// Other uses directly suggest themselves from the details of the implementation, or were trivially added to the basic system:
    ///   - Auto-assign each type a unique type number, e.g. for sending over the network.
    ///     The type number is also "robust", i.e. independent of actual static initialization order.
    ///   - Be able to iterate over all types (entity classes), e.g. for early initialization of the Lua state (adding a metatable
    ///     for each entity class to the registry of the Lua state), or for listing all supported script methods (user help), etc.
    ///   - Factory: Be able to instantiate an entity class by type name (e.g. from map file) or by type number (e.g. as received over the network).
    /// ( - Provide a faster means for dynamic_cast<>(). )
    ///
    /// The Type System achieves its goals by explicitly modelling an inheritance hierarchy of C++ classes as a graph,
    /// which presents a "meta-layer" of information about the hierarchy and thus allows other C++ code to become conciously
    /// aware of the inheritance structure.
    namespace TypeSys
    {
        class TypeInfoManT;


        // Unfortunately, when this is nested in TypeInfoT, it cannot be forward-declared...
        class CreateParamsT
        {
        };


        class MethsDocT
        {
            public:

            const char* Name;
            const char* Doc;
            const char* ReturnType;
            const char* Parameters;
        };


        class VarsDocT
        {
            public:

            const char* Name;
            const char* Doc;
        };


        /// This class keeps type information (about an entity class that occurs in the game).
        /// (Supports single, but not multiple inheritance.)
        class TypeInfoT
        {
            public:

            // TODO: Is there a nice way to make the CreateInstance() call-back type-safe?
            //       Note that for entities (who have a virtual GetType() method), the following asserts should hold:
            //           BaseEntityT* BE=dynamic_cast<BaseEntityT*>(SomeType->CreateInstance());
            //        or BaseEntityT* BE=(BaseEntityT*)SomeType->CreateInstance();
            //           assert(BE!=NULL);
            //           assert(BE->GetType()==SomeType);
            typedef void* (*CreateInstanceT)(const CreateParamsT& Params);

            /// The constructor.
            /// This is supposed to be called only by the static TypeInfoT member of each entity class.
            TypeInfoT(TypeInfoManT& TIM, const char* ClassName_, const char* BaseClassName_, CreateInstanceT CreateInstance_, const luaL_Reg MethodsList_[],
                      const char* DocClass_=NULL, const MethsDocT DocMethods_[]=NULL, const MethsDocT DocCallbacks_[]=NULL, const VarsDocT DocVars_[]=NULL);

            /// Prints the contents of this node to the console.
            /// @param Recurse    Whether the children (derived classes) of this node should be printed.
            /// @param RecDepth   The current recursion depth. This is for the methods internal use - user code should never pass it.
            void Print(bool Recurse=true, unsigned long RecDepth=0) const;

            /// Determines whether the other given type info is in the inheritance tree of this type
            /// (this method roughly corresponds to dynamic_cast<>() in C++).
            bool HierarchyHas(const TypeInfoT* Other) const;

            /// Returns the next type info in the hierarchy.
            const TypeInfoT* GetNext() const;


            const char*      ClassName;         ///< The name of this class.
            const char*      BaseClassName;     ///< The name of the base/super/parent class of this class.
            CreateInstanceT  CreateInstance;    ///< The call-back function that creates an instance of this class.
            const luaL_Reg*  MethodsList;       ///< The list (array) of Lua methods that this class implements.

            const TypeInfoT* Base;              ///< The type info for the base class.
            const TypeInfoT* Sibling;           ///< The type info for the next sibling class (a linked list).
            const TypeInfoT* Child;             ///< The type info for the first child class.
            unsigned long    TypeNr;            ///< The unique and "robust" number of this type, obtained by enumerating the hierarchy nodes in depth-first order.
            unsigned long    LastChildNr;       ///< The highest TypeNr in the subhierarchy of this type. Depth-first enumeration guarantees that for all type numbers T in this subhierarchy, TypeNr<=T<=LastChildNr holds.

            // The next three members provide documentation about the class that this TypeInfoT is about.
            // They are intended to provide the user with live help in CaWE (e.g. as tool-tips), with live
            // help in scripts (e.g. `window:help()`), and to automatically generate "fake" header files
            // that are processed by Doxygen in order to create online scripting reference documentation.
            const char*      DocClass;          ///< Documentation for this class.
            const MethsDocT* DocMethods;        ///< Documentation for the Lua methods in MethodsList.
            const MethsDocT* DocCallbacks;      ///< Documentation for any Lua methods that we call but expect users to provide implementations for.
            const VarsDocT*  DocVars;           ///< Documentation for the variables in this class. (Used in classes that have `cf::TypeSys::VarT<>` instances.)


            private:

            TypeInfoT(const TypeInfoT&);            ///< Use of the Copy Constructor    is not allowed.
            void operator = (const TypeInfoT&);     ///< Use of the Assignment Operator is not allowed.
        };


        /// This class manages the type infos.
        ///
        /// Intended usage:
        /// Each entity class that occurs in the game keeps a static TypeInfoT member
        /// (plus a non-static, *virtual* GetTypeInfo() method that returns that member).
        /// The constructor of each such TypeInfoT member registers its own this-pointer with an instance of this class,
        /// so that when main() begins, the TypeInfoManT is aware of all TypeInfoTs and thus all entity classes.
        /// Its Init() method then completes the type initialization.
        class TypeInfoManT
        {
            public:

            /// The constructor.
            TypeInfoManT();

            /// Registers the given TypeInfoT with this type info manager.
            /// Only the TypeInfoT constructor code should call this, which in turn should only occur during global static initialization time,
            /// never after Init() has been called.
            void Register(TypeInfoT* TI);

            /// Completes the initialization of the TypeInfoManT and the registered type infos.
            /// Should be called exactly once before any other TypeInfoManT method is called.
            void Init();


            /// Returns the type info matching the class name ClassName.
            /// @returns the (pointer to the) TypeInfoT matching ClassName, or NULL if not found.
            const TypeInfoT* FindTypeInfoByName(const char* ClassName) const;

            /// Returns the type info matching the type number TypeNr.
            /// @returns the (pointer to the) TypeInfoT T, so that T->TypeNr==TypeNr.
            const TypeInfoT* FindTypeInfoByNr(unsigned long TypeNr) const;

            /// Returns all type infos registered with this TypeInfoManT, ordered by name.
            const ArrayT<const TypeInfoT*>& GetTypeInfosByName() const { assert(IsInited); return TypeInfosByName; }

            /// Returns all type infos registered with this TypeInfoManT, ordered by type number.
            /// `GetTypeInfosByNr()[i]->TypeNr == i`
            const ArrayT<const TypeInfoT*>& GetTypeInfosByNr() const { assert(IsInited); return TypeInfosByNr; }

            /// Returns the roots of the inheritance trees. (Normally there should only be one root.)
            const ArrayT<const TypeInfoT*>& GetTypeInfoRoots() const { assert(IsInited); return TypeInfoRoots; }


            private:

            bool                     IsInited;        ///< Tells whether Init() has already been called.
            ArrayT<const TypeInfoT*> TypeInfosByName; ///< The registered type infos listed in alphabetical order.
            ArrayT<const TypeInfoT*> TypeInfosByNr;   ///< The registered type infos listed in TypeNr order, TypeInfosByNr[i].TypeNr==i.
            ArrayT<const TypeInfoT*> TypeInfoRoots;   ///< The roots of the inheritance trees. Normally there should only be one root.
        };
    }
}


/* #define DECLARE_TYPE_INFO() \
    /// Returns the proper type info for this class. This method is virtual \
    /// so that it works even when called with a base class pointer. \
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; } \
\
    /// Creates an instance of this class from the given Params object. \
    /// This call-back function is a part (member) of the TypeInfoT of this class. \
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params); \
\
    /// The type info object for (objects/instances of) this class. \
    static const cf::TypeSys::TypeInfoT TypeInfo; */


/* #define IMPLEMENT_TYPE_INFO() \
//     ... */

#endif
