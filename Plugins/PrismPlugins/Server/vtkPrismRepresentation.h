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
// .NAME vtkPrismRepresentation
// .SECTION Description
// vtkPrismRepresentation extends vtkGeometryRepresentationWithFaces to add
// support auto scaling for Prism filters.

#ifndef __vtkPrismRepresentation_h
#define __vtkPrismRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkTransformFilter.h"
#include "vtkTransform.h"
#include "vtkSmartPointer.h"

class VTK_EXPORT vtkPrismRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkPrismRepresentation* New();
  vtkTypeMacro(vtkPrismRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetScaleFactor(double scale[3]);

 void GetPrismRange(double range[3]);

//BTX
protected:
  vtkPrismRepresentation();
  ~vtkPrismRepresentation();

  // Description:
  // Subclasses should override this to connect inputs to the internal pipeline
  // as necessary. Since most representations are "meta-filters" (i.e. filters
  // containing other filters), you should create shallow copies of your input
  // before connecting to the internal pipeline. The convenience method
  // GetInternalOutputPort will create a cached shallow copy of a specified
  // input for you. The related helper functions GetInternalAnnotationOutputPort,
  // GetInternalSelectionOutputPort should be used to obtain a selection or
  // annotation port whose selections are localized for a particular input data object.
  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);


  class MyInternal;
  MyInternal* Internal;


private:
  vtkPrismRepresentation(const vtkPrismRepresentation&); // Not implemented
  void operator=(const vtkPrismRepresentation&); // Not implemented
//ETX
};

#endif
