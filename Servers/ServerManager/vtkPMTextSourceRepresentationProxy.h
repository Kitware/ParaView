/*=========================================================================

  Program:   ParaView
  Module:    vtkPMTextSourceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMTextSourceRepresentationProxy
// .SECTION Description
// vtkPMTextSourceRepresentationProxy is the proxy for
// (representations, TextSourceRepresentation). Merely overrides
// CreateVTKObjects to ensure that the subproxies are passed to the
// vtkTextSourceRepresentation correctly.

#ifndef __vtkPMTextSourceRepresentationProxy_h
#define __vtkPMTextSourceRepresentationProxy_h

#include "vtkPMProxy.h"

class vtkSMViewProxy;

class VTK_EXPORT vtkPMTextSourceRepresentationProxy : public vtkPMProxy
{
public:
  static vtkPMTextSourceRepresentationProxy* New();
  vtkTypeMacro(vtkPMTextSourceRepresentationProxy, vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

// BTX
protected:
  vtkPMTextSourceRepresentationProxy();
  ~vtkPMTextSourceRepresentationProxy();

  // Description:
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkPMTextSourceRepresentationProxy(const vtkPMTextSourceRepresentationProxy&); // Not implemented
  void operator=(const vtkPMTextSourceRepresentationProxy&); // Not implemented
//ETX
};

#endif
