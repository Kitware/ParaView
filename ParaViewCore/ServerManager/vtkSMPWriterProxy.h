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
// The only extra functionality provided by this class is that it sets up the
// piece information on the writer.

#ifndef __vtkSMPWriterProxy_h
#define __vtkSMPWriterProxy_h

#include "vtkSMWriterProxy.h"

class VTK_EXPORT vtkSMPWriterProxy : public vtkSMWriterProxy
{
public:
  static vtkSMPWriterProxy* New();
  vtkTypeMacro(vtkSMPWriterProxy, vtkSMWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMPWriterProxy();
  ~vtkSMPWriterProxy();

private:
  vtkSMPWriterProxy(const vtkSMPWriterProxy&); // Not implemented.
  void operator=(const vtkSMPWriterProxy&); // Not implemented.
};


#endif

