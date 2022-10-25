/*=========================================================================

   Program: ParaView
   Module:  pqSpaceMouseImpl.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension, nameless union
#endif

#include "pqSpaceMouseImpl.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqRenderView.h"
#include "vtkInitializationHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

#include "vtkLogger.h"
#include "vtkMatrix4x4.h"
#include "vtkObject.h"
#include "vtkTransform.h"

#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#define APPLICATION_HAS_ANIMATION_LOOP 0

pqSpaceMouseImpl::pqSpaceMouseImpl()
  : CNavigation3D(false, true)
{
}

pqSpaceMouseImpl::~pqSpaceMouseImpl() = default;

void pqSpaceMouseImpl::setActiveView(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  vtkSMRenderViewProxy* renPxy = rview ? rview->getRenderViewProxy() : nullptr;

  this->Camera = nullptr;
  if (renPxy)
  {
    this->Enable3DNavigation();
    this->Camera = renPxy->GetActiveCamera();
  }
  else
  {
    this->Disable3DNavigation();
  }
}

void pqSpaceMouseImpl::render()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    view->render();
  }
}

// Following methods were copied from
// 3DxWare_SDK_v4-0-2_r17624/samples/navlib_viewer/src/mainfrm.cpp and follow the recommendations of
// 3DxWare_SDK_v4-0-2_r17624/doc/quick_guide.pdf

/////////////////////////////////////////////////////////////////////////////
// navlib

/// Shutdown the connection to the navlib
void pqSpaceMouseImpl::Disable3DNavigation()
{
  nav3d::Enable = false;
}

/// Open the connection to the navlib and expose the property interface
/// functions
bool pqSpaceMouseImpl::Enable3DNavigation()
{
  try
  {
    // Set the hint/title for the '3Dconnexion Properties' Utility.
    std::string title = vtkInitializationHelper::GetApplicationName();
    nav3d::Profile = title.empty() ? "ParaView" : title;

    // Enable input from / output to the Navigation3D controller.
    nav3d::Enable = true;

#if APPLICATION_HAS_ANIMATION_LOOP
    // Use the application render loop as the timing source for the frames
    nav3d::FrameTiming = TimingSource::Application;
#else
    // Use the SpaceMouse as the timing source for the frames.
    nav3d::FrameTiming = TimingSource::SpaceMouse;
#endif
  }
  catch (const std::exception&)
  {
    return false;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////
// The get property assessors
long pqSpaceMouseImpl::GetModelExtents(navlib::box_t& bbox) const
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    // by moving some default bounds to the center-of-rotation, navlib uses our CoR.
    std::vector<double> cor =
      vtkSMPropertyHelper(view->getProxy(), "CenterOfRotation").GetDoubleArray();
    bbox.min = { { cor[0] - 1., cor[1] - 1., cor[2] - 1. } };
    bbox.max = { { cor[0] + 1., cor[1] + 1., cor[2] + 1. } };
    return 0;
  }
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetIsSelectionEmpty(navlib::bool_t& /*empty*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetSelectionExtents(navlib::box_t& /*bbox*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetViewConstructionPlane(navlib::plane_t& /*plane*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetViewExtents(navlib::box_t& /*extents*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetViewFOV(double& fov) const
{
  if (this->Camera)
  {
    fov = this->Camera->GetViewAngle() * M_PI / 180.0;
    return 0;
  }
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetViewFrustum(navlib::frustum_t& /*frustum*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Get whether the view can be rotated
/// <param name="rotatable">true if the view can be rotated</param>
/// <returns>0 on success otherwise an error</returns>
long pqSpaceMouseImpl::GetIsViewRotatable(navlib::bool_t& rotatable) const
{
  if (this->Camera)
  {
    rotatable = true;
    return 0;
  }

  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::IsUserPivot(navlib::bool_t& userPivot) const
{
  // pivot is always the center-of-rotation supplied by PV
  userPivot = true;
  return 0;
}

/// Get the affine of the coordinate system
/// <param name="affine">the world to navlib transformation</param>
/// <returns>0 on success otherwise an error</returns>
long pqSpaceMouseImpl::GetCoordinateSystem(navlib::matrix_t& affine) const
{
  // Y-Up rhs
  navlib::matrix_t cs = { 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1. };

  affine = cs;

  return 0;
}

/// Get the affine of the view
/// <param name="affine">the camera to world transformation</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::GetCameraMatrix(navlib::matrix_t& affine) const
{
  if (this->Camera)
  {
    double* m = this->Camera->GetModelViewTransformMatrix()->GetData();
    // vtkWarningWithObjectMacro(nullptr, << "vtkCM [" << m[0] << ", " << m[1] << ", " << m[2] << ",
    // " << m[3] << "] [" << m[4] << ", " << m[5] << ", " << m[6] << ", " << m[7] << "] [" << m[8]
    // << ", " << m[9] << ", " << m[10] << ", " << m[11] << "] [" << m[12] << ", " << m[13] << ", "
    // << m[14] << ", " << m[15] << "] " ); VTK supplies the world-to-camera tranform, so we must
    // invert.
    vtkMatrix4x4::Invert(m, affine.m);
    return 0;
  }
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

///////////////////////////////////////////////////////////////////////////
// Handle navlib hit testing requests

/// Get the position of the point hit
/// <param name="position">the hit point</param>
/// <returns>0 when something is hit, otherwise navlib::navlib_errc::no_data_available</returns>
long pqSpaceMouseImpl::GetHitLookAt(navlib::point_t& /*position*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Get the position of the rotation pivot in world coordinates
/// <param name="position">the position of the pivot</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::GetPivotPosition(navlib::point_t& position) const
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    std::vector<double> cor =
      vtkSMPropertyHelper(view->getProxy(), "CenterOfRotation").GetDoubleArray();
    position.x = cor[0];
    position.y = cor[1];
    position.z = cor[2];
    return 0;
  }
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetPivotVisible(navlib::bool_t& /*visible*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Get the position of the mouse cursor on the projection plane in world
/// coordinates
/// <param name="position">the position of the mouse cursor</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::GetPointerPosition(navlib::point_t& /*position*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Get the whether the view is a perspective projection
/// <param name="persp">true when the projection is perspective</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::GetIsViewPerspective(navlib::bool_t& persp) const
{
  if (this->Camera)
  {
    persp = !this->Camera->GetParallelProjection();
    return 0;
  }
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// set property handlers (mutators)
//

/// Sets the moving property value.
/// <param name="value">true when moving.</param>
/// <returns>0 on success, otherwise an error.</returns>
/// <description>The navlib sets this to true at the beginning of navigation.</description>
long pqSpaceMouseImpl::SetMotionFlag(bool /*value*/)
{
  return 0;
}

/// Sets the transaction property value
/// <param name="value">!0 at the beginning of a frame;0 at the end of a
/// frame</param> <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::SetTransaction(long /*value*/)
{
  return 0;
}

/// Sets the affine of the view
/// <param name="affine">The camera to world transformation</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::SetCameraMatrix(const navlib::matrix_t& affine)
{
  if (this->Camera)
  {
    // VTK wants the world-to-camera transform, so invert to retrieve the
    // position in world coords.
    double m[16];
    double inv[16];
    vtkMatrix4x4::Invert(affine.m, m);
    vtkMatrix4x4::DeepCopy(inv, affine.m);

    double pos[3] = { inv[3], inv[7], inv[11] };
    double lk[3] = { -m[8], -m[9], -m[10] };
    double vu[3] = { m[4], m[5], m[6] };
    // vtkWarningWithObjectMacro(nullptr, << "SM ps " << pos[0] << " " << pos[1] << " " << pos[2] );
    // vtkWarningWithObjectMacro(nullptr, << "SM lk " << lk[0] << " " << lk[1] << " " << lk[2] );
    // vtkWarningWithObjectMacro(nullptr, << "SM vu " << vu[0] << " " << vu[1] << " " << vu[2] );
    double dist = this->Camera->GetDistance();
    // Set the new values on the camera.
    double fp[3] = { pos[0] + dist * lk[0], pos[1] + dist * lk[1], pos[2] + dist * lk[2] };
    this->Camera->SetPosition(pos);
    this->Camera->SetFocalPoint(fp);
    this->Camera->SetViewUp(vu);
    this->render();
    return 0;
  }

  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::GetSelectionTransform(navlib::matrix_t& /*affine*/) const
{
  return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
}

long pqSpaceMouseImpl::SetSelectionTransform(const navlib::matrix_t& /*affine*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Sets the extents of the view
/// <param name="extents">The view extents</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::SetViewExtents(const navlib::box_t& /*extents*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Sets the visibility of the pivot
/// <param name="show">true to display the pivot</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::SetPivotVisible(bool /*show*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Sets the position of the pivot
/// <param name="position">The position of the pivot in world
/// coordinates</param> <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::SetPivotPosition(const navlib::point_t& /*position*/)
{
  return 0;
}

/// Sets the vertical field of view
/// <param name="fov">The fov in radians</param>
/// <returns>0 on success, otherwise an error.</returns>
long pqSpaceMouseImpl::SetViewFOV(double /*fov*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long pqSpaceMouseImpl::SetViewFrustum(const navlib::frustum_t& /*frustum*/)
{
  return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
}

///////////////////////////////////////////////////////////////////////////
// Handle navlib hit testing parameters

/// Sets the diameter of the aperture in the projection plane to look through.
/// <param name = "diameter">The diameter of the hole/ray in world units.</param>
long pqSpaceMouseImpl::SetHitAperture(double /*diameter*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Sets the direction to look
/// <param name = "direction">Unit vector in world coordinates</param>
long pqSpaceMouseImpl::SetHitDirection(const navlib::vector_t& /*direction*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Sets the position to look from
/// <param name = "position">Position in world coordinates</param>
long pqSpaceMouseImpl::SetHitLookFrom(const navlib::point_t& /*position*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

/// Sets the selection only hit filter
/// <param name = "value">If true then filter non-selected objects</param>
long pqSpaceMouseImpl::SetHitSelectionOnly(bool /*value*/)
{
  return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

//////////////////////////////////////////////////////////////////////////////////////

/// Handle when a command is activated by a mouse button press
long pqSpaceMouseImpl::SetActiveCommand(std::string /*commandId*/)
{
  return 0;
}

/// Handler for settings_changed event
long pqSpaceMouseImpl::SetSettingsChanged(long /*change*/)
{
  return 0;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
