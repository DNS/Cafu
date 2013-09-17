/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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


// Non-float variables cannot be interpolated.
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<int>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<unsigned int>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<bool>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT<std::string>& Var) { }
void VarVisitorGetApproxT::visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var) { }
