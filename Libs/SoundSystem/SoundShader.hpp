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

#ifndef CAFU_SOUNDSYS_SOUND_SHADER_HPP_INCLUDED
#define CAFU_SOUNDSYS_SOUND_SHADER_HPP_INCLUDED

#include <string>


class TextParserT;


/// A SoundShader is a description of a sound with various properties.
/// These properties can be specified by the user through a sound shader script.
/// The sound system uses a sound shader to load soundfiles and play them with sound shader specific properties.
class SoundShaderT
{
    public:

    /// SoundType is used to change properties of all sounds from a specific type in the game code.
    /// e.g. Turn volume of player effects down.
    enum SoundGroupE
    {
        SOUND_MUSIC,  ///< Background music.
        SOUND_EFFECT, ///< Effects like gunshots etc.
        SOUND_PLAYER, ///< Player sounds like footsteps etc.
        SOUND_AMBIENT ///< Ambient sounds like birds.
    };

    /// Determines how a sound file is loaded into memory.
    enum LoadTypeE
    {
        AUTO,      ///< The sound system decides how to load the sound file associated with this shader.
        STATIC,    ///< Load whole sound decrompressed into memory (used for small files).
        STREAM,    ///< Stream sound into memory (decrompressed).
        COMPRESSED ///< Load whole sound compressed into memory (lower memory usage, but higher CPU usage).
    };


    /// Default constructor used to create an "empty" sound shader with default parameters.
    /// This is useful to create a custom sound shader from game code and adjust its parameters manually.
    /// @param SoundShaderName Name of this soundshader (since shaders of this kind are never registered with the
    ///                        global sound shader manager, their names don't need to be unique or set at all).
    SoundShaderT(const std::string& SoundShaderName="");

    /// Constructor to create a sound shader from a scriptfile using the passed TextParser.
    /// This constructor is used by the sound shader manager to load sound shaders from script files.
    /// Throws TextParserT::ParseError on failure.
    /// @param SoundShaderName Name of the sound shader as specified in the material script.
    /// @param TextParser The textparser that is currently parsing a sound shader script (must be at the position of
    ///                   this sound shaders definition).
    /// @param ModDir The directory of the MOD this shader is created in relative to the executables directory.
    SoundShaderT(const std::string& SoundShaderName, TextParserT& TextParser, const std::string& ModDir);


    const std::string Name;           ///< The name of the sound shader.
    std::string       AudioFile;      ///< The sound file this shader is associated with.
    float             InnerVolume;    ///< The volume of this sound (inside its sound cone/at minimal distance). 1.0 meaning 100% volume and 0.0 mute sound.
    float             OuterVolume;    ///< The sounds volume if listener is outside the sound cone.
    float             InnerConeAngle; ///< The inner angle of the cone in which the sound is emited at normal volume.
    float             OuterConeAngle; ///< The outer angle of the cone outside which the sound is emited at outside volume.
    float             MinDistance;    ///< The minimum distance that the sound will cease to continue growing louder at (stays at max. volume).
    float             MaxDistance;    ///< The maximum distance that the sound will cease to attenuate.
    int               NrOfLoops;      ///< The number of times this sound should be looped. -1 for infinite, 1 for one time sounds.
    float             Pause;          ///< Pause in seconds between two loops.
    float             RollOfFactor;   ///< The factor at which the sound is attenuated when listener is outside min distance.
    float             Pitch;          ///< Pitch muliplier for this sound.
    unsigned int      Priority;       ///< Priority for sounds using this shader (higher values mean higher priority).
    SoundGroupE       SoundGroup;     ///< Determines the group this sound belongs to.
    LoadTypeE         LoadType;       ///< Determines the way this sound shaders file is loaded into memory.
};

#endif
