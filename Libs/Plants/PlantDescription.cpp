/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "PlantDescription.hpp"

#include "TextParser/TextParser.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"

#include <cstdio>
#include <math.h>


PlantDescriptionT::ProfileT::ProfileT(ProfileTypeT ProfileType_, float Parameter_) : ProfileType(ProfileType_), Parameter(Parameter_)
{
}


float PlantDescriptionT::ProfileT::Compute(float p, float Value0, float Value1) const
{
    switch (ProfileType)
    {
        case Linear       : return      (1.0f- p       )   *Value0+      p             *Value1;         // (0, 1.0) - (1, 0)
        case LinearDroop  : return 0.5f*((1.0f- p       )  *Value0+      p             *Value1);        // (0, 0.5) - (1, 0)  // BUGGY!?  But still OK for EnglishOak...
        case InvLinear    : return            p            *Value0+(1.0f-p          )  *Value1;         // (0, 0.0) - (1, 1)
        case InvLinearRamp: return (0.5f+0.5f*p)           *Value0+(0.5f+0.5f*(1.0f-p))*Value1;         // (0, 0.5) - (1, 1)
        case ConstantAvg  : return (1.0f-Parameter)        *Value0+      Parameter     *Value1;         // (0, Parameter) - (1, Parameter)
        case Round        : return (1.0f-sin(p*3.1415926f))*Value0+sin(p*3.1415926f)   *Value1;
        // case Round2       : return ((1.0-sin(p*3.1415926))*Value0+sin(p*3.1415926)*Value1)*((1.0-sin(p*3.1415926))*Value0+sin(p*3.1415926)*Value1);
        default           : return Value0;
    }
}


PlantDescriptionT::ProfileT GetProfileFromString(std::string ProfileString, float Parameter)
{
    if (ProfileString=="Linear"       ) return PlantDescriptionT::ProfileT(PlantDescriptionT::ProfileT::Linear       , Parameter);
    if (ProfileString=="LinearDroop"  ) return PlantDescriptionT::ProfileT(PlantDescriptionT::ProfileT::LinearDroop  , Parameter);
    if (ProfileString=="InvLinear"    ) return PlantDescriptionT::ProfileT(PlantDescriptionT::ProfileT::InvLinear    , Parameter);
    if (ProfileString=="InvLinearRamp") return PlantDescriptionT::ProfileT(PlantDescriptionT::ProfileT::InvLinearRamp, Parameter);
    if (ProfileString=="ConstantAvg"  ) return PlantDescriptionT::ProfileT(PlantDescriptionT::ProfileT::ConstantAvg  , Parameter);
    if (ProfileString=="Round"        ) return PlantDescriptionT::ProfileT(PlantDescriptionT::ProfileT::Round        , Parameter);

    printf("Unkown profile string '%s' using 'linear' by default\n", ProfileString.c_str());

    return PlantDescriptionT::ProfileT(PlantDescriptionT::ProfileT::Linear, Parameter);
}


PlantDescriptionT::PlantDescriptionT(TextParserT& TP, const std::string& FileName_)
    : FileName(FileName_),
      RandomSeed(0),
      Size(0.0f),
      SizeV(0.0f),
      BarkMat(NULL),
      BarkRenderMat(NULL),
      Distance0(0.0f),
      Distance1(0.0f),
      DistanceV(0.0f),
      LeafSize(0.0f)
{
    unsigned long DescribeBranchLevel=0;

    try
    {
        if (TP.IsAtEOF()) throw TextParserT::ParseError();

        while (!TP.IsAtEOF())
        {
            std::string Token=TP.GetNextToken();

            // GLOBAL TREE PARAMS
            if (Token=="RandomSeed")
            {
                RandomSeed=(unsigned long)TP.GetNextTokenAsInt();
            }
            else if (Token=="Size")
            {
                Size=TP.GetNextTokenAsFloat();
            }
            else if (Token=="SizeV")
            {
                SizeV=TP.GetNextTokenAsFloat();
            }
            else if (Token=="BarkTexName")
            {
                const std::string BarkMatName=TP.GetNextToken();

                // Get Render material from string.
                if (MaterialManager!=NULL)
                {
                    BarkMat=MaterialManager->GetMaterial(BarkMatName);

                    if (MatSys::Renderer!=NULL)
                        BarkRenderMat=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(BarkMatName));
                }
            }

            // LEAF PARAMS
            else if (Token=="Distance0")
            {
                Distance0=TP.GetNextTokenAsFloat();
            }
            else if (Token=="Distance1")
            {
                Distance1=TP.GetNextTokenAsFloat();
            }
            else if (Token=="DistanceV")
            {
                DistanceV=TP.GetNextTokenAsFloat();
            }
            else if (Token=="DistanceProfile")
            {
                std::string ProfType =TP.GetNextToken();
                float       ProfParam=TP.GetNextTokenAsFloat();

                DistanceProfile=GetProfileFromString(ProfType, ProfParam);
            }
            else if (Token=="LeafTexName")
            {
                const std::string LeafMatName=TP.GetNextToken();

                // Get render material from string.
                if (MaterialManager!=NULL)
                {
                    LeafMats.PushBack(MaterialManager->GetMaterial(LeafMatName));

                    if (MatSys::Renderer!=NULL)
                        LeafRenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(LeafMatName)));
                    else
                        LeafRenderMats.PushBack(NULL); // LeafMats and LeafRenderMats need to have the same size.
                }
            }
            else if (Token=="LeafSize")
            {
                LeafSize=TP.GetNextTokenAsFloat();
            }

            // BRANCH LEVEL DESCRIPTION
            else if (Token=="DescribeBranchLevel")
            {
                DescribeBranchLevel=(unsigned long)TP.GetNextTokenAsInt();

                // "DescribeBranchLevel n" must be called consecutively (0, 1, 2, 3, ...)!
                if (DescribeBranchLevel==BranchLevelDescriptions.Size())
                    BranchLevelDescriptions.PushBackEmpty();
            }
            else if (Token=="Length0")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].Length0=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="Length1")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].Length1=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="LengthV")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].LengthV=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="LengthProfile")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    std::string ProfType =TP.GetNextToken();
                    float       ProfParam=TP.GetNextTokenAsFloat();

                    BranchLevelDescriptions[DescribeBranchLevel].LengthProfile=GetProfileFromString(ProfType, ProfParam);
                }
            }
            else if (Token=="StartAngle0")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].StartAngle0=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="StartAngle1")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].StartAngle1=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="StartAngleV")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].StartAngleV=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="StartAngleProfile")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    std::string ProfType =TP.GetNextToken();
                    float       ProfParam=TP.GetNextTokenAsFloat();

                    BranchLevelDescriptions[DescribeBranchLevel].StartAngleProfile=GetProfileFromString(ProfType, ProfParam);
                }
            }
            else if (Token=="Radius0")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].Radius0=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="Radius1")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].Radius1=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="RadiusV")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].RadiusV=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="RadiusProfile")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    std::string ProfType =TP.GetNextToken();
                    float       ProfParam=TP.GetNextTokenAsFloat();

                    BranchLevelDescriptions[DescribeBranchLevel].RadiusProfile=GetProfileFromString(ProfType, ProfParam);
                }
            }
            else if (Token=="NrOfSegments")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].NrOfSegments=(unsigned long)TP.GetNextTokenAsInt();
                }
            }
            else if (Token=="CrossSectionsResolution")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].CrossSectionsResolution=(unsigned long)TP.GetNextTokenAsInt();
                }
            }
            else if (Token=="AngleProfile_Segments")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    std::string ProfType =TP.GetNextToken();
                    float       ProfParam=TP.GetNextTokenAsFloat();

                    BranchLevelDescriptions[DescribeBranchLevel].AngleProfile_Segments=GetProfileFromString(ProfType, ProfParam);
                }
            }
            else if (Token=="RadiusProfile_Segments")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    std::string ProfType =TP.GetNextToken();
                    float       ProfParam=TP.GetNextTokenAsFloat();

                    BranchLevelDescriptions[DescribeBranchLevel].RadiusProfile_Segments=GetProfileFromString(ProfType, ProfParam);
                }
            }
            else if (Token=="FirstChild")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].FirstChild=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="LastChild")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].LastChild=TP.GetNextTokenAsFloat();
                }
            }
            else if (Token=="NrOfChildren")
            {
                if (DescribeBranchLevel<BranchLevelDescriptions.Size())
                {
                    BranchLevelDescriptions[DescribeBranchLevel].NrOfChildren=TP.GetNextTokenAsFloat();
                }
            }
            else
            {
                printf("\nPlantDescriptionT - Warning: Unknown token '%s' in tree description string:\n", Token.c_str());
                printf("I'll ignore it and try to proceed, but you'd better fix this.\n");
            }
        }
    }
    catch (const TextParserT::ParseError& /*PE*/)
    {
        printf("Error parsing plant description from file '%s'\n", FileName.c_str());
    }

    // Check if the plant description has been initialized properly. Otherwise create default parameters
    // for essential members.
    if (Size==0.0f || BarkMat==NULL || BranchLevelDescriptions.Size()==0)
    {
        printf("Plant description has not been properly parsed from file '%s'\nCreating default parameters...\n", FileName.c_str());

        // Try to fill out values that haven't been properly set, with default values.
        // Note: If file parsing totally failed a plant with just a root branch and no leaves is created.
        if (Size==0.0f) Size=5.0f;

        if (BarkMat==NULL)
            BarkMat=MaterialManager->GetMaterial("Plants/Trees/AhornBark"); //XXX Find a better default material!

        if (BarkRenderMat==NULL)
            if (MatSys::Renderer!=NULL)
                BarkRenderMat=MatSys::Renderer->RegisterMaterial(BarkMat);

        if (BranchLevelDescriptions.Size()==0)
        {
             BranchLevelDescriptionT BLD;

             BLD.Length0=1.0f;
             BLD.Length1=0.0f;
             BLD.LengthV=0.0f;

             BLD.StartAngle0=90.0f;
             BLD.StartAngle1=0.0f;
             BLD.StartAngleV=0.0f;

             BLD.Radius0=0.2f;
             BLD.Radius1=0.2f;
             BLD.RadiusV=0.0f;

             BLD.NrOfSegments=1;
             BLD.CrossSectionsResolution=8;

             BLD.AngleProfile_Segments=ProfileT(ProfileT::ConstantAvg, 0.5);

             BLD.FirstChild=0.0f;
             BLD.LastChild=0.0f;
             BLD.NrOfChildren=0.0f;

             BranchLevelDescriptions.PushBack(BLD);
        }
    }
}


PlantDescriptionT::~PlantDescriptionT()
{
    if (MatSys::Renderer==NULL) return; // If there is no renderer we can't release any render materials (and we shouldn't have any either).

    // Release render materials.
    MatSys::Renderer->FreeMaterial(BarkRenderMat);

    for (unsigned long i=0; i<LeafRenderMats.Size() ; i++)
        MatSys::Renderer->FreeMaterial(LeafRenderMats[i]);
}
