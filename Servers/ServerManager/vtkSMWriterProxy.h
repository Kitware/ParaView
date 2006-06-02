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
  // Must call UpdateVTKObjects() before calling UpdatePipeline()
  // to ensure that the filename etc. are set correctly.
  virtual void UpdatePipeline();

  // Description:
  // Get the error code for the last UpdatePipeline() call. UpdatePipeline()
  // call writes the file(s) and updates the error status. Error codes are 
  // defined in vtkErrorCode.h
  vtkGetMacro(ErrorCode, int);

  // Description:
  // Flag indicating if the writer supports writing in parallel.
  // Not set by default.
  vtkGetMacro(SupportsParallel, int);
  vtkSetMacro(SupportsParallel, int);

protected:
  vtkSMWriterProxy();
  ~vtkSMWriterProxy() {}

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  int ErrorCode;
  int SupportsParallel;
private:
  vtkSMWriterProxy(const vtkSMWriterProxy&); // Not implemented
  void operator=(const vtkSMWriterProxy&); // Not implemented
};

#endif
