// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpaceMouseImpl_h
#define pqSpaceMouseImpl_h

// This header is provided in the 3DxWare SDK, in samples/navlib_viewer/src/SpaceMouse
// If you are not using paraview superbuild, it is simplest to copy the entire
// SpaceMouse directory to the ${3DxWareSDK_INCLUDE_DIR} directory
#include "SpaceMouse/CNavigation3D.hpp"
#include "vtkCamera.h"
#include "vtkWeakPointer.h"

#include <QObject>
class pqView;

/// Communicate with the TDxNavlib SDK provided by 3DConnexion to move the camera with a SpaceMouse
class pqSpaceMouseImpl
  : public QObject
  , public TDx::SpaceMouse::Navigation3D::CNavigation3D
{
  Q_OBJECT
  typedef TDx::SpaceMouse::Navigation3D::CNavigation3D nav3d;
  typedef TDx::SpaceMouse::Navigation3D::CNavigation3D Superclass;

public:
  pqSpaceMouseImpl();
  ~pqSpaceMouseImpl() override;

public Q_SLOTS:
  /// which view are we controlling? The active one.
  void setActiveView(pqView* view);

public:
  void render();

  // navlib initialization
  bool Enable3DNavigation();
  void Disable3DNavigation();

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // accessors and mutators used by the navlib
  // These are the functions supplied to the navlib in the accessors structure so that the navlib
  // can query and update our (the clients) values.
  long GetCoordinateSystem(navlib::matrix_t& affine) const override;
  long GetViewConstructionPlane(navlib::plane_t& plane) const override;
  long GetCameraMatrix(navlib::matrix_t& affine) const override;
  long GetViewExtents(navlib::box_t& affine) const override;
  long GetViewFrustum(navlib::frustum_t& frustum) const override;
  long GetViewFOV(double& fov) const override;
  long GetIsViewRotatable(navlib::bool_t& rotatble) const override;
  long IsUserPivot(navlib::bool_t& userPivot) const override;
  long GetPivotVisible(navlib::bool_t& visible) const override;
  long GetPivotPosition(navlib::point_t& position) const override;
  long GetPointerPosition(navlib::point_t& position) const override;
  long GetIsViewPerspective(navlib::bool_t& persp) const override;
  long GetModelExtents(navlib::box_t& extents) const override;
  long GetIsSelectionEmpty(navlib::bool_t& empty) const override;
  long GetSelectionExtents(navlib::box_t& extents) const override;
  long GetSelectionTransform(navlib::matrix_t&) const override;
  long GetHitLookAt(navlib::point_t& position) const override;

  long SetSettingsChanged(long change) override;

  long SetMotionFlag(bool value) override;
  long SetTransaction(long value) override;
  long SetCameraMatrix(const navlib::matrix_t& affine) override;
  long SetSelectionTransform(const navlib::matrix_t& affine) override;
  long SetViewExtents(const navlib::box_t& extents) override;
  long SetViewFOV(double fov) override;
  long SetViewFrustum(const navlib::frustum_t& frustum) override;
  long SetPivotPosition(const navlib::point_t& position) override;
  long SetPivotVisible(bool visible) override;

  long SetHitLookFrom(const navlib::point_t& position) override;
  long SetHitDirection(const navlib::vector_t& direction) override;
  long SetHitAperture(double diameter) override;
  long SetHitSelectionOnly(bool value) override;
  long SetActiveCommand(std::string commandId) override;

protected:
  vtkWeakPointer<vtkCamera> Camera;
  bool Interacting = false;
};

#endif
