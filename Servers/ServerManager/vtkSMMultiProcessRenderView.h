/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiProcessRenderView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMultiProcessRenderView
// .SECTION Description
// vtkSMMultiProcessRenderView is abstract superclass for all render views that
// support multi-process rendering such as client-server, or mpi, or both.
// This defines the API and come common code such as composting decision.

#ifndef __vtkSMMultiProcessRenderView_h
#define __vtkSMMultiProcessRenderView_h

#include "vtkSMRenderViewProxy.h"

class VTK_EXPORT vtkSMMultiProcessRenderView : public vtkSMRenderViewProxy
{
public:
  vtkTypeRevisionMacro(vtkSMMultiProcessRenderView, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the threshold used to determine if the compositing should be used
  // for rendering. In client-server views, this typically implies remote
  // render with compositing.
  // NOTE: In client-server configurations, this implies remote-render
  // threshold.
  vtkSetClampMacro(RemoteRenderThreshold, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(RemoteRenderThreshold, double);

  // Description:
  // Returns if it is possible to perform remote rendering on the given set up.
  // It may not be possible to remote render due to server issues such as
  // inaccessible display.
  vtkGetMacro(RemoteRenderAvailable, bool);

  // Description:
  // Overridden to ensure that remote rendering is available since remote
  // rendering is required for visible cell selection.
  virtual const char* IsSelectVisibleCellsAvailable();

  // Description:
  // Overridden to force remote rendering before selecting.
  virtual vtkSelection *SelectVisibleCells(unsigned int x0, unsigned int y0, 
    unsigned int x1, unsigned int y1, int ofPoints);

//BTX
protected:
  vtkSMMultiProcessRenderView();
  ~vtkSMMultiProcessRenderView();

  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested.
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(
    int dataType);

  // Description:
  // Called at the end of CreateVTKObjects().
  // Overridden to check if remote rendering is possible on the current setup.
  virtual void EndCreateVTKObjects();

  // Description:
  // Method called before Still Render is called.
  // Used to perform some every-still-render-setup actions.
  virtual void BeginStillRender();

  // Description:
  // Method called before Interactive Render.
  // Used to perform some every-interactive-render-setup actions.
  virtual void BeginInteractiveRender();

  // Description:
  // Indicates if we should render using compositing.
  // Returns true if compositing should be used, otherwise false.
  // Flag stillRender is set when this decision is to be made during StillRender
  // else it's 0 (for InteractiveRender);
  virtual bool GetCompositingDecision(
    unsigned long totalMemory, int stillRender);

  // Description:
  // Pass compositing decision to the parallel render manager and the view
  // helper.
  virtual void SetUseCompositing(bool usecompositing);
  
  // Render window shared on the server side.
  vtkClientServerID SharedRenderWindowID;

  double RemoteRenderThreshold;
  bool LastCompositingDecision;
  bool RemoteRenderAvailable;

private:
  vtkSMMultiProcessRenderView(const vtkSMMultiProcessRenderView&); // Not implemented
  void operator=(const vtkSMMultiProcessRenderView&); // Not implemented
//ETX
};

#endif

