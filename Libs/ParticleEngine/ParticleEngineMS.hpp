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

/********************************/
/*** Particle Engine (Header) ***/
/********************************/

// TODO: REMOVE ALL THE "MATSYS" and "MS" SUFFIXES!
#ifndef _CA_PARTICLE_ENGINE_MATSYS_HPP_
#define _CA_PARTICLE_ENGINE_MATSYS_HPP_


namespace MatSys { class RenderMaterialT; }
struct ParticleMST;


/// This is a pointer to a function that moves a given particle over time.
/// Returns 'true' if the particle is still alive, 'false' otherwise (it will then be removed from the set of alive particles).
typedef bool (*ParticleMoveFunctionMST)(ParticleMST* Particle, float Time);


/// This structure describes a single particle.
struct ParticleMST
{
    float                    Origin[3];     ///< Origin of the particle in Cafu world coordinates.
    float                    Velocity[3];   ///< Velocity vector of the particle.
    float                    Age;           ///< Age of the particle, in seconds.
    unsigned char            Color[4];      ///< ?? REMOVE (MatSys respects ambient light color already) ??    The RGBA color with which this particles texture is modulated.
    float                    Radius;        ///< Billboard radius in world coords.
    unsigned char            Rotation;      ///< Rotation angle of the billboard in the "screen plane". A value of 256 corresponds to 360 degrees.
 // bool                     AsBillboard;   ///< Is this particle drawn as a billboard, or in world coordinates? (In the latter case we also need more data, e.g. a normal.)
    float                    StretchY;      ///< Length of the Y-axis relative to the X-axis, e.g. for sparks, flashes etc.
    MatSys::RenderMaterialT* RenderMat;     ///< The particles RenderMaterial ID.
    ParticleMoveFunctionMST  MoveFunction;  ///< Pointer to the function that moves this particle through time.
    float                    AuxData[8];    ///< Auxiliary particle data.
};


/// These are the functions of the particle engine.
/// To make it as fast as possible, dynamic memory allocation only takes place during initialization (when registering the textures).
/// Later, when huge amounts of particles must be handled, everything takes place in "static" memory.
namespace ParticleEngineMS
{
    /// Registers a new particle with the particle engine.
    void RegisterNewParticle(const ParticleMST& Particle);

    /// Calls the move function for each particle and then removes dead particles.
    void MoveParticles(float Time);

    /// Draws all currently known (alive) particles.
    void DrawParticles();
}

#endif
