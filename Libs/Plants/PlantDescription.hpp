/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TREE_DESCRIPTION_HPP_INCLUDED
#define CAFU_TREE_DESCRIPTION_HPP_INCLUDED

#include "Templates/Array.hpp"
#include <string>


class TextParserT;
class MaterialT;
namespace MatSys { class RenderMaterialT; }


/// Contains a plant description that has all the information needed to create a plant.
/// Using a random seed for plant creation this description is used as a template that
/// is randomly modified to create multiple different plants from the same plant description.
/// The plant description is loaded from a file and always associated with this file.
struct PlantDescriptionT
{
    /// This struct describes a "profile".
    /// Examples include how the radius of a branch changes from its root to its tip,
    /// or how child branches are angled from the own root to the tip.
    struct ProfileT
    {
        /// Contains different types of profiles that together with a parameter build up
        /// a ProfileT object.
        enum ProfileTypeT { Linear, LinearDroop, InvLinear, InvLinearRamp, ConstantAvg, Round };

        ProfileTypeT ProfileType; ///< The type of this profile.
        float        Parameter;   ///< The floating point parameter describing the profile.


        /// Constructor: Creates a new profile.
        /// @param ProfileType_ The type of this profile.
        /// @param Parameter_ The parameter used with this profile.
        ProfileT(ProfileTypeT ProfileType_=Linear, float Parameter_=0.0);

        /// According to this profile, this function computes the "transition value" between two values.
        /// @param p Relative position at which the transition value is computed (must be 0.0<=p<=1.0).
        /// @param Value0 "left" Value (p=0.0).
        /// @param Value1 "right" Value (p=1.0).
        /// @return Transition value at position p.
        float Compute(float p, float Value0, float Value1) const;
    };


    /// This struct contains a description for an individual branch level.
    struct BranchLevelDescriptionT
    {
        float         Length0;                 ///< Our length (in % of global size), when we are attached at the root (  0%) of our parent branch
        float         Length1;                 ///< Our length (in % of global size), when we are attached at the tip  (100%) of our parent branch
        float         LengthV;                 ///< Variance of our length (in % of global size)
        ProfileT      LengthProfile;           ///< Transition function from Length0 to Length1 across our "point of attachment" at our parent

        float         StartAngle0;             ///< Our start angle at the parent, when we are attached at the root (  0%) of our parent branch
        float         StartAngle1;             ///< Our start angle at the parent, when we are attached at the tip  (100%) of our parent branch
        float         StartAngleV;             ///< Variance of our start angle at the parent
        ProfileT      StartAngleProfile;       ///< Transition function from StartAngle0 to StartAngle1 across our "point of attachment" at our parent

        float         Radius0;                 ///< Our radius (at our root, in % of global size), when we are attached at the root ( 0%) of our parent branch
        float         Radius1;                 ///< Our radius (at our root, in % of global size), when we are attached at the tip (100%) of our parent branch
        float         RadiusV;                 ///< Variance of our radius (at our root, in % of global size)
        ProfileT      RadiusProfile;           ///< Transition function from Radius0 to Radius1 across our "point of attachment" at our parent

        unsigned long NrOfSegments;            ///< Number of segments of this parent
        unsigned long CrossSectionsResolution; ///< Number of vertices of the cross-sections of the branches of this level
        ProfileT      AngleProfile_Segments;   ///< Angle profile across segments (together with Gravity, this rules the shape along the length)
        ProfileT      RadiusProfile_Segments;  ///< Radius profile across segments ("thickness" along the length)

        float         FirstChild;              ///< Min. distance (in %) of our children (sub-branches or leaves) from our root
        float         LastChild;               ///< Max. distance (in %) of our children (sub-branches or leaves) from our root
        float         NrOfChildren;            ///< Number of our children per unit length (???)
    };


    // ************
    //    Global
    // ************

    const std::string        FileName;         ///< Name of the file this plant description is stored in.
    unsigned long            RandomSeed;       ///< Seed for the random number generator.
    float                    Size;             ///< Height in meters (or any other arbitrary unit).
    float                    SizeV;            ///< Variance of the height in meters (or any other arbitrary unit).
    MaterialT*               BarkMat;          ///< Material used for the bark of this tree.
    MatSys::RenderMaterialT* BarkRenderMat;    ///< Render material used for the bark of this tree.


    // ************
    //   Branches
    // ************

    ArrayT<BranchLevelDescriptionT> BranchLevelDescriptions;    ///< The list of branch level descriptions.


    // ************
    //    Leaves
    // ************

    // Dies koennte auch in 'BranchLevelDescriptionT' untergebracht sein; dann koennte *jeder* Ast Leaves haben!
    float                            Distance0;          ///< Radial distance (in m) of leaf attachment point from parent branch, when attached at parents root (  0%).
    float                            Distance1;          ///< Radial distance (in m) of leaf attachment point from parent branch, when attached at parents tip  (100%).
    float                            DistanceV;          ///< Variance of the radial distance (in meters).
    ProfileT                         DistanceProfile;    ///< Transition function from Distance0 to Distance1 across our "point of attachment" at the parent.

    ArrayT<MaterialT*>               LeafMats;           ///< Materials of all leaves.
    ArrayT<MatSys::RenderMaterialT*> LeafRenderMats;     ///< Render materials of all leaves.
    float                            LeafSize;           ///< The size of the leaf polygons relative to (in % of) 'Size'.


    /// Constructor. Creates an "empty" tree description.
    /// Those who really want to can use this constructor to fill-in the PlantDescriptionT fields programmatically
    /// (instead of using any of the other constructors, which is preferred).
    /// @param FileName_ The dummy file name of this plant description.
    PlantDescriptionT(const std::string& FileName_) : FileName(FileName_), BarkRenderMat(NULL) {}

    /// Destructor.
    ~PlantDescriptionT();

    /// Constructor: Creates a tree description from a text parser.
    /// @param TP The text parser containing the data to create this tree description from.
    /// @param FileName_ The name of the file this plant description is created from.
    PlantDescriptionT(TextParserT& TP, const std::string& FileName_);
};

#endif
