/*=========================================================================

Program:   ParaView
Module:    vtkSMWriterProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMWriterProxy - proxy for a VTK writer on a server
// .SECTION Description
// vtkSMWriterProxy manages VTK writers that are created on a server using
// the proxy pattern.

#ifndef __vtkSMWriterProxy_h
#define __vtkSMWriterProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMWriterProxy : public vtkSMSourceProxy
{
public:
  static vtkSMWriterProxy* New();
  vtkTypeRevisionMacro(vtkSMWriterProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the pipeline and writes the file(s).
  virtual void UpdatePipeline();

protected:
  vtkSMWriterProxy() {}
  ~vtkSMWriterProxy() {}

private:
  vtkSMWriterProxy(const vtkSMWriterProxy&); // Not implemented
  void operator=(const vtkSMWriterProxy&); // Not implemented
};

#endif
