/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Variables.hpp"
#include "Network/State.hpp"


/****************/
/*** VarBaseT ***/
/****************/

bool cf::TypeSys::VarBaseT::HasFlag(const char* Flag) const
{
    if (!m_Flags) return false;

    for (unsigned int i = 0; m_Flags[i]; i++)
        if (strcmp(Flag, m_Flags[i]) == 0)
            return true;

    return false;
}


const char* cf::TypeSys::VarBaseT::GetFlag(const char* Flag, unsigned int Nr, const char* Default) const
{
    if (!m_Flags) return Default;

    unsigned int i;

    for (i = 0; m_Flags[i]; i++)
        if (strcmp(Flag, m_Flags[i]) == 0)
            break;

    for (unsigned int j = i; m_Flags[j]; j++)
        if (j == i + Nr)
            return m_Flags[j];

    return Default;
}


/***************/
/*** VarT<T> ***/
/***************/

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
        template<>  // Must specialize, because an OutStreamT cannot take a Vector2fT directly.
        void VarT<Vector2fT>::Serialize(cf::Network::OutStreamT& Stream) const
        {
            Stream << Get().x;
            Stream << Get().y;
        }

        template<>  // Must specialize, because an OutStreamT cannot take a Vector3fT directly.
        void VarT<Vector3fT>::Serialize(cf::Network::OutStreamT& Stream) const
        {
            Stream << Get().x;
            Stream << Get().y;
            Stream << Get().z;
        }

        template<>  // Must specialize, because an OutStreamT cannot take a Vector3dT directly.
        void VarT<Vector3dT>::Serialize(cf::Network::OutStreamT& Stream) const
        {
            Stream << Get().x;
            Stream << Get().y;
            Stream << Get().z;
        }

        template<>  // Must specialize, because an OutStreamT cannot take a BoundingBox3dT directly.
        void VarT<BoundingBox3dT>::Serialize(cf::Network::OutStreamT& Stream) const
        {
            Stream << Get().Min.x;
            Stream << Get().Min.y;
            Stream << Get().Min.z;

            Stream << Get().Max.x;
            Stream << Get().Max.y;
            Stream << Get().Max.z;
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
        template<>  // Must specialize, because an InStreamT cannot take a Vector2fT directly.
        void VarT<Vector2fT>::Deserialize(cf::Network::InStreamT& Stream)
        {
            Vector2fT v;

            Stream >> v.x;
            Stream >> v.y;

            // Derived classes may have overridden Set() to add "side-effects", such as updating graphical resources.
            // Therefore, we cannot write `m_Value = v;` in place of `Set(v);` here.
            Set(v);
        }

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

        template<>  // Must specialize, because an InStreamT cannot take a Vector3dT directly.
        void VarT<Vector3dT>::Deserialize(cf::Network::InStreamT& Stream)
        {
            Vector3dT v;

            Stream >> v.x;
            Stream >> v.y;
            Stream >> v.z;

            // Derived classes may have overridden Set() to add "side-effects", such as updating graphical resources.
            // Therefore, we cannot write `m_Value = v;` in place of `Set(v);` here.
            Set(v);
        }

        template<>  // Must specialize, because an InStreamT cannot take a BoundingBox3dT directly.
        void VarT<BoundingBox3dT>::Deserialize(cf::Network::InStreamT& Stream)
        {
            BoundingBox3dT BB;

            Stream >> BB.Min.x;
            Stream >> BB.Min.y;
            Stream >> BB.Min.z;

            Stream >> BB.Max.x;
            Stream >> BB.Max.y;
            Stream >> BB.Max.z;

            // Derived classes may have overridden Set() to add "side-effects", such as updating graphical resources.
            // Therefore, we cannot write `m_Value = BB;` in place of `Set(BB);` here.
            Set(BB);
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


/********************/
/*** VarArrayT<T> ***/
/********************/

template<class T>
cf::TypeSys::VarArrayT<T>::VarArrayT(const char* Name, unsigned int InitSize, const T& InitValue, const char* Flags[])
    : VarBaseT(Name, Flags)
{
    m_Array.PushBackEmptyExact(InitSize);

    for (unsigned int i = 0; i < InitSize; i++)
        m_Array[i] = InitValue;
}


template<class T>
void cf::TypeSys::VarArrayT<T>::Serialize(cf::Network::OutStreamT& Stream) const
{
    Stream << m_Array;
}


template<class T>
void cf::TypeSys::VarArrayT<T>::Deserialize(cf::Network::InStreamT& Stream)
{
    // Contrary to `VarT::Deserialize()`, which calls `VarT::Set()` in order to account for
    // side-effects resulting from derived classes that have provided an override for `VarT::Set()`,
    // any such side-effects are not supported and not accounted for here.
    Stream >> m_Array;
}


template<class T>
void cf::TypeSys::VarArrayT<T>::accept(VisitorT& Visitor)
{
    Visitor.visit(*this);
}


template<class T>
void cf::TypeSys::VarArrayT<T>::accept(VisitorConstT& Visitor) const
{
    Visitor.visit(*this);
}


/***************/
/*** VarManT ***/
/***************/

void cf::TypeSys::VarManT::Add(VarBaseT* Var)
{
    m_VarsArray.PushBack(Var);
    m_VarsMap[Var->GetName()] = Var;
}


void cf::TypeSys::VarManT::AddAlias(const char* Alias, VarBaseT* Var)
{
    m_VarsMap[Alias] = Var;
}


/*******************************/
/*** Template Instantiations ***/
/*******************************/

template class cf::TypeSys::VarT<float>;
template class cf::TypeSys::VarT<double>;
template class cf::TypeSys::VarT<int>;
template class cf::TypeSys::VarT<unsigned int>;
template class cf::TypeSys::VarT<uint16_t>;
template class cf::TypeSys::VarT<uint8_t>;
template class cf::TypeSys::VarT<bool>;
template class cf::TypeSys::VarT<std::string>;
template class cf::TypeSys::VarT<Vector2fT>;
template class cf::TypeSys::VarT<Vector3fT>;
template class cf::TypeSys::VarT<Vector3dT>;
template class cf::TypeSys::VarT<BoundingBox3dT>;
template class cf::TypeSys::VarArrayT<uint32_t>;
template class cf::TypeSys::VarArrayT<uint16_t>;
template class cf::TypeSys::VarArrayT<uint8_t>;
template class cf::TypeSys::VarArrayT<std::string>;
