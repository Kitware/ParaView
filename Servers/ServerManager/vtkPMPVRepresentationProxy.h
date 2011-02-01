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
// .NAME vtkPMPVRepresentationProxy
// .SECTION Description
// vtkPMPVRepresentationProxy is the helper for vtkSMPVRepresentationProxy.

#ifndef __vtkPMPVRepresentationProxy_h
#define __vtkPMPVRepresentationProxy_h

#include "vtkPMProxy.h"

class VTK_EXPORT vtkPMPVRepresentationProxy : public vtkPMProxy
{
public:
  static vtkPMPVRepresentationProxy* New();
  vtkTypeMacro(vtkPMPVRepresentationProxy, vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Triggers UpdateInformation().
  virtual void UpdateInformation();

//BTX
protected:
  vtkPMPVRepresentationProxy();
  ~vtkPMPVRepresentationProxy();

  // Description:
  // Parses the XML to create property/subproxy helpers.
  // Overridden to parse all the "RepresentationType" elements.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

private:
  vtkPMPVRepresentationProxy(const vtkPMPVRepresentationProxy&); // Not implemented
  void operator=(const vtkPMPVRepresentationProxy&); // Not implemented

  void OnVTKObjectModified();

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
