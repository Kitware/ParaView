/*=========================================================================

  Program:   ParaView
  Module:    vtkPMFileSeriesReaderProxy

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMFileSeriesReaderProxy
// .SECTION Description
// vtkPMSourceProxy is the server-side helper for a vtkSMSourceProxy.
// It adds support to handle various vtkAlgorithm specific Invoke requests
// coming from the client. vtkPMSourceProxy also inserts post-processing filters
// for each output port from the vtkAlgorithm. These post-processing filters
// deal with things like parallelizing the data etc.

#ifndef __vtkPMFileSeriesReaderProxy_h
#define __vtkPMFileSeriesReaderProxy_h

#include "vtkPMSourceProxy.h"

class vtkAlgorithm;

class VTK_EXPORT vtkPMFileSeriesReaderProxy : public vtkPMSourceProxy
{
public:
  static vtkPMFileSeriesReaderProxy* New();
  vtkTypeMacro(vtkPMFileSeriesReaderProxy, vtkPMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX

protected:
  vtkPMFileSeriesReaderProxy();
  ~vtkPMFileSeriesReaderProxy();

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
  vtkPMFileSeriesReaderProxy(const vtkPMFileSeriesReaderProxy&); // Not implemented
  void operator=(const vtkPMFileSeriesReaderProxy&); // Not implemented

//ETX
};

#endif
