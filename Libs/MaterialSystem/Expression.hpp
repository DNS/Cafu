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

#ifndef _CA_MATSYS_EXPRESSION_HPP_
#define _CA_MATSYS_EXPRESSION_HPP_

#include <string>
#include "Templates/Array.hpp"


class TextParserT;


class TableT
{
    public:

    struct RowT
    {
        bool          RowSnap;
        bool          RowClamp;
        ArrayT<float> Data;
    };

    std::string  Name;
    bool         ColumnSnap;
    bool         ColumnClamp;
    ArrayT<RowT> Rows;


    /// Constructor.
    TableT() : ColumnSnap(false), ColumnClamp(false) { }

    /// Looks up the table value at x.
    float Lookup(float x) const;
};


/// A typed expression class.
class ExpressionT
{
    public:

    /// The type of the expression.
    /// If bit 0x10000 is set, the type is the generic float variable whose index is obtained from the 16 lower (0x0FFFF) bits.
    /// If bit 0x20000 is set, the type is the generic int   variable whose index is obtained from the 16 lower (0x0FFFF) bits.
    enum TypeT { FloatNumber, IntNumber, SymbolTime, SymbolALRed, SymbolALGreen, SymbolALBlue, TableLookup,
                 CastInt, Add, Sub, Mul, Div, GENERIC_FLOAT_START=0x10000, GENERIC_INT_START=0x20000 };

    /// A helper structure for returning both the type and the value of an expression.
    struct ResultT
    {
        TypeT Type;         ///< The type of the result, can only be FloatNumber or IntNumber.
        float ValueFloat;   ///< If Type==FloatNumber, this is the value of the result. Otherwise it is undefined.
        int   ValueInt;     ///< If Type==IntNumber,   this is the value of the result. Otherwise it is undefined.

        ResultT(float Value) : Type(FloatNumber), ValueFloat(Value), ValueInt(0    ) { }    ///< Constructor.
        ResultT(int   Value) : Type(IntNumber  ), ValueFloat(0.0  ), ValueInt(Value) { }    ///< Constructor.

        float GetAsFloat() const { return Type==FloatNumber ?     ValueFloat  : ValueInt; }
        int   GetAsInt  () const { return Type==FloatNumber ? int(ValueFloat) : ValueInt; }
    };

    /// This structure contains the values for the "variables" of an expression.
    /// An instance of it must be passed to the Evaluate() method.
    struct SymbolsT
    {
        SymbolsT();

        float         Time;
        float         AmbientLightColor[3];
        ArrayT<float> GenFloat;     ///< Generic / general purpose float variables/symbols.
        ArrayT<int>   GenInt;       ///< Generic / general purpose int   variables/symbols.
    };


    /// Constructor.
    ExpressionT(float FloatNumberValue_=0.0);

    /// Constructor.
    ExpressionT(int IntNumberValue_);

    /// This constructor creates an ExpressionT of a given type.
    /// Note that only those types are meaningful for which no other parameter is required,
    /// such as \c SymbolTime, \c SymbolALRed, \c SymbolALGreen and \c SymbolALBlue.
    ExpressionT(TypeT Type_);

    /// Constructor.
    ExpressionT(TextParserT& TP, const ArrayT<TableT*>& ListOfTables) /*throw (TextParserT::ParseError)*/;

    /// Copy Constructor (Law of the Big Three).
    ExpressionT(const ExpressionT& Source);

    /// Destructor (Law of the Big Three).
    ~ExpressionT();

    /// Assignment Operator (Law of the Big Three).
    ExpressionT& operator = (const ExpressionT& Source);

    /// Returns a string description of this ExpressionT.
    std::string GetString() const;

    /// Evaluates this expression and returns the result.
    ResultT Evaluate(const SymbolsT& Symbols) const;


    private:

    TypeT        Type;
    float        FloatNumberValue;
    int          IntNumberValue;
    TableT*      Table;
    ExpressionT* Child1;
    ExpressionT* Child2;

    /// The equal operator is not defined.
    bool operator == (const ExpressionT& rhs) const;
};

#endif
