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

#ifndef CAFU_CAMERA_HPP_INCLUDED
#define CAFU_CAMERA_HPP_INCLUDED

#include "Math3D/Angles.hpp"
#include "Math3D/Matrix.hpp"


/// This class implements a camera. Cameras are associated with the 3D views and controlled with the Camera tool.
/// A camera is represented by an orthogonal, right-handed coordinate system,
/// where the x-axis points right, the y-axis points forward (the viewing direction) and the z-axis points up.
class CameraT
{
    public:

    CameraT();

    Vector3fT      GetXAxis() const;    ///< Returns the x-axis (pointing right)   of the camera space.
    Vector3fT      GetYAxis() const;    ///< Returns the y-axis (pointing forward) of the camera space. This is the direction the camera is looking into!
    Vector3fT      GetZAxis() const;    ///< Returns the z-axis (pointing up)      of the camera space.
    const MatrixT& GetMatrix() const;   ///< Returns the matrix that represents the position and orientation of this camera.

    void SetLookAtPos(const Vector3fT& LookAtPos);  ///< This method automatically computes the orientation of the camera so that it looks at the given point.
    void LimitAngles();                             ///< This method wraps the yaw into the [0°, 360°[ intervall and clamps the pitch to -90° and +90°. Call this method after each manipulation of the angles!


    // Regular members that define the essential properties of the camera.
    Vector3fT          Pos;             ///< The cameras position in the world.
    cf::math::AnglesfT Angles;          ///< The angles that describe the cameras orientation. The pitch value is limited/clamped to the interval from -90° to +90°, and roll is not used at all.
    float              ViewDirLength;   ///< This member defines how long the view direction vector (GetYAxis()) is drawn in the 2D views.

    // Additional members that augment the definition of the cameras view pyramid (frustum).
    float              VerticalFOV;     ///< The cameras field-of-view angle, in vertical (up/down) direction.
    float              NearPlaneDist;   ///< The distance of the near clip plane to the tip of the view pyramid.
    float              FarPlaneDist;    ///< The distance of the far  clip plane to the tip of the view pyramid.


    private:

    mutable Vector3fT          m_MatrixPos;     ///< The position with which the m_Matrix was built.
    mutable cf::math::AnglesfT m_MatrixAngles;  ///< The angles   with which the m_Matrix was built.
    mutable MatrixT            m_Matrix;        ///< The matrix that transforms from world to camera space.
};

#endif
