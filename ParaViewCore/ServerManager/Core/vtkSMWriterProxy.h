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
// vtkSMWriterProxy also provides support for meta-writers. i.e. if the proxy
// has a subproxy name "Writer" that this proxy represents a meta-writer which
// uses the given writer as the writer to write each component.

#ifndef __vtkSMWriterProxy_h
#define __vtkSMWriterProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMWriterProxy : public vtkSMSourceProxy
{
public:
  static vtkSMWriterProxy* New();
  vtkTypeMacro(vtkSMWriterProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the pipeline and writes the file(s).
  // Must call UpdateVTKObjects() before calling UpdatePipeline()
  // to ensure that the filename etc. are set correctly.
  virtual void UpdatePipeline();

  // Description:
  // Updates the pipeline and writes the file(s).
  // Must call UpdateVTKObjects() before calling UpdatePipeline()
  // to ensure that the filename etc. are set correctly.
  virtual void UpdatePipeline(double time);

  // Description:
  // Flag indicating if the writer supports writing in parallel.
  // Not set by default.
  vtkSetMacro(SupportsParallel, int);
  int GetSupportsParallel()
  {
    return this->SupportsParallel || this->ParallelOnly;
  }

  // Description:
  // Flag indicating if the writer works only in parallel. If this is set,
  // SupportsParallel is always true.
  vtkGetMacro(ParallelOnly, int);
  vtkSetMacro(ParallelOnly, int);

protected:
  vtkSMWriterProxy();
  ~vtkSMWriterProxy();

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element);

  int SupportsParallel;
  int ParallelOnly;

  // This is the name of the method used to set the file name on the
  // internal reader. See vtkFileSeriesReader for details.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

  char* FileNameMethod;
private:
  vtkSMWriterProxy(const vtkSMWriterProxy&); // Not implemented
  void operator=(const vtkSMWriterProxy&); // Not implemented
};

#endif
