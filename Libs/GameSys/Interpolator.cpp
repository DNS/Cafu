/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Interpolator.hpp"

using namespace cf::GameSys;


VarVisitorGetApproxT::VarVisitorGetApproxT()
    : m_Approx(NULL)
{
}


VarVisitorGetApproxT::~VarVisitorGetApproxT()
{
    // If m_Approx != NULL, then TransferCommand() was not called and we're leaking memory.
    assert(m_Approx == NULL);
}


ApproxBaseT* VarVisitorGetApproxT::TransferApprox()
{
    ApproxBaseT* Approx = m_Approx;
    m_Approx = NULL;

    return Approx;
}


void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<float>& Var)
{
    assert(m_Approx == NULL);
    m_Approx = new VarInterpolatorT<float>(Var);
}


void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<double>& Var)
{
    assert(m_Approx == NULL);
    m_Approx = new VarInterpolatorT<double>(Var);
}


void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    assert(m_Approx == NULL);
    m_Approx = new VarInterpolatorT<Vector2fT>(Var);
}


void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    assert(m_Approx == NULL);

    if (Var.HasFlag("IsQuat")) m_Approx = new VarSlerpT(Var);
                          else m_Approx = new VarInterpolatorT<Vector3fT>(Var);
}


void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<Vector3dT>& Var)
{
    assert(!Var.HasFlag("IsQuat"));     // User code should really use a Vector3fT instead.
    assert(m_Approx == NULL);
    m_Approx = new VarInterpolatorT<Vector3dT>(Var);
}


// Non-float variables cannot be interpolated.
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<int>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<unsigned int>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<uint16_t>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<uint8_t>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<bool>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<std::string>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<BoundingBox3dT>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarArrayT<uint32_t>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarArrayT<uint16_t>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarArrayT<uint8_t>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarArrayT<std::string>& Var) { }
