/*=========================================================================

  Program:   ParaView
  Module:    vtkCaveSynchronizedRenderers.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * from vtkPVServerOptions::Geometry and the client camera view angle.
 */

#ifndef vtkCaveSynchronizedRenderers_h
#define vtkCaveSynchronizedRenderers_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSynchronizedRenderers.h"

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
  ~vtkCaveSynchronizedRenderers() override;

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
   * These are to initialize the displays. (This is directly copied from
   * vtkCaveRenderManager).
   */
  void SetNumberOfDisplays(int numberOfDisplays);

  /**
   * Set the eye separation for all the displays.
   */
  void SetEyeSeparation(double eyeSeparation);

  void DefineDisplay(int idx, double origin[3], double x[3], double y[3]);

  /**
   * Method to update the camera.
   */
  void ComputeCamera(vtkCamera* cam);

  /**
   * Override Superclass to compute Displays parameters in case
   * of a single screen cave (e.g. zSpace)
   */
  void SetRenderer(vtkRenderer* renderer) override;

  double EyeSeparation;
  int NumberOfDisplays;
  double** Displays;
  double DisplayOrigin[3];
  double DisplayX[3];
  double DisplayY[3];
  int once;

private:
  vtkCaveSynchronizedRenderers(const vtkCaveSynchronizedRenderers&) = delete;
  void operator=(const vtkCaveSynchronizedRenderers&) = delete;
};

#endif
