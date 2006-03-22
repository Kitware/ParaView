/*=========================================================================

  Program:   ParaView
  Module:    vtkSMShrunkContoursProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMShrunkContoursProxy - implementing a VTK pipeline in a proxy.
// .SECTION Description
// This proxy is an example illustrating how to set up a pipeline in a proxy.


#ifndef __vtkSMShrunkContoursProxy_h
#define __vtkSMShrunkContoursProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMShrunkContoursProxy : public vtkSMSourceProxy
{
public:
  static vtkSMShrunkContoursProxy* New();
  vtkTypeRevisionMacro(vtkSMShrunkContoursProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description:
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Create n parts where n is the number of filters. Each part
  // correspond to one output of one filter. This method is overridden 
  // to ensure that the parts are created using the last proxy in the pipeline.
  virtual void CreateParts();

protected:
  vtkSMShrunkContoursProxy();
  ~vtkSMShrunkContoursProxy();
  
private:
  vtkSMShrunkContoursProxy(const vtkSMShrunkContoursProxy&); // Not implemented.
  void operator=(const vtkSMShrunkContoursProxy&); // Not implemented.
};

#endif

