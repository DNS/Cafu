/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SET_COMPONENT_VARIABLE_HPP_INCLUDED
#define CAFU_SET_COMPONENT_VARIABLE_HPP_INCLUDED

#include "CommandPattern.hpp"
#include "Network/State.hpp"


class DocAdapterI;
namespace cf { namespace TypeSys { template<class T> class VarT; } }
namespace cf { namespace TypeSys { template<class T> class VarArrayT; } }


template<class T>
class CommandSetCompVarT : public CommandT
{
    public:

    /// The constructor for setting the given variable to a new value.
    CommandSetCompVarT(DocAdapterI& DocAdapter, cf::TypeSys::VarT<T>& Var, const T& NewValue);

    /// The constructor to be used when the variable has already been set to the new value.
    /// With this constructor, the command is initialized in the "already done" state.
    CommandSetCompVarT(DocAdapterI& DocAdapter, cf::TypeSys::VarT<T>& Var, const cf::Network::StateT& OldState);

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    DocAdapterI&              m_DocAdapter;
    cf::TypeSys::VarT<T>&     m_Var;
    const cf::Network::StateT m_OldState;
    const T                   m_NewValue;
};


template<class T>
class CommandSetCompVarArrayT : public CommandT
{
    public:

    /// The constructor for setting the given arry to a new set of values.
    CommandSetCompVarArrayT(DocAdapterI& DocAdapter, cf::TypeSys::VarArrayT<T>& Var, const ArrayT<T>& NewValues);

    /// The constructor to be used when the array has already been set to the new set of values.
    /// With this constructor, the command is initialized in the "already done" state.
    CommandSetCompVarArrayT(DocAdapterI& DocAdapter, cf::TypeSys::VarArrayT<T>& Var, const cf::Network::StateT& OldState);

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    DocAdapterI&               m_DocAdapter;
    cf::TypeSys::VarArrayT<T>& m_Var;
    const cf::Network::StateT  m_OldState;
    const ArrayT<T>            m_NewValues;
};

#endif
