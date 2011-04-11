/*=========================================================================

  Program:   ParaView
  Module:    vtkSIPVRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIPVRepresentationProxy
// .SECTION Description
// vtkSIPVRepresentationProxy is the helper for vtkSMPVRepresentationProxy.

#ifndef __vtkSIPVRepresentationProxy_h
#define __vtkSIPVRepresentationProxy_h

#include "vtkSIProxy.h"

class VTK_EXPORT vtkSIPVRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSIPVRepresentationProxy* New();
  vtkTypeMacro(vtkSIPVRepresentationProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIPVRepresentationProxy();
  ~vtkSIPVRepresentationProxy();

  // Description:
  // Parses the XML to create property/subproxy helpers.
  // Overridden to parse all the "RepresentationType" elements.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

private:
  vtkSIPVRepresentationProxy(const vtkSIPVRepresentationProxy&); // Not implemented
  void operator=(const vtkSIPVRepresentationProxy&); // Not implemented

  void OnVTKObjectModified();

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
