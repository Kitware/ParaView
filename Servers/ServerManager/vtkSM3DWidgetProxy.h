/*=========================================================================

  Program:   ParaView
  Module:    vtkSM3DWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSM3DWidgetProxy - 
// .SECTION Description


#ifndef __vtkSM3DWidgetProxy_h
#define __vtkSM3DWidgetProxy_h

#include "vtkSMInteractorObserverProxy.h"

class vtkPVProcessModule;
class vtkRenderer;
class vtkInteractorObserver;

class VTK_EXPORT vtkSM3DWidgetProxy : public vtkSMInteractorObserverProxy
{
public:
  vtkTypeRevisionMacro(vtkSM3DWidgetProxy, vtkSMInteractorObserverProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Sends a PlaceWidget event to the 3DWidget.
  virtual void PlaceWidget(double bds[6]);

  // Description:
  // Sets Visibility of the 3DWidget. Internally
  // call SetEnabled.
  virtual void SetVisibility(int visible) 
    { this->SetEnabled(visible); } 

  virtual void SaveInBatchScript(ofstream *file);
protected:
  //BTX
  vtkSM3DWidgetProxy();
  ~vtkSM3DWidgetProxy();

  double Bounds[6]; //PlaceWidget bounds

  friend class vtkPV3DWidget;
  virtual void InitializeObservers(vtkInteractorObserver* widget3D); 
  virtual void CreateVTKObjects(int numObjects);

private:  
  vtkSM3DWidgetProxy(const vtkSM3DWidgetProxy&); // Not implemented
  void operator=(const vtkSM3DWidgetProxy&); // Not implemented
  //ETX
};

#endif
