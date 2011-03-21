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

/*********************/
/*** Array Library ***/
/*********************/

#ifndef _CA_ARRAY_HPP_
#define _CA_ARRAY_HPP_

#include <stdlib.h>
#include <cassert>

#if defined(_WIN32) && defined(_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for

        // Turn off warning 4786: "Bezeichner wurde auf '255' Zeichen in den Debug-Informationen reduziert."
        #pragma warning(disable:4786)
    #endif
#endif


// These classes are intentionally not defined in ArrayT<T>,
// because we don't need them parametrised by T.
class cfArrayError {};                                          ///< General array error.
class cfOutOfRange   : public cfArrayError {};                  ///< Array index exceeds array boundaries.
class cfSizeOverflow : public cfArrayError {};                  ///< Overflow of the arrays size.


template<class T> class ArrayT
{
    private:

    unsigned long MaxNrOfElements;
    unsigned long NrOfElements;
    T*            Elements;


    public:

    ArrayT();                                                   ///< Usual constructor
    ArrayT(const ArrayT<T>& OldArray);                          ///< Copy  constructor
   ~ArrayT();                                                   ///< Destructor
    ArrayT<T>& operator = (const ArrayT<T>& OldArray);          ///< Assignment operator

    unsigned long Size() const;                                 ///< Get size of array
    void          Clear();                                      ///< Clear array (and free allocated memory)
    void          Overwrite();                                  ///< Clear array (but reuse memory)
    T&            operator [] (unsigned long Index);
    const T&      operator [] (unsigned long Index) const;
    void          PushBack(const T Element);
    void          PushBackEmpty(unsigned long Amount=1);
    void          PushBackEmptyExact(unsigned long Amount=1);
    void          DeleteBack(unsigned long Amount=1);

    // "Convenience" methods.
    void          PushBack(const ArrayT<T>& Other);
    void          InsertAt(unsigned long Index, const T Element);   // TODO: Rename to InsertAtAndKeepOrder()
    void          RemoveAt(unsigned long Index);
    void          RemoveAtAndKeepOrder(unsigned long Index);
    void          QuickSort(bool (*IsLess)(const T& E1, const T& E2));
 // void          QuickSort(unsigned long FirstIndex, unsigned long LastIndex, bool (*IsLess)(const T& E1, const T& E2));
    int           Find(const T& Element) const;
};


template<class T> inline ArrayT<T>::ArrayT()                                                // Usual constructor
{
    MaxNrOfElements=0;
    NrOfElements   =0;
    Elements       =NULL;
}


template<class T> inline ArrayT<T>::ArrayT(const ArrayT<T>& OldArray)                       // Copy constructor
{
    MaxNrOfElements=OldArray.MaxNrOfElements;
    NrOfElements   =OldArray.NrOfElements;
    Elements       =MaxNrOfElements>0 ? new T[MaxNrOfElements] : NULL;

    for (unsigned long Nr=0; Nr<NrOfElements; Nr++) Elements[Nr]=OldArray.Elements[Nr];
}


template<class T> inline ArrayT<T>::~ArrayT()                                               // Destructor
{
    delete[] Elements;
}


template<class T> inline ArrayT<T>& ArrayT<T>::operator = (const ArrayT<T>& OldArray)       // Assignment op
{
    // Handles self-assignment implicitly right!
    T* NewElements=OldArray.MaxNrOfElements>0 ? new T[OldArray.MaxNrOfElements] : NULL;
    for (unsigned long Nr=0; Nr<OldArray.NrOfElements; Nr++) NewElements[Nr]=OldArray.Elements[Nr];

    delete[] Elements;

    MaxNrOfElements=OldArray.MaxNrOfElements;
    NrOfElements   =OldArray.NrOfElements;
    Elements       =NewElements;
    return *this;
}


template<class T> inline unsigned long ArrayT<T>::Size() const
{
    return NrOfElements;
}


template<class T> inline void ArrayT<T>::Clear()
{
    delete[] Elements;

    MaxNrOfElements=0;
    NrOfElements   =0;
    Elements       =NULL;
}


template<class T> inline void ArrayT<T>::Overwrite()
{
    // for (unsigned long Nr=0; Nr<NrOfElements; Nr++) Elements[Nr]=T();
    NrOfElements=0;
}


template<class T> inline T& ArrayT<T>::operator [] (unsigned long Index)
{
    assert(Index<NrOfElements);

    return Elements[Index];
}


template<class T> inline const T& ArrayT<T>::operator [] (unsigned long Index) const
{
    assert(Index<NrOfElements);

    return Elements[Index];
}


// Der Parameter muß hier als Kopie übergeben werden, nicht als Referenz. Wenn man einem Array mit PushBack ein eigenes
// Element anhängen möchte (z.B. wie in Array.PushBack(Array[0]);), kann es ansonsten zu einem Fehler führen, wenn das
// Array gleichzeitig reallokiert und verschoben werden muß!
template<class T> inline void ArrayT<T>::PushBack(const T Element)
{
    NrOfElements++;

    if (NrOfElements>MaxNrOfElements)
    {
        if (!MaxNrOfElements) MaxNrOfElements=1;
        while (MaxNrOfElements<NrOfElements)
        {
            if (MaxNrOfElements>=0x80000000) throw cfSizeOverflow();
            MaxNrOfElements*=2;
        }

        T* NewElements=new T[MaxNrOfElements];
        for (unsigned long Nr=0; Nr<NrOfElements-1; Nr++) NewElements[Nr]=Elements[Nr];

        delete[] Elements;
        Elements=NewElements;
    }

    Elements[NrOfElements-1]=Element;
}


template<class T> inline void ArrayT<T>::PushBackEmpty(unsigned long Amount)
{
    NrOfElements+=Amount;

    if (NrOfElements>MaxNrOfElements)
    {
        if (!MaxNrOfElements) MaxNrOfElements=1;
        while (MaxNrOfElements<NrOfElements)
        {
            if (MaxNrOfElements>=0x80000000) throw cfSizeOverflow();
            MaxNrOfElements*=2;
        }

        T* NewElements=new T[MaxNrOfElements];
        for (unsigned long Nr=0; Nr<NrOfElements-Amount; Nr++) NewElements[Nr]=Elements[Nr];

        delete[] Elements;
        Elements=NewElements;
    }
}


template<class T> inline void ArrayT<T>::PushBackEmptyExact(unsigned long Amount)
{
    NrOfElements+=Amount;

    if (NrOfElements>MaxNrOfElements)
    {
        MaxNrOfElements=NrOfElements;

        T* NewElements=new T[MaxNrOfElements];
        for (unsigned long Nr=0; Nr<NrOfElements-Amount; Nr++) NewElements[Nr]=Elements[Nr];

        delete[] Elements;
        Elements=NewElements;
    }
}


template<class T> inline void ArrayT<T>::DeleteBack(unsigned long Amount)
{
    while (Amount>0 && NrOfElements>0)
    {
        Elements[NrOfElements-1]=T();
        NrOfElements--;
        Amount--;
    }
}


template<class T> inline void ArrayT<T>::PushBack(const ArrayT<T>& Other)
{
    for (unsigned long Nr=0; Nr<Other.Size(); Nr++)
        PushBack(Other[Nr]);
}


template<class T> inline void ArrayT<T>::InsertAt(unsigned long Index, const T Element)
{
    assert(Index<=NrOfElements);    // This is intentionally <= and not <, because we immediately add an element below.

    PushBackEmpty();

    for (unsigned long Nr=NrOfElements-1; Nr>Index; Nr--)
        Elements[Nr]=Elements[Nr-1];

    Elements[Index]=Element;
}


template<class T> inline void ArrayT<T>::RemoveAt(unsigned long Index)
{
    assert(Index<NrOfElements);

    Elements[Index]=Elements[NrOfElements-1];
    DeleteBack();
}


template<class T> inline void ArrayT<T>::RemoveAtAndKeepOrder(unsigned long Index)
{
    assert(Index<NrOfElements);

    for (unsigned long Nr=Index; Nr+1<NrOfElements; Nr++)
        Elements[Nr]=Elements[Nr+1];

    DeleteBack();
}


template<class T> inline void ArrayT<T>::QuickSort(bool (*IsLess)(const T& E1, const T& E2))
{
    static ArrayT<unsigned long> ToDoRanges;

    if (NrOfElements<=1) return;

    ToDoRanges.Overwrite();
    ToDoRanges.PushBack(0);
    ToDoRanges.PushBack(NrOfElements-1);


    while (ToDoRanges.Size()>=2)
    {
        const unsigned long LastIndex =ToDoRanges[ToDoRanges.Size()-1]; ToDoRanges.DeleteBack();
        const unsigned long FirstIndex=ToDoRanges[ToDoRanges.Size()-1]; ToDoRanges.DeleteBack();

        if (FirstIndex<LastIndex)
        {
            const T&      x=Elements[LastIndex];
            unsigned long i=FirstIndex-1;

            for (unsigned long j=FirstIndex; j<=LastIndex-1; j++)
                if (IsLess(Elements[j], x))
                {
                    i++;
                    const T Swap=Elements[i];
                    Elements[i]=Elements[j];
                    Elements[j]=Swap;
                }

            const T Swap=Elements[i+1];
            Elements[i+1]=Elements[LastIndex];
            Elements[LastIndex]=Swap;

            i++;

            ToDoRanges.PushBack(i+1); ToDoRanges.PushBack(LastIndex);
            if (i>0) { ToDoRanges.PushBack(FirstIndex); ToDoRanges.PushBack(i-1); }
        }
    }
}


/* // Simple recursive implementation from school.
template<class T> inline void ArrayT<T>::QuickSort(unsigned long FirstIndex, unsigned long LastIndex, bool (*IsLess)(const T& E1, const T& E2))
{
    if (FirstIndex<LastIndex)
    {
        const T&      x=Elements[LastIndex];
        unsigned long i=FirstIndex-1;

        for (unsigned long j=FirstIndex; j<=LastIndex-1; j++)
            if (IsLess(Elements[j], x))
            {
                i++;
                const T Swap=Elements[i];
                Elements[i]=Elements[j];
                Elements[j]=Swap;
            }

        const T Swap=Elements[i+1];
        Elements[i+1]=Elements[LastIndex];
        Elements[LastIndex]=Swap;

        i++;

        if (i>0) QuickSort(FirstIndex, i-1, IsLess);
        QuickSort(i+1, LastIndex, IsLess);
    }
} */


template<class T> inline int ArrayT<T>::Find(const T& Element) const
{
    for (unsigned long Nr=0; Nr<NrOfElements; Nr++)
        if (Elements[Nr]==Element)
            return Nr;

    return -1;
}

#endif
