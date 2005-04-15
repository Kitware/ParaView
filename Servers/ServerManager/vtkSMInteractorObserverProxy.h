/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInteractorObserverProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMInteractorObserverProxy - 
// .SECTION Description

#ifndef __vtkSMInteractorObserverProxy__
#define __vtkSMInteractorObserverProxy__

#include "vtkSMDisplayProxy.h"

class vtkInteractorObserver;
class vtkSMInteractorObserverProxyObserver;
class vtkSMRenderModuleProxy;

class VTK_EXPORT vtkSMInteractorObserverProxy : public vtkSMDisplayProxy
{
public:
  vtkTypeRevisionMacro(vtkSMInteractorObserverProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*) = 0;
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*) = 0;
  
  // Description:
  // Get/Set Enabled state of the InteractorObserver
  virtual void SetEnabled(int e);
  vtkGetMacro(Enabled,int);

  virtual void SaveInBatchScript(ofstream *) { } 

protected:
  vtkSMInteractorObserverProxy();
  ~vtkSMInteractorObserverProxy();

  void SetCurrentRenderModuleProxy(vtkSMRenderModuleProxy* rm);

  // I keep this pointer since some interactor observers may need to access
  // the rendermodule (eg. ScalarBarWidget).
  // Widgets are not enabled until CurrentRenderModuleProxy is set.
  vtkSMRenderModuleProxy* CurrentRenderModuleProxy;

  int Enabled; //flag indicating if the widget is enabled.
  //This is needed since change the Current renderer of the vtk3DWidget
  //does not lead to a call to Enable. 

  virtual void SetCurrentRenderer(vtkSMProxy* renderer);
  virtual void SetInteractor(vtkSMProxy* interactor);

  virtual void CreateVTKObjects(int numObjects);
  
  virtual void InitializeObservers(vtkInteractorObserver* wdg);

  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  vtkSMInteractorObserverProxyObserver* Observer;
//BTX
  friend class vtkSMInteractorObserverProxyObserver;
//ETX

private:
  vtkSMInteractorObserverProxy(const vtkSMInteractorObserverProxy&); // Not implemented
  void operator=(const vtkSMInteractorObserverProxy&); // Not implemented
};


#endif

