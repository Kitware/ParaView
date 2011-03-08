/*=========================================================================

  Program:   ParaView
  Module:    vtkSIFileSeriesReaderProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIFileSeriesReaderProxy
// .SECTION Description
// vtkSISourceProxy is the server-side helper for a vtkSMSourceProxy.
// It adds support to handle various vtkAlgorithm specific Invoke requests
// coming from the client. vtkSISourceProxy also inserts post-processing filters
// for each output port from the vtkAlgorithm. These post-processing filters
// deal with things like parallelizing the data etc.

#ifndef __vtkSIFileSeriesReaderProxy_h
#define __vtkSIFileSeriesReaderProxy_h

#include "vtkSISourceProxy.h"

class vtkAlgorithm;

class VTK_EXPORT vtkSIFileSeriesReaderProxy : public vtkSISourceProxy
{
public:
  static vtkSIFileSeriesReaderProxy* New();
  vtkTypeMacro(vtkSIFileSeriesReaderProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX

protected:
  vtkSIFileSeriesReaderProxy();
  ~vtkSIFileSeriesReaderProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

  // Description:
  // Read xml-attributes.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);

  // This is the name of the method used to set the file name on the
  // internal reader. See vtkFileSeriesReader for details.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

  char* FileNameMethod;

private:
  vtkSIFileSeriesReaderProxy(const vtkSIFileSeriesReaderProxy&); // Not implemented
  void operator=(const vtkSIFileSeriesReaderProxy&); // Not implemented

//ETX
};

#endif
