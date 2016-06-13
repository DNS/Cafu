/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/****************************************/
/*** Dependency Relationship Matrices ***/
/****************************************/

#include "DepRelMatrix.hpp"


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
