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
// .NAME vtkPrismView
// .SECTION Description
// Derived from vtkPVRenderView to overload Update() function. This is
// so common scaling can be computed between PrismRepresentations.
//
#ifndef __vtkPrismView_h
#define __vtkPrismView_h

#include "vtkPVRenderView.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"

class vtkInformationDoubleVectorKey;

class VTK_EXPORT vtkPrismView : public vtkPVRenderView
{
  //*****************************************************************
public:
  static vtkPrismView* New();
  vtkTypeMacro(vtkPrismView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Scaling that should be applied to each object
  // to have it corr
  static vtkInformationDoubleVectorKey* PRISM_WORLD_SCALE();

  // Description:
  // Calls vtkView::REQUEST_INFORMATION() on all representations
  void GatherRepresentationInformation();

public:

//BTX
protected:
  vtkPrismView();
  ~vtkPrismView();

private:
  vtkPrismView(const vtkPrismView&); // Not implemented
  void operator=(const vtkPrismView&); // Not implemented

//ETX


};

#endif
