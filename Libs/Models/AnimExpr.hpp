/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

    /// Advances the time for this anim expression,
    /// i.e.\ frame numbers of the referenced animation sequences, cross-fades, etc.
    virtual void AdvanceTime(float Time, bool ForceLoop=false) { }

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


class AnimExprStandardT : public AnimExpressionT
{
    public:

    AnimExprStandardT(const CafuModelT& Model, int SequNr, float FrameNr);

    // Implementations and overrides for base class methods.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const;
    virtual void AdvanceTime(float Time, bool ForceLoop=false);
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


    private:

    void NormalizeInput();

    int   m_SequNr;     ///< The animation sequence number.
    float m_FrameNr;    ///< The frame number in the sequence.
};


/// Filters the result of another expression by a "channel".
class AnimExprFilterT : public AnimExpressionT
{
    public:

    AnimExprFilterT(const CafuModelT& Model, AnimExpressionPtrT SubExpr, unsigned int ChannelNr);
    AnimExprFilterT(const CafuModelT& Model, AnimExpressionPtrT SubExpr, const std::string& ChannelName);

    // Implementations and overrides for base class methods.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const;
    virtual void AdvanceTime(float Time, bool ForceLoop=false) { m_SubExpr->AdvanceTime(Time, ForceLoop); }
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
    virtual void AdvanceTime(float Time, bool ForceLoop=false);
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
    virtual void AdvanceTime(float Time, bool ForceLoop=false);
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
