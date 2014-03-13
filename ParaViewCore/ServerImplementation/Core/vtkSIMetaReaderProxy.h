/*=========================================================================

  Program:   ParaView
  Module:    vtkSIMetaReaderProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIMetaReaderProxy
// .SECTION Description
// vtkSISourceProxy is the server-side helper for a vtkSMSourceProxy.
// It adds support to handle various vtkAlgorithm specific Invoke requests
// coming from the client. vtkSISourceProxy also inserts post-processing filters
// for each output port from the vtkAlgorithm. These post-processing filters
// deal with things like parallelizing the data etc.

#ifndef __vtkSIMetaReaderProxy_h
#define __vtkSIMetaReaderProxy_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class vtkAlgorithm;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIMetaReaderProxy : public vtkSISourceProxy
{
public:
  static vtkSIMetaReaderProxy* New();
  vtkTypeMacro(vtkSIMetaReaderProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX

protected:
  vtkSIMetaReaderProxy();
  ~vtkSIMetaReaderProxy();

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
  vtkSIMetaReaderProxy(const vtkSIMetaReaderProxy&); // Not implemented
  void operator=(const vtkSIMetaReaderProxy&); // Not implemented

//ETX
};

#endif
