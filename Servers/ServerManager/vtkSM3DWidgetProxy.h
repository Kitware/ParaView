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
// .NAME vtkSM3DWidgetProxy - abstract base class for 3DWiget proxies.
// .SECTION Description
// This is an abstract base class fo 3D Widget proxies. 3D Widget proxies are 
// display proxies which require interactor. The interactors on the servers 
// are dummy interactors and not used to perform any opertations. However, 
// they are required, due to the way 3D widgets are desgined.


#ifndef __vtkSM3DWidgetProxy_h
#define __vtkSM3DWidgetProxy_h

#include "vtkSMDisplayProxy.h"

class vtk3DWidget;
class vtkPVProcessModule;
class vtkRenderer;
class vtkSM3DWidgetProxyObserver;
class vtkSMRenderModuleProxy;

class VTK_EXPORT vtkSM3DWidgetProxy : public vtkSMDisplayProxy
{
public:
  vtkTypeRevisionMacro(vtkSM3DWidgetProxy, vtkSMDisplayProxy);
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

  // Description:
  // Get/Set Enabled state of the InteractorObserver
  virtual void SetEnabled(int e);
  vtkGetMacro(Enabled,int);

  // Description:
  // vtkSMDisplayProxy provides a generic SaveInBatchScript.
  // vtkSM3DWidgets don't use that
  // since the order in which the properties are set is significant
  // for them e.g. PlaceWidget must happend before properties are set etc.
  // This is not favourable, but until that is resolved, we do
  // this.
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
  // Overloaded to hack for IgnorePlaceWidgetChanges flag which must be
  // set to 1 in SM state. This is the most weird way of doing things. I
  // have to get rid of IgnorePlaceWidgetChanges flag soon.
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);

  // Description:
  // Indicator if the PlaceWidget message
  // must be sent to the Servers.
  int Placed; 
 
  double Bounds[6]; //PlaceWidget bounds

  friend class vtkPV3DWidget;
  void InitializeObservers(vtk3DWidget* widget3D); 
  virtual void CreateVTKObjects(int numObjects);

  void SetCurrentRenderModuleProxy(vtkSMRenderModuleProxy* rm);

  // I keep this pointer since some interactor observers may need to access
  // the rendermodule (eg. ScalarBarWidget).
  // Widgets are not enabled until CurrentRenderModuleProxy is set.
  vtkSMRenderModuleProxy* CurrentRenderModuleProxy;

  int Enabled; //flag indicating if the widget is enabled.
  //This is needed since change the Current renderer of the vtk3DWidget
  //does not lead to a call to Enable. 

  // Description
  // Sets the server 3D widget's current renderer and interactor.
  void SetCurrentRenderer(vtkSMProxy* renderer);
  void SetInteractor(vtkSMProxy* interactor);

  // Description:
  // Subclasses override this method to get the values from
  // server objects and update the proxy state. This must be
  // done before calling vtkSM3DWidgetProxy::ExecuteEvent
  // since it raises vtkCommand::WidgetModifiedEvent
  // which tells the GUI to update itself using the Proxy 
  // values.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  vtkSM3DWidgetProxyObserver* Observer;
//BTX
  friend class vtkSM3DWidgetProxyObserver;
//ETX
private:  
  vtkSM3DWidgetProxy(const vtkSM3DWidgetProxy&); // Not implemented
  void operator=(const vtkSM3DWidgetProxy&); // Not implemented
  //ETX
};

#endif
