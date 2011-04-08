/*=========================================================================

  Program:   ParaView
  Module:    vtkSISelectionRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSISelectionRepresentationProxy
// .SECTION Description
// Representation use to show selection. This shows only the selection i.e.
// output of ExtractSelection filter.

#ifndef __vtkSISelectionRepresentationProxy_h
#define __vtkSISelectionRepresentationProxy_h

#include "vtkSIProxy.h"

class vtkSMDataLabelRepresentationProxy;

class VTK_EXPORT vtkSISelectionRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSISelectionRepresentationProxy* New();
  vtkTypeMacro(vtkSISelectionRepresentationProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSISelectionRepresentationProxy();
  ~vtkSISelectionRepresentationProxy();

  // Description:
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkSISelectionRepresentationProxy(const vtkSISelectionRepresentationProxy&); // Not implemented
  void operator=(const vtkSISelectionRepresentationProxy&); // Not implemented
//ETX
};

#endif
