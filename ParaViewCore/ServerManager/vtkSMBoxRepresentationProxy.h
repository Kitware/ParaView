/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoxRepresentationProxy - proxy for vtkBoxRepresentation
// .SECTION Description
// vtkSMBoxRepresentationProxy is a proxy for vtkBoxRepresentation. A
// specialization is needed to set the tranform on the vtkBoxRepresentation.

#ifndef __vtkSMBoxRepresentationProxy_h
#define __vtkSMBoxRepresentationProxy_h

#include "vtkSMWidgetRepresentationProxy.h"

class VTK_EXPORT vtkSMBoxRepresentationProxy : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMBoxRepresentationProxy* New();
  vtkTypeMacro(vtkSMBoxRepresentationProxy, vtkSMWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void UpdateVTKObjects();
  virtual void UpdatePropertyInformation();
  virtual void UpdatePropertyInformation(vtkSMProperty* prop)
    { this->Superclass::UpdatePropertyInformation(prop); }

//BTX
protected:
  vtkSMBoxRepresentationProxy();
  ~vtkSMBoxRepresentationProxy();

  // This method is overridden to set the transform on the vtkWidgetRepresentation.
  virtual void CreateVTKObjects();

private:
  vtkSMBoxRepresentationProxy(const vtkSMBoxRepresentationProxy&); // Not implemented
  void operator=(const vtkSMBoxRepresentationProxy&); // Not implemented
//ETX
};

#endif

