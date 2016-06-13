/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_TRACE_RESULT_HPP_INCLUDED
#define CAFU_CLIPSYS_TRACE_RESULT_HPP_INCLUDED

#include "Math3D/Vector3.hpp"


class MaterialT;


namespace cf
{
    namespace ClipSys
    {
        class ClipModelT;


        /// This class describes the result of tracing an object (a ray, a bounding-box,
        /// or a convex solid) through a collision model, a clip model, or a clip world.
        ///
        ///   - If `StartSolid` is `true`, the trace started in solid. In this case,
        ///     `Fraction` is accordingly set to 0.0.
        ///
        ///   - If `Fraction` is smaller than the value that the trace was started with,
        ///     something was hit along the trace. `Fraction`, `ImpactNormal` and `Material`
        ///     provide details about the point of impact.
        ///
        ///   - If `Fraction` did not change, the entire trace succeeded without hitting
        ///     anything.
        ///
        struct TraceResultT
        {
            /// The constructor.
            TraceResultT(double Fraction_ = 1.0) : Fraction(Fraction_), StartSolid(false), Material(NULL) { }

            double     Fraction;        ///< How much of the trace could be completed before a hit occurred (if any).
            bool       StartSolid;      ///< Did the trace start in a solid part of the collision or clip model?
            Vector3dT  ImpactNormal;    ///< On impact, this is the normal vector of the hit surface.
            MaterialT* Material;        ///< The material at the point of impact. Can be NULL when an edge (i.e. a bevel plane) was hit.
        };


        /// This class describes one result (of possibly several) of tracing an object
        /// (a ray, a bounding-box, or a convex solid) through a clip world.
        struct WorldTraceResultT
        {
            WorldTraceResultT() : Result(), ClipModel(NULL) { }
            WorldTraceResultT(const TraceResultT& TR, ClipModelT* CM) : Result(TR), ClipModel(CM) { }

            TraceResultT Result;        ///< The result of the trace that hit ClipModel (`Result.Fraction < 1.0`).
            ClipModelT*  ClipModel;     ///< The clip model related to the trace result. If `NULL`, the trace result is related to the ClipWorldT::WorldCollMdl.
        };
    }
}

#endif
