/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

// TODO: REMOVE ALL THE "MATSYS" and "MS" SUFFIXES!
#ifndef CAFU_PARTICLE_ENGINE_MATSYS_HPP_INCLUDED
#define CAFU_PARTICLE_ENGINE_MATSYS_HPP_INCLUDED

#include "Templates/Array.hpp"
#include <string>


namespace MatSys { class RenderMaterialT; }
struct ParticleMST;


/// This is a pointer to a function that moves a given particle over time.
/// Returns 'true' if the particle is still alive, 'false' otherwise (it will then be removed from the set of alive particles).
typedef bool (*ParticleMoveFunctionMST)(ParticleMST* Particle, float Time);


/// This structure describes a single particle.
struct ParticleMST
{
    float                             Origin[3];    ///< Origin of the particle in Cafu world coordinates.
    float                             Velocity[3];  ///< Velocity vector of the particle.
    float                             Age;          ///< Age of the particle, in seconds.
    unsigned char                     Color[4];     ///< ?? REMOVE (MatSys respects ambient light color already) ??    The RGBA color with which this particles texture is modulated.
    float                             Radius;       ///< Billboard radius in world coords.
    unsigned char                     Rotation;     ///< Rotation angle of the billboard in the "screen plane". A value of 256 corresponds to 360 degrees.
 // bool                              AsBillboard;  ///< Is this particle drawn as a billboard, or in world coordinates? (In the latter case we also need more data, e.g. a normal.)
    float                             StretchY;     ///< Length of the Y-axis relative to the X-axis, e.g. for sparks, flashes etc.
    ArrayT<MatSys::RenderMaterialT*>* AllRMs;       ///< The list of all render materials possibly used with this particle.
    MatSys::RenderMaterialT*          RenderMat;    ///< The particles RenderMaterial ID.
    ParticleMoveFunctionMST           MoveFunction; ///< Pointer to the function that moves this particle through time.
    float                             AuxData[8];   ///< Auxiliary particle data.
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


/// This class represents a set of (render-)materials.
/// The materials in one such set are typically used for implementing an animated particle, e.g. an explosion.
class ParticleMaterialSetT
{
    public:

    ParticleMaterialSetT(const char* SetName, const char* MatNamePattern);

    ~ParticleMaterialSetT();

    ArrayT<MatSys::RenderMaterialT*>& GetRenderMats() { return m_RenderMats; }


    private:

    const std::string                m_SetName;
    ArrayT<MatSys::RenderMaterialT*> m_RenderMats;
};

#endif
