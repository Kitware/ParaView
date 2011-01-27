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
// .NAME vtkPMXMLAnimationWriterRepresentationProperty
// .SECTION Description
// vtkPMXMLAnimationWriterRepresentationProperty extends vtkPMInputProperty to
// add push-API specific to vtkXMLPVAnimationWriter to add representations while
// assigning them unique names consistently across all processes.

#ifndef __vtkPMXMLAnimationWriterRepresentationProperty_h
#define __vtkPMXMLAnimationWriterRepresentationProperty_h

#include "vtkPMInputProperty.h"

class VTK_EXPORT vtkPMXMLAnimationWriterRepresentationProperty : public vtkPMInputProperty
{
public:
  static vtkPMXMLAnimationWriterRepresentationProperty* New();
  vtkTypeMacro(vtkPMXMLAnimationWriterRepresentationProperty, vtkPMInputProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMXMLAnimationWriterRepresentationProperty();
  ~vtkPMXMLAnimationWriterRepresentationProperty();

  // Description:
  // Overridden to call AddRepresentation on the vtkXMLPVAnimationWriter
  // instance with correct API.
  virtual bool Push(vtkSMMessage*, int);

private:
  vtkPMXMLAnimationWriterRepresentationProperty(const vtkPMXMLAnimationWriterRepresentationProperty&); // Not implemented
  void operator=(const vtkPMXMLAnimationWriterRepresentationProperty&); // Not implemented
//ETX
};

#endif
