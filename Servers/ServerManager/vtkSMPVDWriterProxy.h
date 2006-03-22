/*=========================================================================

Program:   ParaView
Module:    vtkSMPVDWriterProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPVDWriterProxy - proxy for a VTK writer on a server
// .SECTION Description
// vtkSMPVDWriterProxy manages VTK writers that are created on a server using
// the proxy pattern.

#ifndef __vtkSMPVDWriterProxy_h
#define __vtkSMPVDWriterProxy_h

#include "vtkSMWriterProxy.h"

class VTK_EXPORT vtkSMPVDWriterProxy : public vtkSMWriterProxy
{
public:
  static vtkSMPVDWriterProxy* New();
  vtkTypeRevisionMacro(vtkSMPVDWriterProxy, vtkSMWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the pipeline and writes the file(s).
  virtual void UpdatePipeline();

protected:
  vtkSMPVDWriterProxy() {}
  ~vtkSMPVDWriterProxy() {}

private:
  vtkSMPVDWriterProxy(const vtkSMPVDWriterProxy&); // Not implemented
  void operator=(const vtkSMPVDWriterProxy&); // Not implemented
};

#endif
