/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSimpleRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSimpleRenderModule - Mangages rendering and displaying data.
// .SECTION Description
// This calss is not used unless it is specified by the 
// --render-module=SimpleRenderModule command line argument.

#ifndef __vtkPVSimpleRenderModule_h
#define __vtkPVSimpleRenderModule_h

#include "vtkPVRenderModule.h"

#include "vtkClientServerID.h" // Needed for RenderWindowID and RendererID

class vtkMultiProcessController;
class vtkPVProcessModule;
class vtkPVData;
class vtkSMSourceProxy;
class vtkPVWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkCollection;
class vtkPVPartDisplay;
class vtkPVDisplay;
class vtkPVSimpleRenderModuleObserver;

class VTK_EXPORT vtkPVSimpleRenderModule : public vtkPVRenderModule
{
public:
  static vtkPVSimpleRenderModule* New();
  vtkTypeRevisionMacro(vtkPVSimpleRenderModule,vtkPVRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is used for special plot displays.
  // It is a way to have the render module update displays
  // that are not part displays created by AddSource.
  // I do not expect that this will be a permenant method.
  // As we create more types of displays, I will have to
  // find a different way of managing them.
  virtual void AddDisplay(vtkPVDisplay* disp);
  virtual void RemoveDisplay(vtkPVDisplay* disp);
  
  // Description:
  // Compute the bounding box of all the visibile props
  // Used in ResetCamera() and ResetCameraClippingRange()
  void ComputeVisiblePropBounds( double bounds[6] ); 
    
  // Description:
  // This method is executed in all processes.
  void AddSource(vtkSMSourceProxy *s);
  void RemoveSource(vtkSMSourceProxy *s);

  // Description:
  // Callback for the triangle strips check button
  void SetUseTriangleStrips(int val);
  
  // Description:
  // Callback for the immediate mode rendering check button
  void SetUseImmediateMode(int val);
    
  // Description:
  // Change between parallel or perspective camera.
  // Since this is a camera manipulation, it does not have to be here.
  void SetUseParallelProjection(int val);

  // Description:
  // Update the cache of all visible part displays. For flip books.
  void CacheUpdate(int idx, int total);

  // Description:
  // Calls InvalidateGeometry() on all part displays. For flip books.
  void InvalidateAllGeometries();
  
protected:
  vtkPVSimpleRenderModule();
  ~vtkPVSimpleRenderModule();

  // Subclass can create their own vtkPVPartDisplay object by
  // implementing this method.
  virtual vtkPVPartDisplay* CreatePartDisplay();

  // This is used before a render to make sure all visible sources
  // have been updated.
  virtual void UpdateAllDisplays();
 
  vtkPVSimpleRenderModule(const vtkPVSimpleRenderModule&); // Not implemented
  void operator=(const vtkPVSimpleRenderModule&); // Not implemented
};


#endif
