/*=========================================================================

Program:   ParaView
Module:    vtkSMPWriterProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPWriterProxy - proxy for a VTK writer that supports parallel 
// writing.
// .SECTION Description
// vtkSMPWriterProxy is the proxy for all writers that can write in parallel.

#ifndef __vtkSMPWriterProxy_h
#define __vtkSMPWriterProxy_h

#include "vtkSMWriterProxy.h"

class VTK_EXPORT vtkSMPWriterProxy : public vtkSMWriterProxy
{
public:
  static vtkSMPWriterProxy* New();
  vtkTypeRevisionMacro(vtkSMPWriterProxy, vtkSMWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void AddInput(vtkSMSourceProxy* input, 
                const char* method,
                int hasMultipleInputs);

  virtual void UpdatePipeline();
protected:
  vtkSMPWriterProxy();
  ~vtkSMPWriterProxy();

  // Description:
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects(int numObjects);

private:
  vtkSMPWriterProxy(const vtkSMPWriterProxy&); // Not implemented.
  void operator=(const vtkSMPWriterProxy&); // Not implemented.
};


#endif

