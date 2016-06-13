/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SetCompVar.hpp"
#include "DocumentAdapter.hpp"
#include "Variables.hpp"


namespace
{
    cf::Network::StateT GetOldState(cf::TypeSys::VarBaseT& Var)
    {
        cf::Network::StateT     State;
        cf::Network::OutStreamT Stream(State);

        Var.Serialize(Stream);

        return State;
    }
}


/**************************/
/*** CommandSetCompVarT ***/
/**************************/

template<class T>
CommandSetCompVarT<T>::CommandSetCompVarT(DocAdapterI& DocAdapter, cf::TypeSys::VarT<T>& Var, const T& NewValue)
    : m_DocAdapter(DocAdapter),
      m_Var(Var),
      m_OldState(GetOldState(Var)),
      m_NewValue(NewValue)
{
}


template<class T>
CommandSetCompVarT<T>::CommandSetCompVarT(DocAdapterI& DocAdapter, cf::TypeSys::VarT<T>& Var, const cf::Network::StateT& OldState)
    : m_DocAdapter(DocAdapter),
      m_Var(Var),
      m_OldState(OldState),
      m_NewValue(Var.Get())
{
    m_Done = true;
}


template<class T>
bool CommandSetCompVarT<T>::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    m_Var.Set(m_NewValue);

    m_DocAdapter.UpdateAllObservers_VarChanged(m_Var);
    m_Done=true;
    return true;
}


template<class T>
void CommandSetCompVarT<T>::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    cf::Network::InStreamT Stream(m_OldState);

    // Calling m_Var.Set() above may have caused side-effects, not only on internals (e.g. graphical resources) of the
    // component that the variable is a part of, but also on other, sibling variables of the same component.
    // A call like `m_Var.Set(m_OldValue);` would properly address only the former, but not the latter.
    m_Var.Deserialize(Stream);

    m_DocAdapter.UpdateAllObservers_VarChanged(m_Var);
    m_Done=false;
}


template<class T>
wxString CommandSetCompVarT<T>::GetName() const
{
    return wxString("Set ") + m_Var.GetName();
}


/*******************************/
/*** CommandSetCompVarArrayT ***/
/*******************************/

template<class T>
CommandSetCompVarArrayT<T>::CommandSetCompVarArrayT(DocAdapterI& DocAdapter, cf::TypeSys::VarArrayT<T>& Var, const ArrayT<T>& NewValues)
    : m_DocAdapter(DocAdapter),
      m_Var(Var),
      m_OldState(GetOldState(Var)),
      m_NewValues(NewValues)
{
}


template<class T>
CommandSetCompVarArrayT<T>::CommandSetCompVarArrayT(DocAdapterI& DocAdapter, cf::TypeSys::VarArrayT<T>& Var, const cf::Network::StateT& OldState)
    : m_DocAdapter(DocAdapter),
      m_Var(Var),
      m_OldState(OldState),
      m_NewValues(Var.Get())
{
    m_Done = true;
}


template<class T>
bool CommandSetCompVarArrayT<T>::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    m_Var.Overwrite();
    for (unsigned int i = 0; i < m_NewValues.Size(); i++)
        m_Var.PushBack(m_NewValues[i]);

    m_DocAdapter.UpdateAllObservers_VarChanged(m_Var);
    m_Done=true;
    return true;
}


template<class T>
void CommandSetCompVarArrayT<T>::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    cf::Network::InStreamT Stream(m_OldState);

    // Working with VarArrayTs can actually not involve side-effects, so a loop that sets `m_Var[i] = m_OldValues[i]`
    // would be feasible here. Still, let's use code like in CommandSetCompVarT for symmetry.
    m_Var.Deserialize(Stream);

    m_DocAdapter.UpdateAllObservers_VarChanged(m_Var);
    m_Done=false;
}


template<class T>
wxString CommandSetCompVarArrayT<T>::GetName() const
{
    return wxString("Set ") + m_Var.GetName();
}


/*******************************/
/*** Template Instantiations ***/
/*******************************/

template class CommandSetCompVarT<float>;
template class CommandSetCompVarT<double>;
template class CommandSetCompVarT<int>;
template class CommandSetCompVarT<unsigned int>;
template class CommandSetCompVarT<uint16_t>;
template class CommandSetCompVarT<uint8_t>;
template class CommandSetCompVarT<bool>;
template class CommandSetCompVarT<std::string>;
template class CommandSetCompVarT<Vector2fT>;
template class CommandSetCompVarT<Vector3fT>;
template class CommandSetCompVarT<Vector3dT>;
template class CommandSetCompVarT<BoundingBox3dT>;
template class CommandSetCompVarArrayT<uint32_t>;
template class CommandSetCompVarArrayT<uint16_t>;
template class CommandSetCompVarArrayT<uint8_t>;
template class CommandSetCompVarArrayT<std::string>;
