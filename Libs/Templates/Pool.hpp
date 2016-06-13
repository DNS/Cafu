/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_POOL_HPP_INCLUDED
#define CAFU_POOL_HPP_INCLUDED


namespace cf
{
    /// This class manages memory for a pool of objects.
    /// Works best for POD- and simple class types, because:
    /// - Only calls the default ctor.
    /// - Does not really free the elements, only when the entire pool is deleted.
    /// - Works best when only small arrays (with few elements) are alloc'ed, so that there is not too much Verschnitt.
    /// - GetSize() does not take calls Free() into account.
    template<class T> class PoolNoFreeT
    {
        public:

        PoolNoFreeT(unsigned long SizeOfFirstBlock=8) : SizeOfNextBlock(SizeOfFirstBlock), ElementsRemaining(0), UserSize(0)
        {
        }

        ~PoolNoFreeT()
        {
            // TODO: It would be great if we could assert that each call to Alloc() has been matched by a call to Free() when we get here.
            for (unsigned long BlockNr=0; BlockNr<Blocks.Size(); BlockNr++)
                delete[] Blocks[BlockNr];
        }

        /// Returns a pointer to Size allocated objects of type T.
        T* Alloc(unsigned long Size=1)
        {
            if (Size==0) return NULL;

            if (Size>ElementsRemaining)
            {
                // There are not enough elements left for an array of length Size.
                // First make sure that the next block *will* have enough elements.
                while (SizeOfNextBlock<Size) SizeOfNextBlock*=2;

                Blocks.PushBack(new T[SizeOfNextBlock]);
                ElementsRemaining=SizeOfNextBlock;

                SizeOfNextBlock*=2;
            }

            ElementsRemaining-=Size;
            UserSize+=Size;
            return Blocks[Blocks.Size()-1]+ElementsRemaining;
        }

        void Free(T* /*Array*/)
        {
            // Intentionally do nothing here - we can't do anything useful anyway, that is.
            // TODO: assert(Array is actually from one of our Blocks);
            // TODO: Update the UserSize variable!
        }

        unsigned long GetSize() const
        {
            return UserSize;
        }


        private:

        PoolNoFreeT(const PoolNoFreeT<T>&);         ///< Use of the Copy Constructor    is not allowed.
        void operator = (const PoolNoFreeT<T>&);    ///< Use of the Assignment Operator is not allowed.

        unsigned long SizeOfNextBlock;
        unsigned long ElementsRemaining;    ///< How many elements are still unused and thus free for allocation in the last block.
        unsigned long UserSize;             ///< The total number of elements requested with and returned by Alloc().
        ArrayT<T*>    Blocks;
    };


    /// This class manages memory for a pool of objects.
    /// Works best for POD- and simple class types, because:
    /// - Doesn't really care about ctor and dtor calls.
    /// - Can only allocate a single element at a time.
    template<class T> class PoolSingleT
    {
        public:

        PoolSingleT(unsigned long SizeOfFirstBlock=8) : SizeOfNextBlock(SizeOfFirstBlock), ElementsRemaining(0), AllocedElemCount(0)
        {
            FreeElements.PushBackEmpty(SizeOfFirstBlock*3);     // Reserve enough for first and second block.
            FreeElements.Overwrite();
        }

        ~PoolSingleT()
        {
            // If AllocedElemCount!=0 here, the user is leaking memory.
            assert(AllocedElemCount==0);

            for (unsigned long BlockNr=0; BlockNr<Blocks.Size(); BlockNr++)
                delete[] Blocks[BlockNr];
        }

        /// Returns a pointer to an objects of type T.
        T* Alloc()
        {
            AllocedElemCount++;

            if (FreeElements.Size()>0)
            {
                T* Element=FreeElements[FreeElements.Size()-1];

                FreeElements.DeleteBack();
                // Note: No constructor call for Element here.
                return Element;
            }

            if (ElementsRemaining>0)
            {
                ElementsRemaining--;
                return Blocks[Blocks.Size()-1]+ElementsRemaining;
            }

            // No elements were left in the last block.
            assert(SizeOfNextBlock>0);
            Blocks.PushBack(new T[SizeOfNextBlock]);
            ElementsRemaining=SizeOfNextBlock;

            SizeOfNextBlock*=2;

            ElementsRemaining--;
            return Blocks[Blocks.Size()-1]+ElementsRemaining;
        }

        void Free(T* Element)
        {
            // Note: No destructor call for Element here.
            FreeElements.PushBack(Element);
            AllocedElemCount--;
        }


        private:

        PoolSingleT(const PoolSingleT<T>&);         ///< Use of the Copy Constructor    is not allowed.
        void operator = (const PoolSingleT<T>&);    ///< Use of the Assignment Operator is not allowed.

        unsigned long SizeOfNextBlock;  ///< How many elements the next allocated block will have.
        unsigned long ElementsRemaining;///< How many elements are still unused and thus free for allocation in the last block.
        ArrayT<T*>    Blocks;           ///< The actual blocks of allocated T.
        ArrayT<T*>    FreeElements;     ///< Instances of T that have been freed and are thus ready for reuse.
        unsigned long AllocedElemCount; ///< The number of elements that are currently been allocated by the user.
    };
}



/*
// Variante 1:
// - Only calls the default ctor.
// - Cannot alloc arrays (only just one).

// Variante 2:
// - Only calls the default ctor.
// - Can alloc arrays, but not individually free().

template<class T> class PoolT
{
    public:

    PoolT();    ///< Constructor.
    ~PoolT();   ///< Destructor.

    /// Returns a pointer to Size allocated objects of type T.
    T* alloc(unsigned long Size=1);

    void free(T*);


    private:

    PoolT(const PoolT<T>&);             ///< Use of the Copy Constructor    is not allowed.
    void operator = (const PoolT<T>&);  ///< Use of the Assignment Operator is not allowed.

    ArrayT<T*> Elements;
}; */

#endif
