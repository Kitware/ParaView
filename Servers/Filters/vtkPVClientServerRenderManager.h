/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerRenderManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVClientServerRenderManager - superclass for render managers used in
// client-server mode.
// .SECTION Description
// vtkPVClientServerRenderManager is the superclass for render managers used in
// client-server mode. It handles the initialization of the parallel render
// manager for ParaView's client-server operation.
// This class is designed to make is possible to use the render manager for a
// server with multiple client connections (or even vice-versa [theoretically]).
#ifndef __vtkPVClientServerRenderManager_h
#define __vtkPVClientServerRenderManager_h

#include "vtkParallelRenderManager.h"

class vtkRemoteConnection;
class VTK_EXPORT vtkPVClientServerRenderManager : public vtkParallelRenderManager
{
public:
  vtkTypeRevisionMacro(vtkPVClientServerRenderManager, vtkParallelRenderManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform connection specific initialization e.g. registering callbacks for
  // Socket RMIs etc.
  void Initialize(vtkRemoteConnection*);

  // Description:
  // Overridden to warn against calling these method directly. Using these
  // methods makes it difficult to use the parallel render manager with multiple
  // connections.
  virtual void InitializeRMIs();
  virtual void SetController(vtkMultiProcessController*);

  // Description:
  // All these methods are overridden to ensure that this->Controller is setup
  // correctly before the
  virtual void RenderRMI()
    {
    this->Activate();
    this->Superclass::RenderRMI();
    this->DeActivate();
    }

  virtual void GenericStartRenderCallback()
    {
    this->Activate();
    this->Superclass::GenericStartRenderCallback();
    }
  virtual void GenericEndRenderCallback()
    {
    this->Superclass::GenericEndRenderCallback();
    this->DeActivate();
    }

//BTX
protected:
  vtkPVClientServerRenderManager();
  ~vtkPVClientServerRenderManager();

  void Activate();
  void DeActivate();

private:
  vtkPVClientServerRenderManager(const vtkPVClientServerRenderManager&); // Not implemented
  void operator=(const vtkPVClientServerRenderManager&); // Not implemented
  
  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

