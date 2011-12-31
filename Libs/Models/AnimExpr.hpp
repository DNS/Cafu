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

#ifndef _CAFU_MODEL_ANIM_EXPRESSION_HPP_
#define _CAFU_MODEL_ANIM_EXPRESSION_HPP_

#include "Math3D/Quaternion.hpp"


class CafuModelT;


class AnimExpressionT
{
    public:

    /// The constructor.        TODO: Make protected, so that only derived classes can use it?
    AnimExpressionT(const CafuModelT& Model) : m_Model(Model) { }

    /// The (virtual) destructor.
    virtual ~AnimExpressionT() { }

    /// Returns the model that this is an anim expression for.
    const CafuModelT& GetModel() const { return m_Model; }

    /// Returns a number that changes whenever this expression changes,
    /// or more precisely, that changes whenever the data returned by GetData() changes.
    /// This is the case for example after every call to AdvanceTime() with a nonzero Time,
    /// or when a sub-expression has been modified (e.g. got a new sequence number assigned).
    /// The caller can use this number in order to control refreshes of its mesh caches.
    virtual unsigned int GetChangeCount() const=0;

    /// For the joint with the given JointNr, this function returns
    ///   - the joint weight,
    ///   - the joints position, quaternion and scale values.
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const=0;

    /// Advances the frame numbers of the referenced animation sequences, cross-fades, etc.
    virtual void AdvanceTime(float Time, bool ForceLoop=false) { }


    private:

    AnimExpressionT(const AnimExpressionT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const AnimExpressionT&);       ///< Use of the Assignment Operator is not allowed.

    const CafuModelT& m_Model;  ///< The related model that this is an anim expression for.
};


class AnimExprStandardT : public AnimExpressionT
{
    public:

    AnimExprStandardT(const CafuModelT& Model, int SequNr, float FrameNr);

    // Implementations and overrides for base class methods.
    virtual unsigned int GetChangeCount() const { return m_ChangeCount; }
    virtual void GetData(unsigned int JointNr, float& Weight, Vector3fT& Pos, cf::math::QuaternionfT& Quat, Vector3fT& Scale) const;
    virtual void AdvanceTime(float Time, bool ForceLoop=false);

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

    int          m_SequNr;      ///< The animation sequence number.
    float        m_FrameNr;     ///< The frame number in the sequence.
    unsigned int m_ChangeCount; ///< Changes whenever the data returned by GetData() might have changed.
};

#endif
