/*******************************/
/*** Integer Matrix (Header) ***/
/*******************************/


#ifndef _MATRIX_HPP_
#define _MATRIX_HPP_


// A very specialized matrix class for node-to-node movement computations.
// OPTIMIZE: The last trivial line (0 0 0 1) can be dropped, yielding m of dimensions m[3][4]!
class IntMatrixT
{
    public:

    int m[4][4];


    // Constructor: Erzeugt eine neue Matrix und initialisiert sie mit der Identitaet.
    IntMatrixT();

    // Gibt die durch 'T' definierte Translationsmatrix zurueck.
    static IntMatrixT GetTranslationMatrix(int tx, int ty, int tz);

    // Gibt diejenige Matrix zurueck, die eine Drehung um 'Angle' (in Grad) um die X-Achse definiert.
    static IntMatrixT GetRotationXMatrix(int Angle);

    // Gibt diejenige Matrix zurueck, die eine Drehung um 'Angle' (in Grad) um die Y-Achse definiert.
    static IntMatrixT GetRotationYMatrix(int Angle);

    // Gibt diejenige Matrix zurueck, die eine Drehung um 'Angle' (in Grad) um die Z-Achse definiert.
    static IntMatrixT GetRotationZMatrix(int Angle);

    // Gibt das Ergebnis des Matrizenprodukts A*B zurueck ('operator *' waere auch denkbar...)
    friend IntMatrixT operator * (const IntMatrixT& A, const IntMatrixT& B);

    // Gibt das Ergebnis des Matrizenprodukts A*B zurueck ('operator *' waere auch denkbar...)
    friend bool operator == (const IntMatrixT& A, const IntMatrixT& B);
};

#endif
