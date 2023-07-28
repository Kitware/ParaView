// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/
// .NAME vtkPVRenderViewWithEDL
// .SECTION Description
// vtkPVRenderViewWithEDL aims to create a custom render-view
// that uses an image-processing render pass for processing the image
// before rendering it on the screen.

#ifndef vtkPVRenderViewWithEDL_h
#define vtkPVRenderViewWithEDL_h

#include "vtkPVRenderView.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVRenderViewWithEDL : public vtkPVRenderView
{
public:
  static vtkPVRenderViewWithEDL* New();
  vtkTypeMacro(vtkPVRenderViewWithEDL, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVRenderViewWithEDL();

private:
  vtkPVRenderViewWithEDL(const vtkPVRenderViewWithEDL&) = delete;
  void operator=(const vtkPVRenderViewWithEDL&) = delete;
};

#endif
