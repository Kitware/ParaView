/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAbstractViewModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAbstractViewModuleProxy - Manages rendering and displaying data.
// .SECTION Description
// This is an abstract super class for all rendering modules.
// Provides very basic functionality.
#ifndef __vtkSMAbstractViewModuleProxy_h
#define __vtkSMAbstractViewModuleProxy_h

#include "vtkSMProxy.h"

class vtkCamera;
class vtkCollection;
class vtkImageData;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkPVRenderModuleHelper;
class vtkSMDisplay;
class vtkSMDisplayProxy;
class vtkPVClientServerIdCollectionInformation;
class vtkTimerLog;

// TODO: have to change the PVCameraManipulators to do ResetCamera on
// the RenderModule rather than renderer.
class VTK_EXPORT vtkSMAbstractViewModuleProxy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMAbstractViewModuleProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Any display that must be rendered by this rendermodule
  // needs to be added to it. 
  // NOTE: If you call this method directly (without using properties)
  // don't forget to call UpdateVTKObjects() on the RenderModule.
  virtual void AddDisplay(vtkSMDisplayProxy* disp);
  virtual void RemoveDisplay(vtkSMDisplayProxy* disp);

  // Description:
  // Removes all added displays. 
  // (Calls RemoveFromRenderModule on all displays).
  virtual void RemoveAllDisplays();

  // Description:
  // Renders using Still/FullRes or interactive/LODs
  virtual void StillRender();
  virtual void InteractiveRender();
  
  // Description
  // Subclass can create their own vtkSMDisplayProxy object by
  // implementing this method.
  // So far, others displays are not.
  virtual vtkSMDisplayProxy* CreateDisplayProxy();

  // Description:
  // Returns the display collection.
  vtkGetObjectMacro(Displays, vtkCollection);

  // Description:
  // Save the display in batch script. This will eventually get 
  // removed as we will generate batch script from ServerManager
  // state. However, until then.
  virtual void SaveInBatchScript(ofstream* file);

  // Description:
  // Called when saving server manager state.
  // Overridden to SynchronizeCameraProperties before saving the properties.
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);

  // Description:
  // Indicates if we should locally render.
  virtual int IsRenderLocal() { return 1; }

  // Description:
  // Update all visible displays (therefore sources)
  virtual void UpdateAllDisplays();  

protected:
  // This is the XMLName of the proxy to get created when CreateDisplayProxy
  // is called. It must be a proxy belonging to the group "displays"
  // and must be a subclass of vtkSMDisplayProxy.
  vtkGetStringMacro(DisplayXMLName);
  vtkSetStringMacro(DisplayXMLName);

  // Return the servers  where the PrepareProgress request
  // must be sent when rendering. By default,
  // it is RENDER_SERVER|CLIENT, however in CompositeRenderModule,
  // when rendering locally, the progress messages need not 
  // be sent to the servers.
  virtual vtkTypeUInt32 GetRenderingProgressServers();

  // Description:
  // Method called before/after Still Render is called.
  // Can be used to set GlobalLODFlag.
  virtual void BeginStillRender();
  virtual void EndStillRender();

  virtual void BeginInteractiveRender();
  virtual void EndInteractiveRender();

  virtual void PerformRender() {};
 
  // Description:
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects(int numObjects);

  vtkSMAbstractViewModuleProxy();
  ~vtkSMAbstractViewModuleProxy();

private:
  // Description:
  // Overridden since Interactor properties must be cleared.
  void UnRegisterVTKObjects();

  // This collection keeps a reference to all Display Proxies added
  // to this module.
  vtkCollection* Displays;

  // This is the XMLName of the proxy to get created when CreateDisplayProxy
  // is called. It must be a proxy belonging to the group "displays"
  // and must be a subclass of vtkSMDisplayProxy.
  char* DisplayXMLName;

private:
  vtkSMAbstractViewModuleProxy(const vtkSMAbstractViewModuleProxy&); // Not implemented.
  void operator=(const vtkSMAbstractViewModuleProxy&); // Not implemented.
};


#endif


