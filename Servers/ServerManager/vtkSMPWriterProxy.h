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

  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input, 
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

  virtual void UpdatePipeline();
  virtual void UpdatePipeline(double time);

protected:
  vtkSMPWriterProxy();
  ~vtkSMPWriterProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMPWriterProxy(const vtkSMPWriterProxy&); // Not implemented.
  void operator=(const vtkSMPWriterProxy&); // Not implemented.
};


#endif

