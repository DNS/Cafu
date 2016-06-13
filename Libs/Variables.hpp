/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TYPESYS_VARIABLES_HPP_INCLUDED
#define CAFU_TYPESYS_VARIABLES_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "Math3D/Vector2.hpp"
#include "Math3D/Vector3.hpp"
#include "Math3D/BoundingBox.hpp"

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif

#include <cstring>
#include <map>


/**
@page OverviewVariables TypeSys Variables Overview

The "variable" classes in cf::TypeSys are supposed to turn a "normal" member
variable of a class into something "more" that also other code can work with.

Problem and Motivation
======================

In Cafu we use "components" to compose map entities and GUI windows, see the
cf::GuiSys::ComponentBaseT hierarchy for an example.
Components are classes like any other, but their member variables should be:

  - editable in our graphical map and GUI editor CaWE,
  - accessible via scripting, and
  - easy to serialize and deserialize (maps, GUIs, prefabs).

However, none of these features should impact the component in its actual,
inherent purpose, being a component. The cf::GuiSys::ComponentBaseT classes
should not be hindered in their performance, and their interfaces and
implementations should not be cluttered with the details of the above
requirements.
In fact, it would be desireable to separate these issues from the components,
so that the way in which the editors let users edit the variables or the
details in which variables are bound to scripts can be varied without ever
affecting the components themselves.

How it works
============

Consider the typical code structure of a component that has a member variable `x`:

~~~~~~~~~~~~~~~~{cpp}
class SomeComponentT
{
    public:

    // [ other methods ... ]
    void SetX(int newX);
    int GetX() const;


    private:

    int x;
};
~~~~~~~~~~~~~~~~

A member variable usually implies three items: the variable itself, a "Get"
function and a "Set" function.
The implementations of the two functions are often trivial and `inline`, but
it is not unusual that especially the "Set" function has side-effects, such as
updating a graphical resource if a new filename has been set, etc.

Now, the key idea is to replace the variable and its Get/Set functions with a
"wrapper" class.
This is what the cf::TypeSys::VarT classes in this file are for.

As we also need a list of all cf::TypeSys::VarT instances in the class that
contains them (the component), the cf::TypeSys::VarT classes all must derive
from a common base class, cf::TypeSys::VarBaseT.
As the list of variables should actually have more features than a plain array
or list, we manage them in a cf::TypeSys::VarManT instance.

Finally, we apply the [Visitor pattern](http://en.wikipedia.org/wiki/Visitor_pattern)
to the cf::TypeSys::VarBaseT hierarchy: It allows user code to add arbitrary
operations to the cf::TypeSys::VarBaseT instances without modifying them.
*/


namespace cf
{
    namespace Network { class InStreamT; }
    namespace Network { class OutStreamT; }


    namespace TypeSys
    {
        class VisitorT;
        class VisitorConstT;


        /// This is the common base class for the VarT classes.
        ///
        /// It allows other code to work with VarT%s without having to know their concrete type.
        /// For example, VarManT is a container for pointers to VarBaseT%s.
        ///
        /// @see \ref OverviewVariables
        class VarBaseT
        {
            public:

            VarBaseT(const char* Name, const char* Flags[])
                : m_Name(Name), m_Flags(Flags) { }

            const char* GetName() const { return m_Name; }

            const char** GetFlags() const { return m_Flags; }
            bool HasFlag(const char* Flag) const;
            const char* GetFlag(const char* Flag, unsigned int Nr, const char* Default=NULL) const;

            /// Sometimes actions on variables generate extra messages that are relevant to the user.
            /// For example, setting a string that is interpreted as a filename can generate an extra
            /// message with details if the related resource could not be opened.
            /// This function returns the last extra message for this variable.
            /// However, note that a proper logging system would be a better solution to this problem,
            /// and we should probably replace this method with logging in the future.
            virtual std::string GetExtraMessage() const { return ""; }

            /// Stores the value of this variable in the given Stream.
            /// An implementation may also store additional data, so that Deserialize() is able to recover from side-
            /// effects where restoring the value of the variable alone does not suffice.
            /// For example, consider the side-effects of setting a new model name in a ComponentModelT as described
            /// at VarT::Set(): restoring the previous model name will properly restore the internal model resource,
            /// but not undo any clamps or resets that setting the new name caused for the animation and skin numbers.
            /// Thus, the implementation of this method for the model name would also store the values of the affected
            /// sibling variables, so that Deserialize() can implement a proper restore / "undo".
            virtual void Serialize(Network::OutStreamT& Stream) const = 0;

            /// Restores the value of this variable from the given Stream.
            /// See Serialize() for additional details.
            virtual void Deserialize(Network::InStreamT& Stream) = 0;

            virtual void accept(VisitorT&      Visitor) = 0;
            virtual void accept(VisitorConstT& Visitor) const = 0;


            private:

            const char*  m_Name;    ///< The name of the variable.
            const char** m_Flags;   ///< An optional list of context-dependent flags.
        };


        // template<class T> Network::InStreamT&  operator >> (Network::InStreamT&  Stream, VarBaseT& Var) { Var.Deserialize(Stream); return Stream; }
        // template<class T> Network::OutStreamT& operator << (Network::OutStreamT& Stream, const VarBaseT& Var) { Var.Serialize(Stream); return Stream; }


        /// This is a "wrapper" around a normal C++ variable.
        ///
        /// It can be used in place of a normal variable whenever the functionality
        /// described in \ref OverviewVariables is desired, for example in the member
        /// variables of component classes of game entities and GUI windows.
        ///
        /// User code can derive from this class and override the Set() method in
        /// order to customize the behaviour.
        ///
        /// @see \ref OverviewVariables
        template<class T> class VarT : public VarBaseT
        {
            public:

            /// The constructor.
            VarT(const char* Name, const T& Value, const char* Flags[]=NULL)
                : VarBaseT(Name, Flags), m_Value(Value) { }

            /// Returns the value of this variable.
            const T& Get() const { return m_Value; }

            /// Sets the value of this variable to the given value `v`.
            /// Derived classes can override this method to add "side-effects", such as updating graphical resources.
            /// In some cases, side-effects can even affect *other* variables (siblings). For example, setting a new
            /// model name in ComponentModelT not only updates the internal (private) model resource, but it can also
            /// imply updates (resets or clamps) to the animation and skin number variables.
            virtual void Set(const T& v) { m_Value = v; }

            /// This method returns a list of acceptable input values for this variable, along with a string
            /// representation of each.
            /// The relevancy of the returned tuples is limited: They are intended to create helpful user interfaces
            /// in our graphical editors and to provide extra information in scripts, but if Set() is called with a
            /// value that is not in `Values`, it will work and is not an error.
            /// If the method returns no tuples at all, it means that user input is free and any value is acceptable.
            virtual void GetChoices(ArrayT<std::string>& Strings, ArrayT<T>& Values) const { }

            void Serialize(Network::OutStreamT& Stream) const /*override*/;
            void Deserialize(Network::InStreamT& Stream) /*override*/;

            void accept(VisitorT&      Visitor) /*override*/;
            void accept(VisitorConstT& Visitor) const /*override*/;


            private:

            T m_Value;  ///< The actual variable that is wrapped by this class.
        };


        /// This is a "wrapper" around a normal C++ variable specifically of type ArrayT<T>.
        ///
        /// This class is similar to VarT< ArrayT<T> >, but was invented because working with VarT< ArrayT<T> >s
        /// is actually difficult in practice:
        ///
        ///   - Setting individual elements of the array or modifying the array itself (e.g. pushing back another
        ///     element) via the VarT::Set() method is cumbersome.
        ///   - Adding a non-const Get() method to VarT< ArrayT<T> > was not desired, because it would break the
        ///     accounting for side-effects that is guaranteed by the VarT::Set() method.
        ///   - (Arrays seem to need side-effects as little as lists of pre-made choices.)
        ///
        /// Therefore, VarArrayT<T> was made to have an interface that is easier to use and more efficient when
        /// working with arrays, and consciously omits the "possible side-effects" feature that VarT::Set() has.
        ///
        /// It can be used in place of a normal ArrayT variable whenever the functionality described in
        /// \ref OverviewVariables is desired, for example in the member variables of component classes of game
        /// entities and GUI windows.
        ///
        /// @see \ref OverviewVariables
        template<class T> class VarArrayT : public VarBaseT
        {
            public:

            /// The constructor.
            VarArrayT(const char* Name, unsigned int InitSize, const T& InitValue, const char* Flags[]=NULL);

            const ArrayT<T>& Get() const   { return m_Array; }
            unsigned int Size() const      { return m_Array.Size(); }
            void Clear()                   { m_Array.Clear(); }
            void Overwrite()               { m_Array.Overwrite(); }
            void PushBack(const T Element) { m_Array.PushBack(Element); }

            const T& operator [] (unsigned int i) const { return m_Array[i]; }
            T& operator [] (unsigned int i) { return m_Array[i]; }

            void Serialize(Network::OutStreamT& Stream) const /*override*/;
            void Deserialize(Network::InStreamT& Stream) /*override*/;

            void accept(VisitorT&      Visitor) /*override*/;
            void accept(VisitorConstT& Visitor) const /*override*/;


            private:

            ArrayT<T> m_Array;  ///< The actual array that is wrapped by this class.
        };


        /// This is the base class for the visitors of VarT%s.
        ///
        /// With the Visitor pattern, the data structure being used is independent
        /// of the uses to which it is being put.
        ///
        /// @see \ref OverviewVariables
        class VisitorT
        {
            public:

            virtual ~VisitorT() { }

            virtual void visit(VarT<float>& Var) = 0;
            virtual void visit(VarT<double>& Var) = 0;
            virtual void visit(VarT<int>& Var) = 0;
            virtual void visit(VarT<unsigned int>& Var) = 0;
            virtual void visit(VarT<uint16_t>& Var) = 0;
            virtual void visit(VarT<uint8_t>& Var) = 0;
            virtual void visit(VarT<bool>& Var) = 0;
            virtual void visit(VarT<std::string>& Var) = 0;
            virtual void visit(VarT<Vector2fT>& Var) = 0;
            virtual void visit(VarT<Vector3fT>& Var) = 0;
            virtual void visit(VarT<Vector3dT>& Var) = 0;
            virtual void visit(VarT<BoundingBox3dT>& Var) = 0;
            virtual void visit(VarArrayT<uint32_t>& Var) = 0;
            virtual void visit(VarArrayT<uint16_t>& Var) = 0;
            virtual void visit(VarArrayT<uint8_t>& Var) = 0;
            virtual void visit(VarArrayT<std::string>& Var) = 0;
        };


        /// Like VisitorT, but for `const` VarT%s.
        ///
        /// @see \ref OverviewVariables.
        class VisitorConstT
        {
            public:

            virtual ~VisitorConstT() { }

            virtual void visit(const VarT<float>& Var) = 0;
            virtual void visit(const VarT<double>& Var) = 0;
            virtual void visit(const VarT<int>& Var) = 0;
            virtual void visit(const VarT<unsigned int>& Var) = 0;
            virtual void visit(const VarT<uint16_t>& Var) = 0;
            virtual void visit(const VarT<uint8_t>& Var) = 0;
            virtual void visit(const VarT<bool>& Var) = 0;
            virtual void visit(const VarT<std::string>& Var) = 0;
            virtual void visit(const VarT<Vector2fT>& Var) = 0;
            virtual void visit(const VarT<Vector3fT>& Var) = 0;
            virtual void visit(const VarT<Vector3dT>& Var) = 0;
            virtual void visit(const VarT<BoundingBox3dT>& Var) = 0;
            virtual void visit(const VarArrayT<uint32_t>& Var) = 0;
            virtual void visit(const VarArrayT<uint16_t>& Var) = 0;
            virtual void visit(const VarArrayT<uint8_t>& Var) = 0;
            virtual void visit(const VarArrayT<std::string>& Var) = 0;
        };


        /// This class is a simple container for pointers to VarBaseTs.
        ///
        /// Together with the VarT classes, it provides a very simple kind of
        /// "reflection" or "type introspection" feature.
        /// See class ComponentBaseT for an example use.
        ///
        /// @see \ref OverviewVariables
        class VarManT
        {
            public:

            struct CompareCStr
            {
                // See "Die C++ Programmiersprache" by Bjarne Stroustrup pages 498 and 510 and
                // Scott Meyers "Effective STL" Item 21 for more information about this struct.
                bool operator () (const char* a, const char* b) const { return std::strcmp(a, b) < 0; }
            };

            typedef std::map<const char*, VarBaseT*, CompareCStr> MapVarBaseT;


            void Add(VarBaseT* Var);

            /// Adds an alias name for the given variable so that a call to Find() will also find the variable
            /// under the alias name.
            /// The purpose of this method is to provide backwards-compatibility if variables must be renamed
            /// after they have been introduced and became widely used, e.g. in custom user scripts.
            void AddAlias(const char* Alias, VarBaseT* Var);

            const ArrayT<VarBaseT*>& GetArray() const { return m_VarsArray; }

            VarBaseT* Find(const char* Name) const
            {
                const MapVarBaseT::const_iterator It = m_VarsMap.find(Name);

                return It != m_VarsMap.end() ? It->second : NULL;
            }


            private:

            ArrayT<VarBaseT*> m_VarsArray;  ///< Keeps the variables in the order they were added.
            MapVarBaseT       m_VarsMap;    ///< Keeps the variables by name (for find by name and lexicographical traversal).
        };
    }
}

#endif
