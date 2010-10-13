/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextSourceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTextSourceRepresentationProxy
// .SECTION Description
// vtkSMTextSourceRepresentationProxy is the proxy for
// (representations, TextSourceRepresentation). Merely overrides
// CreateVTKObjects to ensure that the subproxies are passed to the
// vtkTextSourceRepresentation correctly.

#ifndef __vtkSMTextSourceRepresentationProxy_h
#define __vtkSMTextSourceRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkSMTextWidgetRepresentationProxy;
class vtkSMViewProxy;

class VTK_EXPORT vtkSMTextSourceRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMTextSourceRepresentationProxy* New();
  vtkTypeMacro(vtkSMTextSourceRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

// BTX
protected:
  vtkSMTextSourceRepresentationProxy();
  ~vtkSMTextSourceRepresentationProxy();

  // Description:
  virtual void CreateVTKObjects();
private:
  vtkSMTextSourceRepresentationProxy(const vtkSMTextSourceRepresentationProxy&); // Not implemented
  void operator=(const vtkSMTextSourceRepresentationProxy&); // Not implemented
//ETX
};

#endif
