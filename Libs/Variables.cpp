/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "Variables.hpp"
#include "Network/State.hpp"


bool cf::TypeSys::VarBaseT::HasFlag(const char* Flag) const
{
    if (!m_Flags) return false;

    for (unsigned int i = 0; m_Flags[i]; i++)
        if (strcmp(Flag, m_Flags[i]) == 0)
            return true;

    return false;
}


void cf::TypeSys::VarManT::Add(VarBaseT* Var)
{
    m_VarsArray.PushBack(Var);
    m_VarsMap[Var->GetName()] = Var;
}


template<class T>
void cf::TypeSys::VarT<T>::Serialize(cf::Network::OutStreamT& Stream) const
{
    Stream << Get();
}


// With g++, we cannot write cf::TypeSys::VarT<Vector3fT>::Serialize(...),
// see search results for "g++ error specialization in different namespace" for details.
namespace cf
{
    namespace TypeSys
    {
        template<>  // Must specialize, because an OutStreamT cannot take a Vector3fT directly.
        void VarT<Vector3fT>::Serialize(cf::Network::OutStreamT& Stream) const
        {
            Stream << Get().x;
            Stream << Get().y;
            Stream << Get().z;
        }
    }
}


template<class T>
void cf::TypeSys::VarT<T>::Deserialize(cf::Network::InStreamT& Stream)
{
    T v;

    Stream >> v;

    // Derived classes may have overridden Set() to add "side-effects", such as updating graphical resources.
    // Therefore, we cannot write `m_Value = v;` in place of `Set(v);` here.
    Set(v);
}


// With g++, we cannot write cf::TypeSys::VarT<Vector3fT>::Deserialize(...),
// see search results for "g++ error specialization in different namespace" for details.
namespace cf
{
    namespace TypeSys
    {
        template<>  // Must specialize, because an InStreamT cannot take a Vector3fT directly.
        void VarT<Vector3fT>::Deserialize(cf::Network::InStreamT& Stream)
        {
            Vector3fT v;

            Stream >> v.x;
            Stream >> v.y;
            Stream >> v.z;

            // Derived classes may have overridden Set() to add "side-effects", such as updating graphical resources.
            // Therefore, we cannot write `m_Value = v;` in place of `Set(v);` here.
            Set(v);
        }
    }
}


template<class T>
void cf::TypeSys::VarT<T>::accept(VisitorT& Visitor)
{
    Visitor.visit(*this);
}


template<class T>
void cf::TypeSys::VarT<T>::accept(VisitorConstT& Visitor) const
{
    Visitor.visit(*this);
}


template class cf::TypeSys::VarT<float>;
template class cf::TypeSys::VarT<double>;
template class cf::TypeSys::VarT<int>;
template class cf::TypeSys::VarT<unsigned int>;
template class cf::TypeSys::VarT<std::string>;
template class cf::TypeSys::VarT<Vector3fT>;
template class cf::TypeSys::VarT< ArrayT<std::string> >;
