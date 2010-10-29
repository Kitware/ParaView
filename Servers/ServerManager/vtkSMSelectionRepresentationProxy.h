/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSelectionRepresentationProxy
// .SECTION Description
// Representation use to show selection. This shows only the selection i.e.
// output of ExtractSelection filter.

#ifndef __vtkSMSelectionRepresentationProxy_h
#define __vtkSMSelectionRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkSMDataLabelRepresentationProxy;

class VTK_EXPORT vtkSMSelectionRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMSelectionRepresentationProxy* New();
  vtkTypeMacro(vtkSMSelectionRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMSelectionRepresentationProxy();
  ~vtkSMSelectionRepresentationProxy();

  // Description:
  virtual void CreateVTKObjects();

private:
  vtkSMSelectionRepresentationProxy(const vtkSMSelectionRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSelectionRepresentationProxy&); // Not implemented
//ETX
};

#endif

