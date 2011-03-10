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

class VTK_EXPORT vtkPrismView : public vtkPVRenderView
{
  //*****************************************************************
public:
  static vtkPrismView* New();
  vtkTypeMacro(vtkPrismView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to not call Update() directly on the input representations,
  // instead use ProcessViewRequest() for all vtkPVDataRepresentations.
  void Update();



public:

//BTX
protected:
  vtkPrismView();
  ~vtkPrismView();
 //// Subclass "hooks" for notifying subclasses of vtkView when representations are added
 // // or removed. Override these methods to perform custom actions.
 // virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
 // virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

private:
  vtkPrismView(const vtkPrismView&); // Not implemented
  void operator=(const vtkPrismView&); // Not implemented

//ETX


};

#endif
