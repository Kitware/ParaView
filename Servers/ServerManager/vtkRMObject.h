/*=========================================================================

  Program:   ParaView
  Module:    vtkRMObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRM3DWidget - 
// .SECTION Description


#ifndef __vtkRMObject_h
#define __vtkRMObject_h

#include "vtkSMProxy.h"
#include "vtkClientServerID.h" //needed for Create

class vtkPVProcessModule;

class VTK_EXPORT vtkRMObject : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkRMObject, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  virtual void Create(vtkPVProcessModule *pm, vtkClientServerID rendererID, 
    vtkClientServerID interactorID)=0;

protected:
  vtkRMObject();
  ~vtkRMObject();
  
private:  
  vtkRMObject(const vtkRMObject&); // Not implemented
  void operator=(const vtkRMObject&); // Not implemented
//ETX
};

#endif
