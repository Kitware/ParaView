/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDeskTopRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDeskTopRenderModule - IceT with desktop delivery.
// .SECTION Description
// 

#ifndef __vtkPVDeskTopRenderModule_h
#define __vtkPVDeskTopRenderModule_h

#include "vtkPVMPIRenderModule.h"

class vtkCompositeRenderManager;

class VTK_EXPORT vtkPVDeskTopRenderModule : public vtkPVMPIRenderModule
{
public:
  static vtkPVDeskTopRenderModule* New();
  vtkTypeRevisionMacro(vtkPVDeskTopRenderModule,vtkPVMPIRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the application right after construction.
  virtual void SetProcessModule(vtkPVProcessModule *pm);

  void StillRender();
  void InteractiveRender();

  // Description:
  // Superclass supports this, but this class does not.
  virtual void SetUseCompositeCompression(int) {};

protected:
  vtkPVDeskTopRenderModule();
  ~vtkPVDeskTopRenderModule();

  // This is the IceT manager that is ignorent of the client.
  // It runs on all processes of the server.
  vtkSetMacro(DisplayManagerID,vtkClientServerID);
  vtkClientServerID DisplayManagerID;

  vtkPVDeskTopRenderModule(const vtkPVDeskTopRenderModule&); // Not implemented
  void operator=(const vtkPVDeskTopRenderModule&); // Not implemented
};


#endif
