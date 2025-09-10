// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVCenterAxesActor
 *
 * vtkPVCenterAxesActor is an actor for the center-axes used in ParaView. It
 * merely uses vtkAxes as the poly data source.
 */

#ifndef vtkPVCenterAxesActor_h
#define vtkPVCenterAxesActor_h

#include "vtkLookupTable.h"
#include "vtkOpenGLActor.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkAxes;
class vtkPolyDataMapper;

class VTKREMOTINGVIEWS_EXPORT vtkPVCenterAxesActor : public vtkOpenGLActor
{
public:
  static vtkPVCenterAxesActor* New();
  vtkTypeMacro(vtkPVCenterAxesActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If Symmetric is on, the the axis continue to negative values.
   */
  void SetSymmetric(int);

  /**
   * Option for computing normals.  By default they are computed.
   */
  void SetComputeNormals(int);

  void SetXAxisColor(double r, double g, double b);
  void SetYAxisColor(double r, double g, double b);
  void SetZAxisColor(double r, double g, double b);

protected:
  vtkPVCenterAxesActor();
  ~vtkPVCenterAxesActor() override;

  vtkAxes* Axes;
  vtkPolyDataMapper* Mapper;
  vtkNew<vtkLookupTable> LUT;

private:
  vtkPVCenterAxesActor(const vtkPVCenterAxesActor&) = delete;
  void operator=(const vtkPVCenterAxesActor&) = delete;

  void SetAxisColor(int axis, double r, double g, double b);
};

#endif
