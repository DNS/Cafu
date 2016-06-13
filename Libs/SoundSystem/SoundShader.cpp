/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
