// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCaveSynchronizedRenderers
 * @brief   vtkSynchronizedRenderers subclass that
 * handles adjusting of camera for cave configurations.
 *
 * vtkCaveSynchronizedRenderers is vtkSynchronizedRenderers used for in CAVE
 * configuration. It is used on the render-server side. It ensures that the
 * camera is transformed based on the orientations specified in  the pvx
 * configuration file.
 * This code was previously in class vtkCaveRenderManager.
 *
 * In case of a single display, if the cave bounds are not set, we compute these values
 * from vtkDisplayConfiguration::Geometry and the client camera view angle.
 */

#ifndef vtkCaveSynchronizedRenderers_h
#define vtkCaveSynchronizedRenderers_h

#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_6_1_0
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSynchronizedRenderers.h"

#include <array>  // For array
#include <vector> // For vector

class vtkCamera;
class vtkMatrix4x4;

class VTKREMOTINGVIEWS_EXPORT vtkCaveSynchronizedRenderers : public vtkSynchronizedRenderers
{
public:
  static vtkCaveSynchronizedRenderers* New();
  vtkTypeMacro(vtkCaveSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCaveSynchronizedRenderers();
  ~vtkCaveSynchronizedRenderers() override = default;

  /**
   * Called at the start of each render. Overridden to ensure that the camera is
   * updated based on the configuration.
   */
  void HandleStartRender() override;

  /**
   * During constructor, processes the configuration file to setup the display
   * information.
   */
  void ProcessCaveConfiguration();

  /**
   * Set the number of displays to be defined, must be called before DefineDisplay
   */
  void SetNumberOfDisplays(int numberOfDisplays);

  /**
   * Set the eye separation which will be used when defined displays.
   */
  vtkSetMacro(EyeSeparation, double);

  /**
   * Set the use of off axis projection which will be used when defined displays, set to true by
   * default.
   */
  vtkSetMacro(UseOffAxisProjection, bool);

  /**
   * Define a display origin, x and y coordinates at index idx.
   * If idx is the local process id, also set Origin, DisplayX and DisplayY
   * Call SetNumberOfDisplays before calling this method.
   * Return true on success, false otherwise.
   */
  bool DefineDisplay(int idx, double origin[3], double x[3], double y[3]);

  /**
   * Method to update the camera.
   */
  PARAVIEW_DEPRECATED_IN_6_1_0("Use InitializeCamera instead")
  void ComputeCamera(vtkCamera* cam);

  /**
   * Initialize the camera for CAVE mode using display information.
   * Initialization will be performed only once for a given vtkCaveSynchronizedRenderers instance
   * further call will do nothing
   */
  void InitializeCamera(vtkCamera* cam);

  /**
   * Override Superclass to compute Displays parameters in case
   * of a single screen cave (e.g. zSpace)
   * Does not do anything if NumberOfDisplays != 1.
   */
  void SetRenderer(vtkRenderer* renderer) override;

private:
  vtkCaveSynchronizedRenderers(const vtkCaveSynchronizedRenderers&) = delete;
  void operator=(const vtkCaveSynchronizedRenderers&) = delete;

  double EyeSeparation = 0.065;
  bool UseOffAxisProjection = true;
  int NumberOfDisplays = 0;
  std::vector<std::array<double, 12>> Displays;
  std::array<double, 3> DisplayOrigin = { -0.5, -0.5, -0.5 };
  std::array<double, 3> DisplayX = { 0.5, -0.5, -0.5 };
  std::array<double, 3> DisplayY = { 0.5, 0.5, -0.5 };
  bool CameraInitialized = false;
};

#endif
