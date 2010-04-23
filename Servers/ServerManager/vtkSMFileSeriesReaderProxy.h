/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFileSeriesReaderProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMFileSeriesReaderProxy - Proxy for vtkFileSeriesReader
// .SECTION Description
// vtkSMFileSeriesReaderProxy is used to manager vtkFileSeriesReaders.
// It creates the internal reader as a sub-proxy and sets it on the
// meta-reader in CreateVTKObjects(). It also sets FileNameMethod from
// the xml attribute file_name_method.
// .SECTION See Also
// vtkFileSeriesReader

#ifndef __vtkSMFileSeriesReaderProxy_h
#define __vtkSMFileSeriesReaderProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMFileSeriesReaderProxy : public vtkSMSourceProxy
{
public:
  static vtkSMFileSeriesReaderProxy* New();
  vtkTypeMacro(vtkSMFileSeriesReaderProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMFileSeriesReaderProxy();
  ~vtkSMFileSeriesReaderProxy();

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  virtual void CreateVTKObjects();

  // This is the name of the method used to set the file name on the
  // internal reader. See vtkFileSeriesReader for details.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

  char* FileNameMethod;

private:
  vtkSMFileSeriesReaderProxy(const vtkSMFileSeriesReaderProxy&); // Not implemented.
  void operator=(const vtkSMFileSeriesReaderProxy&); // Not implemented.
};

#endif

