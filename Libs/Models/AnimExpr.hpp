/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODEL_ANIM_EXPRESSION_HPP_INCLUDED
#define CAFU_MODEL_ANIM_EXPRESSION_HPP_INCLUDED

#include "Math3D/Quaternion.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"


class AnimExpressionT;
class CafuModelT;

typedef IntrusivePtrT<AnimExpressionT> AnimExpressionPtrT;


/// Animation expressions describe the "skeleton pose" of a model.
///
/// They are used as an input to AnimPoseT instances, which use the "skeleton pose" expressed by an anim
/// expression in order to derive the "full pose", including meshes and other features such as collision
/// detection.
/// In other words, AnimPoseT's refer to an AnimExpressionT for the "configuration" of its pose.
///
/// AnimExpressionT's can be hierarchically composed, just like mathematical expressions,
/// which is in fact their strongest and most important feature.
/// They are also very easy and care-free to use and have very good performance, because when obtained
/// from an AnimExprPoolT, the pool minimizes both the number of instances as well as the penalties from
/// memory allocations and deletes.
class AnimExpressionT
{
    public:

    /// The constructor.
    /// (It's ok to have this in "public" instead of "protected": we have pure virtual methods as well.)
    AnimExpressionT(const CafuModelT& Model);

    /// The (virtual) destructor.
    virtual ~AnimExpressionT() { }

    /// Returns the model that this is an anim expression for.
    const CafuModelT& GetModel() const { return m_Model; }

    /// Returns the number of IntrusivePtrT<>'s that currently refer to this anim expression.
    /// This is especially useful for the implementation of "pools" of AnimExpressionT's,
    /// so that the pool implementation can learn if this instance is available for being re-used.
    unsigned int GetRefCount() const { return m_RefCount; }

    /// For the joint with the given JointNr, this function returns
    ///   - the joint weight,
    ///   - the joints position, quaternion and scale values.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const=0;

    /// Advances the time for this anim expression, that is, frame numbers of
    /// underlying animation sequences, cross-fades, etc.
    /// Returns `true` if the end of an underlying animation sequence was reached
    /// (or in case of a looping sequence, if the sequence was wrapped).
    virtual bool AdvanceTime(float Time) { return false; }

    /// The virtual copy constructor.
    /// Creates a new anim expression that is an exact copy of this, even when called
    /// via the base class pointer (the caller doesn't need to know the exact derived class).
    virtual AnimExpressionPtrT Clone() const=0;

    /// Returns whether this anim expression is equal to \c A.
    /// Two anim expressions are equal if their GetData() methods return the same data.
    virtual bool IsEqual(const AnimExpressionPtrT& AE) const=0;


    private:

    template<class T> friend class IntrusivePtrT;

    AnimExpressionT(const AnimExpressionT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const AnimExpressionT&);       ///< Use of the Assignment Operator is not allowed.

    const CafuModelT& m_Model;      ///< The related model that this is an anim expression for.
    unsigned int      m_RefCount;   ///< How many IntrusivePtrT<>'s currently refer to this anim expression?
};


/// This class implements the "standard" skeleton pose based on a sequence number and frame number.
///
/// Note that for backwards-compatibility reasons, this expression never progresses into "other" sequences.
/// That is, for any Model.GetAnims()[SequNr].Next, only the values -1 and SequNr are supported; any other
/// non-negative value is treated as if it was SequNr.
/// This restriction is in place because we have a lot of non-trivial user code that relies on obtaining
/// the same value from GetSequNr() that was last set with SetSequNr().
/// If in the future any user code requires following the "Next" sequences properly, a new AnimExpressionT-
/// derived class is required that fully implements this.
class AnimExprStandardT : public AnimExpressionT
{
    public:

    /// Create a new "standard" expression from the given parameters.
    /// The "force loop" setting for the newly created expression is \c false.
    AnimExprStandardT(const CafuModelT& Model, int SequNr, float FrameNr);

    // Implementations and overrides for base class methods.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const;
    virtual bool AdvanceTime(float Time);
    virtual AnimExpressionPtrT Clone() const;   // Unfortunately, the proper covariant return type cannot be used with smart pointers.
    virtual bool IsEqual(const AnimExpressionPtrT& AE) const;

    /// Returns the sequence number that is currently set in this expression.
    int GetSequNr() const { return m_SequNr; }

    /// Sets the number of the animation sequence that is used by this expression.
    /// @param SequNr   The number of the animation sequence to use, -1 for the bind pose.
    void SetSequNr(int SequNr);

    /// Returns the frame number that is currently set in this expression.
    float GetFrameNr() const { return m_FrameNr; }

    /// Sets the frame number in the current animation sequence.
    /// @param FrameNr   The frame number in the animation sequence to use.
    void SetFrameNr(float FrameNr);

    /// Returns if this expression will override the "next" setting at the end of the current animation sequence.
    bool GetForceLoop() const { return m_ForceLoop; }

    /// Sets whether this expression should override the "next" setting at the end of the current animation sequence.
    /// @param ForceLoop   Override the "next" setting at the end of the current animation sequence?
    void SetForceLoop(bool ForceLoop);


    private:

    void NormalizeInput();

    int   m_SequNr;     ///< The animation sequence number.
    float m_FrameNr;    ///< The frame number in the sequence.
    bool  m_ForceLoop;  ///< Override the "next" setting at the end of the current animation sequence?
};


/// Filters the result of another expression by a "channel".
class AnimExprFilterT : public AnimExpressionT
{
    public:

    AnimExprFilterT(const CafuModelT& Model, AnimExpressionPtrT SubExpr, unsigned int ChannelNr);
    AnimExprFilterT(const CafuModelT& Model, AnimExpressionPtrT SubExpr, const std::string& ChannelName);

    // Implementations and overrides for base class methods.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const;
    virtual bool AdvanceTime(float Time) { return m_SubExpr->AdvanceTime(Time); }
    virtual AnimExpressionPtrT Clone() const;   // Unfortunately, the proper covariant return type cannot be used with smart pointers.
    virtual bool IsEqual(const AnimExpressionPtrT& AE) const;

    /// Re-initializes this anim expression, so that it can be re-used with different parameters (on the same model).
    void ReInit(AnimExpressionPtrT SubExpr, unsigned int ChannelNr);


    private:

    AnimExpressionPtrT m_SubExpr;
    unsigned int       m_ChannelNr;
};


class AnimExprCombineT : public AnimExpressionT
{
    public:

    AnimExprCombineT(const CafuModelT& Model, AnimExpressionPtrT A, AnimExpressionPtrT B);

    // Implementations and overrides for base class methods.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const;
    virtual bool AdvanceTime(float Time);
    virtual AnimExpressionPtrT Clone() const;   // Unfortunately, the proper covariant return type cannot be used with smart pointers.
    virtual bool IsEqual(const AnimExpressionPtrT& AE) const;

    /// Re-initializes this anim expression, so that it can be re-used with different parameters (on the same model).
    void ReInit(AnimExpressionPtrT A, AnimExpressionPtrT B);


    private:

    AnimExpressionPtrT m_A;
    AnimExpressionPtrT m_B;
};


class AnimExprBlendT : public AnimExpressionT
{
    public:

    AnimExprBlendT(const CafuModelT& Model, AnimExpressionPtrT A, AnimExpressionPtrT B, float Duration);

    // Implementations and overrides for base class methods.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const;
    virtual bool AdvanceTime(float Time);
    virtual AnimExpressionPtrT Clone() const;   // Unfortunately, the proper covariant return type cannot be used with smart pointers.
    virtual bool IsEqual(const AnimExpressionPtrT& AE) const;

    /// Re-initializes this anim expression, so that it can be re-used with different parameters (on the same model).
    /// Note that resetting \c A, \c B or \c Duration individually is not possible, because the implementation
    /// may prune and drop \c A when the blend is complete.
    void ReInit(AnimExpressionPtrT A, AnimExpressionPtrT B, float Duration);

    /// Returns the "blend from" sub-expression.
    AnimExpressionPtrT GetA() const { return m_A; }

    /// Returns the "blend to" sub-expression.
    AnimExpressionPtrT GetB() const { return m_B; }

    /// Returns how far the blend has advanced.
    float GetFrac() const { return m_Frac; }


    private:

    AnimExpressionPtrT m_A;
    AnimExpressionPtrT m_B;
    float              m_Duration;
    float              m_Frac;
};


class AnimExprPoolT
{
    public:

    AnimExprPoolT(const CafuModelT& Model) : m_Model(Model) { }

    // These methods mimic the ctors of the anim expression classes.
    IntrusivePtrT<AnimExprStandardT> GetStandard(int SequNr, float FrameNr);
    IntrusivePtrT<AnimExprFilterT>   GetFilter(AnimExpressionPtrT SubExpr, unsigned int ChannelNr);
    IntrusivePtrT<AnimExprFilterT>   GetFilter(AnimExpressionPtrT SubExpr, const std::string& ChannelName);
    IntrusivePtrT<AnimExprCombineT>  GetCombine(AnimExpressionPtrT A, AnimExpressionPtrT B);
    IntrusivePtrT<AnimExprBlendT>    GetBlend(AnimExpressionPtrT A, AnimExpressionPtrT B, float Duration);


    private:

    /// This function makes sure that anim expressions that are unused don't keep pointers
    /// to subexpressions, such that the subexpressions are available as unused as well.
    void FlattenUnused();

    const CafuModelT&                          m_Model;     ///< The related model that this is an anim expression pool for.
    ArrayT< IntrusivePtrT<AnimExprStandardT> > m_Standard;
    ArrayT< IntrusivePtrT<AnimExprFilterT> >   m_Filter;
    ArrayT< IntrusivePtrT<AnimExprCombineT> >  m_Combine;
    ArrayT< IntrusivePtrT<AnimExprBlendT> >    m_Blend;
};

#endif
