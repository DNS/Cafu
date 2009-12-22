/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

/*************/
/*** Model ***/
/*************/

#ifndef _MODEL_HPP_
#define _MODEL_HPP_

#include "Math3D/Vector3.hpp"
#include <string>


/// This class represents a generic model (or rather, the interface to a model).
/// All methods are const inspectors, as suggested by the FlyWeight and Proxy patterns that we employ with the models hierarchy:
/// The FlyWeight pattern requires common intrinsic state, and that the extrinsic state is passed via parameters.
/// The Proxy pattern requires copy-on-write semantics if several proxies represent a single "real" object.
class ModelT
{
    public:

    /// A class for throwing exceptions on load errors.
    class LoadError { };


    /// Returns the file name of this model.
    virtual const std::string& GetFileName() const=0;

    /// Draws the model in sequence SequenceNr at frame FrameNr. The current MatSys model-view matrix determines the position and orientation.
    /// If 'SubModel' points to another ModelT and this model supports sub-models, SubModel is drawn as a sub-model.
    /// @param SequenceNr The sequence to render this model in.
    /// @param FrameNr The frame of the sequence to render this model in.
    /// @param LodDist Distance that is taken into account for level of detail considerations.
    /// @param SubModel Model to be drawn as a sub model. If NULL no sub model is drawn.
    virtual void Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel=0 /*NULL*/) const=0;

    /// If this model has a panel for an associated GUI, this method returns its related plane in model space.
    /// As with the Draw() method, you can (and have to) specify the SequenceNr, FrameNr and LodDist,
    /// as the GUI could be on an animated model and thus its plane position and orientation depends on these values.
    /// @param SequenceNr   As with Draw().
    /// @param FrameNr      As with Draw().
    /// @param LodDist      As with Draw().
    /// @param GuiOrigin    The origin (Stützvektor) of the GUI plane in model space is returned in this parameter.
    /// @param GuiAxisX     The x-axis (Richtungsvektor) of the GUI plane in model space is returned in this parameter.
    /// @param GuiAxisY     The y-axis (Richtungsvektor) of the GUI plane in model space is returned in this parameter.
    /// @return Whether the call was successful, i.e. whether this model has a GUI panel at all and the GuiOrigin,
    ///     GuiAxisX and GuiAxisY parameters were filled-out properly.
    virtual bool GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const=0;

    /// Prints the model data in human readable form to stdout.
    // TODO: Change this to  std::string GetDescription() const=0;  ?
    virtual void Print() const=0;


    /// Gets the number of animation sequences of this model.
    /// @return Number of sequences of this model.
    virtual int GetNrOfSequences() const=0;

    /// Returns a pointer to six statically allocated floats, describing the per-sequence bounding box of sequence SequenceNr at frame FrameNr.
    /// @param SequenceNr The sequence number to get the bounding box for.
    /// @param FrameNr The frame number to get this models bounding box for.
    /// @return Six statically allocated floats, describing the bounding box of this model.
    virtual const float* GetSequenceBB(int SequenceNr, float FrameNr) const=0;

    // Returns the number of frames of sequence SequenceNr. Useful for non-repeating (e.g. death) sequences.
 // virtual float GetNrOfFrames(int SequenceNr) const=0;

    /// For sequence SequenceNr at frame FrameNr, this advances the frame number by DeltaTime, and returns the new frame number.
    /// Pass 'Loop==true' for *looping* (repeating, wrapped) sequences (e.g. idle, walk, ...),
    /// 'Loop==false' for play-once (non-repeating, clamped) sequences (e.g. dying).
    /// @param SequenceNr The sequence number for which the frame number is advanced.
    /// @param FrameNr The current frame number.
    /// @param DeltaTime The time delta in seconds.
    /// @param Loop Whether the animation should be looped.
    /// @return The new frame number.
    virtual float AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop=true) const=0;

    /// The virtual destructor. Needed because derived classes can be deleted via pointers to ModelT.
    virtual ~ModelT() { }
};

#endif
