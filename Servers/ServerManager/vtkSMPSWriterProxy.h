/*=========================================================================

Program:   ParaView
Module:    vtkSMPSWriterProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPSWriterProxy - proxy for the parallel-serial writer.
// .SECTION Description
// vtkSMPSWriterProxy is the proxy for all vtkParallelSerialWriter
// objects. It is responsible of setting the internal writer that is
// configured as a sub-proxy.

#ifndef __vtkSMPSWriterProxy_h
#define __vtkSMPSWriterProxy_h

#include "vtkSMPWriterProxy.h"

class VTK_EXPORT vtkSMPSWriterProxy : public vtkSMPWriterProxy
{
public:
  static vtkSMPSWriterProxy* New();
  vtkTypeRevisionMacro(vtkSMPSWriterProxy, vtkSMPWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMPSWriterProxy();
  ~vtkSMPSWriterProxy();

  virtual void CreateVTKObjects();

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  // This is the name of the method used to set the file name on the
  // internal reader. See vtkFileSeriesReader for details.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

  char* FileNameMethod;

private:
  vtkSMPSWriterProxy(const vtkSMPSWriterProxy&); // Not implemented.
  void operator=(const vtkSMPSWriterProxy&); // Not implemented.
};


#endif

