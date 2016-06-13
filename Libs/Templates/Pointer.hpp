/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SMART_POINTER_HPP_INCLUDED
#define CAFU_SMART_POINTER_HPP_INCLUDED


/// A base class for objects that are reference-counted with IntrusivePtrTs.
/// See http://www.drdobbs.com/article/print?articleId=229218807 for details.
class RefCountedT
{
    public:

    unsigned int GetRefCount() const { return m_RefCount; }


    protected:

    RefCountedT() : m_RefCount(0) { }
    RefCountedT(const RefCountedT&) : m_RefCount(0) { }
    RefCountedT& operator = (const RefCountedT&) { return *this; }


    private:

    template<class T> friend class IntrusivePtrT;

    /// m_RefCount is mutable, so that an IntrusivePtrT<const T> can still modify it.
    mutable unsigned int m_RefCount;
};


/// This class implements smart (reference-counted) pointers.
///
/// The implementation is intrusive: It requires from the class \c T that it is used with
/// that it keeps a member \c m_RefCount that it can access either publicly or as a friend.
///
/// This class might be replaced by <code>boost::intrusive_ptr<></code> at a later time.
template<class T>
class IntrusivePtrT
{
    public:

    /// The constructor.
    IntrusivePtrT(T* Ptr = 0)
        : m_Ptr(Ptr)
    {
        if (m_Ptr) m_Ptr->m_RefCount++;
    }

    /// The copy constructor.
    IntrusivePtrT(const IntrusivePtrT& IP)
        : m_Ptr(IP.m_Ptr)
    {
        if (m_Ptr) m_Ptr->m_RefCount++;
    }

    /// The copy constructor (for Y classes that are derived from T).
    template<class Y> IntrusivePtrT(const IntrusivePtrT<Y>& IP)
        : m_Ptr(IP.get())
    {
        if (m_Ptr) m_Ptr->m_RefCount++;
    }

    /// The destructor.
    ~IntrusivePtrT()
    {
        if (m_Ptr)
        {
            m_Ptr->m_RefCount--;
            if (m_Ptr->m_RefCount == 0) delete m_Ptr;
        }
    }

    /// The assignment operator.
    IntrusivePtrT& operator = (const IntrusivePtrT& IP)
    {
        // Do not change the order of these statements!
        // This order properly handles self-assignment as well as recursion,
        // e.g. when an T contains IntrusivePtrT's.
        T* const Old=m_Ptr;

        m_Ptr = IP.m_Ptr;
        if (m_Ptr) m_Ptr->m_RefCount++;

        if (Old)
        {
            Old->m_RefCount--;
            if (Old->m_RefCount == 0) delete Old;
        }

        return *this;
    }

    /// Returns the stored pointer.
    T* get() const { return m_Ptr; }

    /// The structure dereference operator.
    T* operator -> () const { return m_Ptr; }

    /// The dereference operator.
    T& operator * () { return *m_Ptr; }

    /// The dereference operator (const).
    const T& operator * () const { return *m_Ptr; }

    /// The equality operator.
    bool operator == (const IntrusivePtrT& IP) const { return m_Ptr == IP.m_Ptr; }
    template<class U> bool operator == (U* b) const { return m_Ptr == b; }

    /// The inequality operator.
    bool operator != (const IntrusivePtrT& IP) const { return m_Ptr != IP.m_Ptr; }
    template<class U> bool operator != (U* b) const { return m_Ptr != b; }

    /// A safe alternative for the bool conversion operator.
    /// For details, see:
    ///   - http://stackoverflow.com/questions/7226801/how-does-shared-ptr-work-in-if-condition
    ///   - http://stackoverflow.com/questions/6967448/does-msvc10-visual-studio-2010-support-c-explicit-conversion-operators
    bool IsNull() const { return m_Ptr == 0; }


    private:

    T* m_Ptr;   ///< The pointer to the reference-counted object.
};


template<class T, class U> IntrusivePtrT<T> static_pointer_cast(IntrusivePtrT<U> const& IP)
{
    return static_cast<T*>(IP.get());
}


template<class T, class U> IntrusivePtrT<T> dynamic_pointer_cast(IntrusivePtrT<U> const& IP)
{
    return dynamic_cast<T*>(IP.get());
}

#endif
