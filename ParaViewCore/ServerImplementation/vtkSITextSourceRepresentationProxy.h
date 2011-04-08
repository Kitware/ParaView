/*=========================================================================

  Program:   ParaView
  Module:    vtkSITextSourceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSITextSourceRepresentationProxy
// .SECTION Description
// vtkSITextSourceRepresentationProxy is the proxy for
// (representations, TextSourceRepresentation). Merely overrides
// CreateVTKObjects to ensure that the subproxies are passed to the
// vtkTextSourceRepresentation correctly.

#ifndef __vtkSITextSourceRepresentationProxy_h
#define __vtkSITextSourceRepresentationProxy_h

#include "vtkSIProxy.h"

class vtkSMViewProxy;

class VTK_EXPORT vtkSITextSourceRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSITextSourceRepresentationProxy* New();
  vtkTypeMacro(vtkSITextSourceRepresentationProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

// BTX
protected:
  vtkSITextSourceRepresentationProxy();
  ~vtkSITextSourceRepresentationProxy();

  // Description:
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkSITextSourceRepresentationProxy(const vtkSITextSourceRepresentationProxy&); // Not implemented
  void operator=(const vtkSITextSourceRepresentationProxy&); // Not implemented
//ETX
};

#endif
