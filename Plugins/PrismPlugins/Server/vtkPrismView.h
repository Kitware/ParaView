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
#include "vtkBoundingBox.h"

class vtkTransform;
class vtkInformationDoubleVectorKey;

class VTK_EXPORT vtkPrismView : public vtkPVRenderView
{
  //*****************************************************************
public:
  static vtkPrismView* New();
  vtkTypeMacro(vtkPrismView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Adds the representation to the view.
  void AddRepresentation(vtkDataRepresentation* rep);

  // Description:
  // Removes the representation from the view.
  void RemoveRepresentation(vtkDataRepresentation* rep);

  // Description:
  // Scaling that should be applied to each object
  // to have it corr
  static vtkInformationDoubleVectorKey* PRISM_GEOMETRY_BOUNDS();

  // Description:
  // Calls vtkView::REQUEST_INFORMATION() on all representations
  void GatherRepresentationInformation();

//BTX
protected:
  vtkPrismView();
  ~vtkPrismView();

  void UpdateWorldScale(const vtkBoundingBox& worldBounds);

  vtkTransform *Transform;

private:
  vtkPrismView(const vtkPrismView&); // Not implemented
  void operator=(const vtkPrismView&); // Not implemented

//ETX


};

#endif
