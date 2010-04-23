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
  // Get the error code for the last UpdatePipeline() call. UpdatePipeline()
  // call writes the file(s) and updates the error status. Error codes are 
  // defined in vtkErrorCode.h
  vtkGetMacro(ErrorCode, int);

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

  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input, 
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

protected:
  vtkSMWriterProxy();
  ~vtkSMWriterProxy();

  virtual void CreateVTKObjects();

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  int ErrorCode;
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
