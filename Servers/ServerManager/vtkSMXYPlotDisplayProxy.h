/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYPlotDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXYPlotDisplayProxy - Proxy for XY Plot Display.
// .SECTION Desription
// This is the display proxy for XY Plot. It can be added to a render module
// proxy to be rendered.
// .SECTION See Also
// vtkSMXYPlotActorProxy

#ifndef __vtkSMXYPlotDisplayProxy_h
#define __vtkSMXYPlotDisplayProxy_h

#include "vtkSMDisplayProxy.h"

class vtkSMXYPlotDisplayProxyObserver;
class vtkXYPlotWidget;
class vtkSMRenderModuleProxy;

class vtkPolyData;

class VTK_EXPORT vtkSMXYPlotDisplayProxy : public vtkSMDisplayProxy
{
public:
  static vtkSMXYPlotDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMXYPlotDisplayProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);


  // Description:
  // I have this funny looking AddInput instead of a simple
  // SetInput as I want to have an InputProperty for the input (rather than
  // a proxy property).
  void AddInput(vtkSMSourceProxy* input, const char*, int , int );

  //BTX
  // Description:
  // The Probe needs access to this to fill in the UI point values.
  // Only needed when probing one point only.
  // TODO: I have to find a means to get rid of this!!
  vtkPolyData *GetCollectedData();
  //ETX
  
  // Description:
  // Sets the visibility of the XYPlotActor. Also enables/disables
  // the XYPlotWidget.
  void SetVisibility(int visible);

  // Description:
  // This method updates the piece that has been assigned to this process.
  // Leads to a call to ForceUpdate on UpdateSuppressorProxy iff
  // GeometryIsValid==0;
  virtual void Update();
  
  // Description:
  // Marks for Update.
  virtual void InvalidateGeometry();

protected:
  vtkSMXYPlotDisplayProxy();
  ~vtkSMXYPlotDisplayProxy();
  // Description:
  // Calls MarkConsumersAsModified() on all consumers. Sub-classes
  // should add their functionality and call this.
  // Overridden to clean up cached geometry as well. 
  virtual void MarkConsumersAsModified();
  
  virtual void CreateVTKObjects(int numObjects);

  void SetupPipeline();
  void SetupDefaults();
  void SetupWidget();

  // This is not reference counted. 
  vtkSMRenderModuleProxy* RenderModuleProxy;
  vtkSMProxy* XYPlotActorProxy;
  vtkSMProxy* PropertyProxy;
  vtkSMProxy* UpdateSuppressorProxy;
  vtkSMProxy* CollectProxy;

  vtkXYPlotWidget* XYPlotWidget; // This is the widget on the client side.
  int Visibility;
  int GeometryIsValid; // Flag indicating is Update must call ForceUpdate.
  //BTX
  friend class vtkSMXYPlotDisplayProxyObserver;
  vtkSMXYPlotDisplayProxyObserver* Observer;
  void ExecuteEvent(vtkObject* obj, unsigned long event, void* calldata);
  //ETX
private:
  vtkSMXYPlotDisplayProxy(const vtkSMXYPlotDisplayProxy&); // Not implemented.
  void operator=(const vtkSMXYPlotDisplayProxy&); // Not implemented.
};


#endif
