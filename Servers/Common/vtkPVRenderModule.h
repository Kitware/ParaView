/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderModule - Mangages rendering and displaying data.
// .SECTION Description

// This is a super class for all rendering modules.
// Subclasses manages creation and manipulation of the render window.  
// No user interface in this class.
// Some common camera manipulation (and other render window control)
// may be moved back into vtkPVRenderView to simplify this module.
// Although som methods are implemented in this class, it is abstract, 
// and is created only for an API.

#ifndef __vtkPVRenderModule_h
#define __vtkPVRenderModule_h

#include "vtkObject.h"

#include "vtkClientServerID.h" // Needed for RenderWindowID and RendererID

class vtkMultiProcessController;
class vtkPVProcessModule;
class vtkProcessModule;
class vtkPVData;
class vtkSMSourceProxy;
class vtkPVWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkCollection;
class vtkSMPartDisplay;
class vtkSMDisplay;
class vtkPVRenderModuleObserver;

class VTK_EXPORT vtkPVRenderModule : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkPVRenderModule,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the application right after construction.
  virtual void SetProcessModule(vtkProcessModule *pm);
  vtkProcessModule* GetProcessModule();

  // Description:
  // Compute the bounding box of all the visibile props
  // Used in ResetCamera() and ResetCameraClippingRange()
  virtual void ComputeVisiblePropBounds( double bounds[6] ) = 0; 
  
  // Description:
  // This is used for special plot displays.
  // It is a way to have the render module update displays
  // that are not part displays created by AddSource.
  // I do not expect that this will be a permenant method.
  // As we create more types of displays, I will have to
  // find a different way of managing them.
  virtual void AddDisplay(vtkSMDisplay* disp) = 0;
  virtual void RemoveDisplay(vtkSMDisplay* disp) = 0;
  
  // Description:
  // Renders using Still/FullRes or interactive/LODs
  virtual void StillRender();
  virtual void InteractiveRender();

  // Description:
  // Are we currently in interactive mode?
  //int IsInteractive() { return this->Interactive; }
  
  //BTX
  // Description:
  // It would be nice to keep renderers private.
  vtkRenderWindow *GetRenderWindow();
  vtkRenderer *GetRenderer();
  vtkRenderer *GetRenderer2D();
  //ETX
  vtkClientServerID GetRenderWindowID() { return this->RenderWindowID;}
  vtkClientServerID GetRendererID() { return this->RendererID;}
  vtkClientServerID GetRenderer2DID() { return this->Renderer2DID;}

  // Description:
  // Change the background color.
  void SetBackgroundColor(float r, float g, float b);
  virtual void SetBackgroundColor(float *c) {this->SetBackgroundColor(c[0],c[1],c[2]);}
  void GetBackgroundColor(float* rgb);

  // Description:
  // Callback for the triangle strips check button
  virtual void SetUseTriangleStrips(int val) = 0;
  
  // Description:
  // Callback for the immediate mode rendering check button
  virtual void SetUseImmediateMode(int val) = 0;
    
  // Description:
  // Change between parallel or perspective camera.
  // Since this is a camera manipulation, it does not have to be here.
  void SetUseParallelProjection(int val);

  // Description:
  // Controls whether the render module invokes abort check events.
  vtkSetMacro(RenderInterruptsEnabled,int);
  vtkGetMacro(RenderInterruptsEnabled,int);
  vtkBooleanMacro(RenderInterruptsEnabled,int);

  // Description:
  // Get the value of the z buffer at a position. 
  // This is necessary for picking the center of rotation.
  virtual float GetZBufferValue(int x, int y);

  // Description:
  // This is just a convenient spot to keep this flag.
  // Render view uses this to descide whether to recompute the
  // total memory size of visible geometry.  This is necessary
  // to decide between collection vs. distributed rendering.
  // When we create rendering modules, the render view will be
  // integrated with the vtkPVProcessModule and vtkPVParts and
  // this flag can be moved.
  vtkSetMacro(TotalVisibleMemorySizeValid, int);
  vtkGetMacro(TotalVisibleMemorySizeValid, int);

  // Description:
  // Update the cache of all visible part displays. For flip books.
  virtual void CacheUpdate(int idx, int total) = 0;

  // Description:
  // Calls InvalidateGeometry() on all part displays. For flip books.
  virtual void InvalidateAllGeometries() = 0;
  
  // Description:
  // This method is called when the 3D renderer renders so that the 2D window
  // can stay in sync
  void StartRenderEvent();

  // Description
  // Subclass can create their own vtkSMPartDisplay object by
  // implementing this method.
  // PartDisplays are specific to the render module.  
  // So far, others displays are not.
  virtual vtkSMPartDisplay* CreatePartDisplay() = 0;

  vtkClientServerID GetInteractorID() { return this->InteractorID;}
  //BTX
  void SetInteractorID(vtkClientServerID id) {this->InteractorID = id;}
  //ETX

protected:
  vtkPVRenderModule();
  ~vtkPVRenderModule();

  // This collection keeps a reference to all PartDisplays created
  // by this module.
  vtkCollection* Displays;

  // This is used before a render to make sure all visible sources
  // have been updated.
  virtual void UpdateAllDisplays() = 0;
 
  vtkPVProcessModule* ProcessModule;
  vtkRenderWindow*  RenderWindow;
  vtkRenderer*      Renderer;
  vtkRenderer*      Renderer2D;
  float             BackgroundColor[3];

  //int Interactive;
  int TotalVisibleMemorySizeValid;
  
  //Code for initializing InteractorID needs to be moved
  //from vtkPVWindow
  vtkClientServerID InteractorID;
  
  vtkClientServerID RenderWindowID;
  vtkClientServerID RendererID;
  vtkClientServerID Renderer2DID;
    
  double StillRenderTime;
  double InteractiveRenderTime;

  int DisableRenderingFlag;
  int RenderInterruptsEnabled;

  // Handle reset observers because we can do them more efficiently than
  // the render manager can.
  void InitializeObservers();
  unsigned long ResetCameraClippingRangeTag;

  vtkPVRenderModuleObserver* Observer;

private:
  vtkPVRenderModule(const vtkPVRenderModule&); // Not implemented
  void operator=(const vtkPVRenderModule&); // Not implemented
};


#endif
