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

class VTK_EXPORT vtkSMInteractorObserverProxy : public vtkSMDisplayerProxy
{
public:
  vtkTypeRevisionMacro(vtkSMInteractorObserverProxy, vtkSMDisplayerProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set Enabled state of the InteractorObserver
  vtkSetMacro(Enabled,int);
  vtkGetMacro(Enabled,int);

  virtual void SaveInBatchScript(ofstream *) { };

  // Description:
  // Adds this displayer to the Display window proxy
  virtual void AddToDisplayWindow(vtkSMDisplayWindowProxy* dw);

   // Description:
   // Update the VTK object on the server by pushing the values of all 
   // modifed properties (un-modified properties are ignored). If the 
   // object has not been created, it will be created first.
   virtual void UpdateVTKObjects();

protected:
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
//BTX
  friend class vtkSMInteractorObserverProxyObserver;
//ETX
private:
  vtkSMInteractorObserverProxy(const vtkSMInteractorObserverProxy&); // Not implemented
  void operator=(const vtkSMInteractorObserverProxy&); // Not implemented
};


#endif

