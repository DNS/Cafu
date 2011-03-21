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

/****************************************/
/*** Dependency Relationship Matrices ***/
/****************************************/

#include "DepRelMatrix.hpp"

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


unsigned long DepRelMatrixT::GlobalIDCount=0;


DepRelMatrixT::DepRelMatrixT()
    : Age(0),
      ID(GlobalIDCount++)
{
}


DepRelMatrixT::DepRelMatrixT(const DepRelMatrixT& Other)
    : Matrix(Other.Matrix),
      Age(0),
      ID(GlobalIDCount++)
{
}




InverseMatrixT::InverseMatrixT(DepRelMatrixT* Source)
    : m_Source(Source)
{
}


void InverseMatrixT::SetSourceMatrix(DepRelMatrixT* Source)
{
    m_Source=Source;
}


void InverseMatrixT::Update()
{
    // First make recursively sure that the source matrix is up-to-date.
    m_Source->Update();

    // Now see if updating the source actually aged (changed) it.
    if (m_Source->Age > Age)
    {
        Matrix=m_Source->Matrix.GetInverse();
        Age   =m_Source->Age;
    }
}




ProductMatrixT::ProductMatrixT(DepRelMatrixT& A, DepRelMatrixT& B)
    : m_A(A), m_B(B)
{
}


void ProductMatrixT::Update()
{
    // First make recursively sure that the sources are up-to-date.
    m_A.Update();
    m_B.Update();

    // Now see if updating them actually aged (changed) them.
    if (m_A.Age+m_B.Age > Age)
    {
        Matrix=m_A.Matrix*m_B.Matrix;
        Age   =m_A.Age+m_B.Age;
    }
}
