/*=========================================================================

  Program:   ParaView
  Module:    vtkPMSelectionRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMSelectionRepresentationProxy
// .SECTION Description
// Representation use to show selection. This shows only the selection i.e.
// output of ExtractSelection filter.

#ifndef __vtkPMSelectionRepresentationProxy_h
#define __vtkPMSelectionRepresentationProxy_h

#include "vtkPMProxy.h"

class vtkSMDataLabelRepresentationProxy;

class VTK_EXPORT vtkPMSelectionRepresentationProxy : public vtkPMProxy
{
public:
  static vtkPMSelectionRepresentationProxy* New();
  vtkTypeMacro(vtkPMSelectionRepresentationProxy, vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMSelectionRepresentationProxy();
  ~vtkPMSelectionRepresentationProxy();

  // Description:
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkPMSelectionRepresentationProxy(const vtkPMSelectionRepresentationProxy&); // Not implemented
  void operator=(const vtkPMSelectionRepresentationProxy&); // Not implemented
//ETX
};

#endif
