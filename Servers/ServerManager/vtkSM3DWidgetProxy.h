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
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Sends a PlaceWidget event to the 3DWidget.
  // A PlaceWidget call on most of the 3DWidgets (eg. ImplicitPlane)
  // leads to changes in some of the properties of the widget (such as
  // center/normal). These changes are accepted or totally ignored
  // depending on the status of the IgnorePlaceWidgetChanges flag.
  virtual void PlaceWidget(double bds[6]);

  // Description:
  // A PlaceWidget call on most of the 3DWidgets (eg. ImplicitPlane)
  // leads to changes in some of the properties of the widget (such as
  // center/normal). This flag indicates if the changes to the widget 
  // due to a call to PlaceWidget are to be accepted or ignored.
  // They are ignored when this flag is set and accepted otherwise.
  // By default, this flag is not set, hence the changes are accepted.
  vtkSetMacro(IgnorePlaceWidgetChanges,int);
  
  // Description:
  // Sets Visibility of the 3DWidget. Internally
  // calls SetEnabled.
  virtual void SetVisibility(int visible) 
    { this->SetEnabled(visible); } 

  virtual void SaveInBatchScript(ofstream *file);

   // Description:
   // Update the VTK object on the server by pushing the values of all 
   // modified properties (un-modified properties are ignored). If the 
   // object has not been created, it will be created first.
   virtual void UpdateVTKObjects();
protected:
  //BTX
  vtkSM3DWidgetProxy();
  ~vtkSM3DWidgetProxy();

  // Description:
  // Indicator if the positions suggested on PlaceWidget call on a
  // VTK object are to be ignored. If set, the suggestions are rejected.
  // If not set the suggestions are accepted.
  int IgnorePlaceWidgetChanges; 

  // Description:
  // Indicator if the PlaceWidget message
  // must be sent to the Servers.
  int Placed; 
 
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
