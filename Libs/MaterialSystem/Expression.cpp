/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Expression.hpp"
#include "TextParser/TextParser.hpp"

#include <sstream>


/*************/
/*** Table ***/
/*************/

float TableT::Lookup(float x) const
{
    if (Rows.Size()==0) return 0.0;
    if (Rows[0].Data.Size()==0) return 0.0;

    // x is normalized with respect to the table length.
    // Now do un-normalize.
    x*=Rows[0].Data.Size();

    if (Rows[0].RowSnap)
    {
        // Snap.
        const int x_int=(x>=0.0) ? int(x) : int(x-1.0);     // The integral part of x. This is quasi a floor() function that returns ints.

        if (Rows[0].RowClamp)
        {
            // Clamp.
            if (x_int<                         0) return Rows[0].Data[                    0];
            if (x_int>int(Rows[0].Data.Size()-1)) return Rows[0].Data[Rows[0].Data.Size()-1];

            return Rows[0].Data[x_int];
        }
        else
        {
            // Wrap.
            return Rows[0].Data[x_int % Rows[0].Data.Size() + (x_int>=0 ? 0 : Rows[0].Data.Size())];
        }
    }
    else
    {
        // Interpolate.
        int   x1_int=(x>=0.0) ? int(x) : int(x-1.0);     // The integral part of x. This is quasi a floor() function that returns ints.
        int   x2_int=x1_int+1;
        float x_frac=x-x1_int;

        if (Rows[0].RowClamp)
        {
            // Clamp.
            if (x1_int<                         0) x1_int=                         0;
            if (x1_int>int(Rows[0].Data.Size()-1)) x1_int=int(Rows[0].Data.Size()-1);

            if (x2_int<                         0) x2_int=                         0;
            if (x2_int>int(Rows[0].Data.Size()-1)) x2_int=int(Rows[0].Data.Size()-1);
        }
        else
        {
            // Wrap.
            x1_int=x1_int % Rows[0].Data.Size() + (x1_int>=0 ? 0 : Rows[0].Data.Size());
            x2_int=x2_int % Rows[0].Data.Size() + (x2_int>=0 ? 0 : Rows[0].Data.Size());
        }

        return Rows[0].Data[x1_int]*(1.0f-x_frac) + Rows[0].Data[x2_int]*x_frac;
    }
}


/******************/
/*** Expression ***/
/******************/

ExpressionT::SymbolsT::SymbolsT()
    : Time(0.0f),
      GenFloat(),
      GenInt()
{
    AmbientLightColor[0]=0.0f;
    AmbientLightColor[1]=0.0f;
    AmbientLightColor[2]=0.0f;
}


ExpressionT::ExpressionT(float FloatNumberValue_)
    : Type(FloatNumber),
      FloatNumberValue(FloatNumberValue_),
      IntNumberValue(0),
      Table(NULL),
      Child1(NULL),
      Child2(NULL)
{
}


ExpressionT::ExpressionT(int IntNumberValue_)
    : Type(IntNumber),
      FloatNumberValue(0.0),
      IntNumberValue(IntNumberValue_),
      Table(NULL),
      Child1(NULL),
      Child2(NULL)
{
}


ExpressionT::ExpressionT(TypeT Type_)
    : Type(Type_),
      FloatNumberValue(0.0),
      IntNumberValue(0),
      Table(NULL),
      Child1(NULL),
      Child2(NULL)
{
    assert(Type==SymbolTime || Type==SymbolALRed || Type==SymbolALGreen || Type==SymbolALBlue);
}


ExpressionT::ExpressionT(TextParserT& TP, const ArrayT<TableT*>& ListOfTables) /*throw (TextParserT::ParseError)*/
    : Type(FloatNumber),
      FloatNumberValue(0.0),
      IntNumberValue(0),
      Table(NULL),
      Child1(NULL),
      Child2(NULL)
{
    if (TP.IsAtEOF()) return;
    std::string Token=TP.GetNextToken();

    if (Token=="add")
    {
        Type=Add;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        Child2=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="sub")
    {
        Type=Sub;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        Child2=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="mul")
    {
        Type=Mul;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        Child2=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="div")
    {
        Type=Div;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=",") throw TextParserT::ParseError();
        Child2=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="int")
    {
        Type=CastInt;
        if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
        Child1=new ExpressionT(TP, ListOfTables);
        if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
    }
    else if (Token=="time")
    {
        Type=SymbolTime;
    }
    else if (Token=="ambientLightRed")
    {
        Type=SymbolALRed;
    }
    else if (Token=="ambientLightGreen")
    {
        Type=SymbolALGreen;
    }
    else if (Token=="ambientLightBlue")
    {
        Type=SymbolALBlue;
    }
    else if (Token.find("fParam")==0)
    {
        unsigned long Index=atoi(Token.c_str()+6);

        if (Token.length()>7 && Token.at(6)=='0') throw TextParserT::ParseError();  // Don't want leading 0s here.
        if (Index>0xFFFF) throw TextParserT::ParseError();

        Type=TypeT(GENERIC_FLOAT_START+Index);
    }
    else if (Token.find("iParam")==0)
    {
        unsigned long Index=atoi(Token.c_str()+6);

        if (Token.length()>7 && Token.at(6)=='0') throw TextParserT::ParseError();  // Don't want leading 0s here.
        if (Index>0xFFFF) throw TextParserT::ParseError();

        Type=TypeT(GENERIC_INT_START+Index);
    }
    else
    {
        // See if the token is a table name.
        for (unsigned long TableNr=0; TableNr<ListOfTables.Size(); TableNr++)
            if (Token==ListOfTables[TableNr]->Name)
            {
                Type=TableLookup;
                Table=new TableT(*ListOfTables[TableNr]);
                if (TP.GetNextToken()!="[") throw TextParserT::ParseError();
                Child1=new ExpressionT(TP, ListOfTables);
                if (TP.GetNextToken()!="]") throw TextParserT::ParseError();
                break;
            }

        if (Type!=TableLookup)
        {
            // Token was no table name, so lets hope that it is a number.
            if (Token=="+" || Token=="-") throw TextParserT::ParseError();

            unsigned long c;

            for (c=0; c<Token.length(); c++)
                if (!isdigit(Token.at(c)) && !(c==0 && (Token.at(c)=='+' || Token.at(c)=='-'))) break;

            // If we only found digits, preceeded by an optional '+' or '-', take Token as an integer, otherwise as a float.
            if (c>=Token.length()) { Type=IntNumber;   IntNumberValue  =      atoi(Token.c_str());  }
                              else { Type=FloatNumber; FloatNumberValue=float(atof(Token.c_str())); }
        }
    }
}


// Copy Constructor (Law of the Big Three).
ExpressionT::ExpressionT(const ExpressionT& Source)
{
    Type            =Source.Type;
    FloatNumberValue=Source.FloatNumberValue;
    IntNumberValue  =Source.IntNumberValue;
    Table           =Source.Table  ? new TableT     (*Source.Table ) : NULL;
    Child1          =Source.Child1 ? new ExpressionT(*Source.Child1) : NULL;
    Child2          =Source.Child2 ? new ExpressionT(*Source.Child2) : NULL;
}


// Destructor (Law of the Big Three).
ExpressionT::~ExpressionT()
{
    delete Table;
    delete Child1;
    delete Child2;
}


// Assignment Operator (Law of the Big Three).
ExpressionT& ExpressionT::operator = (const ExpressionT& Source)
{
    // Make sure that self-assignment is handled properly.
    if (this==&Source) return *this;

    Type            =Source.Type;
    FloatNumberValue=Source.FloatNumberValue;
    IntNumberValue  =Source.IntNumberValue;

    // This is not the most clever way of doing things, but self-assignment is
    // explicitly checked for above, and a failing new operator is fatal anyway.
    delete Table;  Table =Source.Table  ? new TableT     (*Source.Table ) : NULL;
    delete Child1; Child1=Source.Child1 ? new ExpressionT(*Source.Child1) : NULL;
    delete Child2; Child2=Source.Child2 ? new ExpressionT(*Source.Child2) : NULL;

    return *this;
}


std::string ExpressionT::GetString() const
{
    switch (Type)
    {
        case FloatNumber:
        {
            std::stringstream String;
            String << FloatNumberValue;
            return String.str();
        }

        case IntNumber:
        {
            std::stringstream String;
            String << IntNumberValue;
            return String.str();
        }

        case SymbolTime:
            return "time";

        case SymbolALRed:
            return "ambientLightRed";

        case SymbolALGreen:
            return "ambientLightGreen";

        case SymbolALBlue:
            return "ambientLightBlue";

        default:
            return "<expr>";
    }
}


ExpressionT::ResultT ExpressionT::Evaluate(const ExpressionT::SymbolsT& Symbols) const
{
    switch (Type)
    {
        case FloatNumber:   return ResultT(FloatNumberValue);
        case IntNumber:     return ResultT(IntNumberValue);
        case SymbolTime:    return ResultT(Symbols.Time);
        case SymbolALRed:   return ResultT(Symbols.AmbientLightColor[0]);
        case SymbolALGreen: return ResultT(Symbols.AmbientLightColor[1]);
        case SymbolALBlue:  return ResultT(Symbols.AmbientLightColor[2]);
        case TableLookup:   return ResultT(Table->Lookup(Child1->Evaluate(Symbols).GetAsFloat()));
        case CastInt:       return ResultT(Child1->Evaluate(Symbols).GetAsInt());

        case Add:
        {
            ResultT r1=Child1->Evaluate(Symbols);
            ResultT r2=Child2->Evaluate(Symbols);

            if (r1.Type==FloatNumber && r2.Type==FloatNumber) return ResultT(r1.ValueFloat + r2.ValueFloat);
            if (r1.Type==FloatNumber && r2.Type==  IntNumber) return ResultT(r1.ValueFloat + r2.ValueInt  );
            if (r1.Type==  IntNumber && r2.Type==FloatNumber) return ResultT(r1.ValueInt   + r2.ValueFloat);
            if (r1.Type==  IntNumber && r2.Type==  IntNumber) return ResultT(r1.ValueInt   + r2.ValueInt  );
        }

        case Sub:
        {
            ResultT r1=Child1->Evaluate(Symbols);
            ResultT r2=Child2->Evaluate(Symbols);

            if (r1.Type==FloatNumber && r2.Type==FloatNumber) return ResultT(r1.ValueFloat - r2.ValueFloat);
            if (r1.Type==FloatNumber && r2.Type==  IntNumber) return ResultT(r1.ValueFloat - r2.ValueInt  );
            if (r1.Type==  IntNumber && r2.Type==FloatNumber) return ResultT(r1.ValueInt   - r2.ValueFloat);
            if (r1.Type==  IntNumber && r2.Type==  IntNumber) return ResultT(r1.ValueInt   - r2.ValueInt  );
        }

        case Mul:
        {
            ResultT r1=Child1->Evaluate(Symbols);
            ResultT r2=Child2->Evaluate(Symbols);

            if (r1.Type==FloatNumber && r2.Type==FloatNumber) return ResultT(r1.ValueFloat * r2.ValueFloat);
            if (r1.Type==FloatNumber && r2.Type==  IntNumber) return ResultT(r1.ValueFloat * r2.ValueInt  );
            if (r1.Type==  IntNumber && r2.Type==FloatNumber) return ResultT(r1.ValueInt   * r2.ValueFloat);
            if (r1.Type==  IntNumber && r2.Type==  IntNumber) return ResultT(r1.ValueInt   * r2.ValueInt  );
        }

        case Div:
        {
            ResultT r1=Child1->Evaluate(Symbols);
            ResultT r2=Child2->Evaluate(Symbols);

            if (r1.Type==FloatNumber && r2.Type==FloatNumber) return ResultT(r2.ValueFloat!=0.0 ? r1.ValueFloat / r2.ValueFloat : 0.0f);
            if (r1.Type==FloatNumber && r2.Type==  IntNumber) return ResultT(r2.ValueInt  !=0   ? r1.ValueFloat / r2.ValueInt   : 0.0f);
            if (r1.Type==  IntNumber && r2.Type==FloatNumber) return ResultT(r2.ValueFloat!=0.0 ? r1.ValueInt   / r2.ValueFloat : 0.0f);
            if (r1.Type==  IntNumber && r2.Type==  IntNumber) return ResultT(r2.ValueInt  !=0   ? r1.ValueInt   / r2.ValueInt   : 0   );
        }

        default:
        {
            const unsigned long Index=Type & 0x0000FFFF;

            if ((Type & 0xFFFF0000)==GENERIC_FLOAT_START && Index<Symbols.GenFloat.Size()) return ResultT(Symbols.GenFloat[Index]);
            if ((Type & 0xFFFF0000)==GENERIC_INT_START   && Index<Symbols.GenInt  .Size()) return ResultT(Symbols.GenInt  [Index]);

            // If we ever get here, Type has an unknown value! (Or Index was just too large.)
        }
    }

    return ResultT(0);
}
