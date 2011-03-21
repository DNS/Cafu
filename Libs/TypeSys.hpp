/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _CF_TYPESYS_TYPEINFO_HPP_
#define _CF_TYPESYS_TYPEINFO_HPP_

#include "Templates/Array.hpp"
#include <ostream>


struct luaL_Reg;


namespace cf
{
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
            TypeInfoT(TypeInfoManT& TIM, const char* ClassName_, const char* BaseClassName_, CreateInstanceT CreateInstance_, const luaL_Reg MethodsList_[]);

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

         // const ArrayT<TypeInfoT*>& GetListByName() const;    // Only call after Init().
         // const ArrayT<TypeInfoT*>& GetListByNr() const;      // Only call after Init().
            const ArrayT<const TypeInfoT*>& GetTypeInfoRoots() const { assert(IsInited); return TypeInfoRoots; }  // Only call after Init().

            /// This is an auxiliary method for creating Lua scripting documentation for the registered classes.
            /// Assuming that the classes registered with this type info manager provide methods for access from Lua scripts,
            /// this method creates Doxygen input files ("fake headers") that documentation writers can complete to create
            /// related reference documentation.
            void CreateLuaDoxygenHeader(std::ostream& Out) const;


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
