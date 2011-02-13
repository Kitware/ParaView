/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPV2DRenderView
// .SECTION Description
// vtkPV2DRenderView is a render view specialization that enforces 2D
// interactions. Also, it adds a legend for showing the scale in the 2D view.

#ifndef __vtkPV2DRenderView_h
#define __vtkPV2DRenderView_h

#include "vtkPVRenderView.h"

class vtkLegendScaleActor;
class VTK_EXPORT vtkPV2DRenderView : public vtkPVRenderView
{
public:
  static vtkPV2DRenderView* New();
  vtkTypeMacro(vtkPV2DRenderView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the interaction mode. Overridden to force 2D mode when 3D is
  // requested.
  virtual void SetInteractionMode(int mode);

  // Description:
  // Set the visibility for the overlay legend.
  void SetAxisVisibility(bool);

  virtual void SetCenterAxesVisibility(bool)
    { this->Superclass::SetCenterAxesVisibility(false); }
  virtual void SetOrientationAxesInteractivity(bool)
    { this->Superclass::SetOrientationAxesInteractivity(false); }
  virtual void SetOrientationAxesVisibility(bool)
    { this->Superclass::SetOrientationAxesVisibility(false); }

//BTX
protected:
  vtkPV2DRenderView();
  ~vtkPV2DRenderView();
  vtkLegendScaleActor* LegendScaleActor;

private:
  vtkPV2DRenderView(const vtkPV2DRenderView&); // Not implemented
  void operator=(const vtkPV2DRenderView&); // Not implemented
//ETX
};

#endif
