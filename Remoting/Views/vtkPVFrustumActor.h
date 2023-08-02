// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVFrustumActor
 *
 * vtkPVFrustumActor is an actor that renders a frustum. Used in ParaView to
 * show the frustum used for frustum selection extraction.
 */

#ifndef vtkPVFrustumActor_h
#define vtkPVFrustumActor_h

#include "vtkOpenGLActor.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkOutlineSource;
class vtkPolyDataMapper;

class VTKREMOTINGVIEWS_EXPORT vtkPVFrustumActor : public vtkOpenGLActor
{
public:
  static vtkPVFrustumActor* New();
  vtkTypeMacro(vtkPVFrustumActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the frustum.
   */
  void SetFrustum(double corners[24]);

  ///@{
  /**
   * Convenience method to set the color.
   */
  void SetColor(double r, double g, double b);
  void SetLineWidth(double r);
  ///@}

protected:
  vtkPVFrustumActor();
  ~vtkPVFrustumActor() override;

  vtkOutlineSource* Outline;
  vtkPolyDataMapper* Mapper;

private:
  vtkPVFrustumActor(const vtkPVFrustumActor&) = delete;
  void operator=(const vtkPVFrustumActor&) = delete;
};

#endif
