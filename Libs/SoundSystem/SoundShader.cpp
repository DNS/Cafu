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

#include "SoundShader.hpp"

#include "TextParser/TextParser.hpp"


SoundShaderT::SoundShaderT(const std::string& SoundShaderName)
    : Name(SoundShaderName),
      AudioFile(""),
      InnerVolume(0.5f),
      OuterVolume(0.0f),
      InnerConeAngle(360.0f),
      OuterConeAngle(360.0f),
      MinDistance(0.0f),
      MaxDistance(1000000000.0f),
      NrOfLoops(1),
      Pause(0.0f),
      RollOfFactor(1.0f),
      Pitch(1.0f),
      Priority(0),
      SoundGroup(SOUND_EFFECT),
      LoadType(AUTO)
{
}


SoundShaderT::SoundShaderT(const std::string& SoundShaderName, TextParserT& TextParser, const std::string& ModDir)
    : Name(SoundShaderName),
      AudioFile(""),
      InnerVolume(0.5f),
      OuterVolume(0.0f),
      InnerConeAngle(360.0f),
      OuterConeAngle(360.0f),
      MinDistance(0.0f),
      MaxDistance(1000000000.0f),
      NrOfLoops(1),
      Pause(0.0f),
      RollOfFactor(1.0f),
      Pitch(1.0f),
      Priority(0),
      SoundGroup(SOUND_EFFECT),
      LoadType(AUTO)
{
    if (TextParser.GetNextToken()!="{") throw TextParserT::ParseError();

    while (true)
    {
        std::string Token=TextParser.GetNextToken();

        if (Token=="}")
        {
            // End of sound shader definition.
            break;
        }
        else if (Token=="AudioFile")
        {
            AudioFile=ModDir+TextParser.GetNextToken();
        }
        else if (Token=="MinDistance")
        {
            MinDistance=(float)atof(TextParser.GetNextToken().c_str());
        }
        else if (Token=="LoadType")
        {
            std::string Value=TextParser.GetNextToken();

            if (Value=="STATIC")
                LoadType=STATIC;
            else if (Value=="STREAM")
                LoadType=STREAM;
            else if (Value=="COMPRESSED")
                LoadType=COMPRESSED;
            else
                LoadType=AUTO;
        }
        else if (Token=="Priority")
        {
            Priority=(unsigned int)atoi(TextParser.GetNextToken().c_str());
        }
        else if (Token=="InnerVolume")
        {
            InnerVolume=(float)atof(TextParser.GetNextToken().c_str());
        }
        else if (Token=="InnerConeAngle")
        {
            InnerConeAngle=(float)atof(TextParser.GetNextToken().c_str());
        }
        else if (Token=="OuterConeAngle")
        {
            OuterConeAngle=(float)atof(TextParser.GetNextToken().c_str());
        }
        else if (Token=="OuterVolume")
        {
            OuterVolume=(float)atof(TextParser.GetNextToken().c_str());
        }
        else throw TextParserT::ParseError();   // Unknown token!
    }
}
