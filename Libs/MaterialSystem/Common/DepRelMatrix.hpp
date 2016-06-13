/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/****************************************/
/*** Dependency Relationship Matrices ***/
/****************************************/

#ifndef CAFU_DEP_REL_MATRIX_HPP_INCLUDED
#define CAFU_DEP_REL_MATRIX_HPP_INCLUDED

#include "Math3D/Matrix.hpp"


/// A matrix class with which dependencies among matrices can be handled.
/// In order to model a specific dependency relationship, child classes should be derived from this class,
/// see InverseMatrixT and ProductMatrixT for examples.
/// Note that also the roots/parents/sources of the dep. relationships should (or at least: can) be matrix objects
/// of the DepRelMatrixT class, because that helps to avoid unecessary updates of the dependents.
class DepRelMatrixT
{
    public:

    DepRelMatrixT();
    DepRelMatrixT(const DepRelMatrixT& Other);
    virtual ~DepRelMatrixT() { }

    /// This method updates this matrix from the matrices it depends on (the source matrices).
    /// Derived classes are expected to overwrite this method in order to provide the desired behaviour.
    /// Their code should make good use of the Age member in order to minimize update efforts.
    /// User code should call this method before accessing the Matrix (or Age) member whenever
    /// there is a chance that the source matrices changed since the last call to Update().
    virtual void Update()
    {
    }

    MatrixT             Matrix;     ///< The matrix.
    unsigned long       Age;        ///< The "age" or change-count of this matrix. How old the source matrix was when we were last updated.
    const unsigned long ID;         ///< The unique ID of this matrix. Useful for unambiguous identification.


    private:

    static unsigned long GlobalIDCount;
};


/// This class models the relationship with which a inverse matrix depends on its original matrix.
class InverseMatrixT : public DepRelMatrixT
{
    public:

    InverseMatrixT(DepRelMatrixT* Source=NULL);

    /// Sets the source matrix. Useful if InverseMatrixTs are stored in an array.
    void SetSourceMatrix(DepRelMatrixT* Source);

    /// Overwrite the base class method.
    virtual void Update();


    private:

    DepRelMatrixT* m_Source;
};


/// This class models the relationship with which a product matrix A*B depends on its components A and B
/// (e.g.\ how a model-to-view matrix depends on the model-to-world and world-to-view matrices).
///
/// Note that \code ProductMatrixT ModelView(WorldToView, ModelToWorld); \endcode would be the correct statement
/// for a model-to-view matrix, whereas the opposite component order
/// \code ProductMatrixT ModelView(ModelToWorld, WorldToView); \endcode is wrong.
class ProductMatrixT : public DepRelMatrixT
{
    public:

    ProductMatrixT(DepRelMatrixT& A, DepRelMatrixT& B);

    /// Overwrite the base class method.
    virtual void Update();


    private:

    DepRelMatrixT& m_A;
    DepRelMatrixT& m_B;
};

#endif
