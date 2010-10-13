/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFetchDataProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMFetchDataProxy - filter to deliver data to client.
// representation.
// .SECTION Description
// This is a filter proxy that delivers any data input to client.

#ifndef __vtkSMFetchDataProxy_h
#define __vtkSMFetchDataProxy_h

#include "vtkSMSourceProxy.h"

class vtkDataObject;

class VTK_EXPORT vtkSMFetchDataProxy : public vtkSMSourceProxy
{
public:
  static vtkSMFetchDataProxy* New();
  vtkTypeMacro(vtkSMFetchDataProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the data pipeline.
  // Overridden to pass meta-data to the vtk-filter.
  virtual void UpdatePipeline();
  virtual void UpdatePipeline(double time);

  // Description:
  // Retrieves the local data. Use only after UpdatePipeline() has been called.
  vtkDataObject* GetData();
//BTX
protected:
  vtkSMFetchDataProxy();
  ~vtkSMFetchDataProxy();

  virtual void PassMetaData(double time, bool use_time);

private:
  vtkSMFetchDataProxy(const vtkSMFetchDataProxy&); // Not implemented
  void operator=(const vtkSMFetchDataProxy&); // Not implemented
//ETX
};

#endif
