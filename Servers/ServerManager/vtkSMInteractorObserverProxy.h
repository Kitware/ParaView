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

#include "vtkSMDisplayerProxy.h"

class vtkInteractorObserver;
class vtkSMInteractorObserverProxyObserver;
class vtkRenderer;

class VTK_EXPORT vtkSMInteractorObserverProxy : public vtkSMDisplayerProxy
{
public:
  vtkTypeRevisionMacro(vtkSMInteractorObserverProxy, vtkSMDisplayerProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetEnabled(int enable);

  virtual void SaveInBatchScript(ofstream *) { };

  // Description:
  // Adds this displayer to the Display window proxy
  virtual void AddToDisplayWindow(vtkSMDisplayWindowProxy* dw);

protected:
//BTX
  vtkSMInteractorObserverProxy();
  ~vtkSMInteractorObserverProxy();
  
  int Enabled; //flag indicating if the widget is enabled.
  //This is needed since change the Current renderer of the vtk3DWidget
  //does not lead to a call to Enable. 

  void SetCurrentRenderer(vtkClientServerID rendererID);
  void SetInteractor(vtkClientServerID interactorID);

  // Flags indicating if the CurrentRenderer/Interactor are set.
  // Unless they are, SetEnabled messages are not sent to the 
  // VTK Widget
  int RendererInitialized;
  int InteractorInitialized;
  
  virtual void CreateVTKObjects(int numObjects);
  
  virtual void InitializeObservers(vtkInteractorObserver* wdg);

  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  vtkSMInteractorObserverProxyObserver* Observer;
  friend class vtkSMInteractorObserverProxyObserver;
private:
  vtkSMInteractorObserverProxy(const vtkSMInteractorObserverProxy&); // Not implemented
  void operator=(const vtkSMInteractorObserverProxy&); // Not implemented
//ETX  
};


#endif

