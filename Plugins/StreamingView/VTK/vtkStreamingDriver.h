/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingDriver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamingDriver - orchestrates progression of streamed rendering
// .SECTION Description
// vtkStreamingDriver automates the process of streamed rendering. It tells
// vtk's rendering classes to setup for streamed rendering, watches the events
// they fire to start, restart and stop streaming, and cycles through passes
// until completion.
// It controls the pipeline with the help of vtkStreamingHarness classes.
// It delegates the task of deciding what piece to show next to subclasses.

#ifndef __vtkStreamingDriver_h
#define __vtkStreamingDriver_h

#include "vtkObject.h"

class vtkCallbackCommand;
class vtkCollection;
class vtkRenderer;
class vtkRenderWindow;
class vtkStreamingHarness;

class VTK_EXPORT vtkStreamingDriver : public vtkObject
{
public:
  vtkTypeMacro(vtkStreamingDriver,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Assign what window to automate streaming in.
  void SetRenderWindow(vtkRenderWindow *);
  vtkRenderWindow* GetRenderWindow();

  // Description:
  // Assign what renderer to automate streaming in.
  void SetRenderer(vtkRenderer *);
  vtkRenderer* GetRenderer();

  // Description:
  // Control over the list of things that this renders in streaming fashion.
  void AddHarness(vtkStreamingHarness *);
  void RemoveHarness(vtkStreamingHarness *);
  void RemoveAllHarnesses();
  vtkCollection *GetHarnesses();

  // Description:
  // Assign a function that this driver can call to schedule eventual
  // render calls. This allows automatic streaming to work as part of
  // a GUI event loop so that it can be interruptable.
  void AssignRenderLaterFunction(void (*function)(void *), void *arg);

  // Description:
  // Sometimes, as in ParaView, window events happen too late.
  // Before assigning a render window, set these to true (the default is false)
  // to take over and drive the progression yourself.
  vtkSetMacro(ManualStart, bool);
  vtkSetMacro(ManualFinish, bool);

  //Description:
  //Controls if the display is updated when fully drawn (0), or
  //when each new piece is drawn (1). Fully drawn is the default.
  vtkSetMacro(DisplayFrequency, int);
  vtkGetMacro(DisplayFrequency, int);

  //Description:
  //Sets the cache size of all of the piece cache filters for all
  //harnesses shown in in the window. Default is 32.
  void SetCacheSize(int);
  vtkGetMacro(CacheSize, int);

  //Description:
  //A command to halt streaming as soon as possible.
  virtual void StopStreaming() = 0;

  // Description:
  // For internal use, window events call back here.
  virtual void StartRenderEvent() = 0;
  virtual void EndRenderEvent() = 0;

protected:
  vtkStreamingDriver();
  ~vtkStreamingDriver();

  // Description:
  // Driver calls this to ask for a render. If a render later function is
  // assigned, that is used, otherwise render is called directly.
  void RenderEventually();

  // Description:
  // Determines if camera has changed AND sets up for view prioritization
  // This is common to prioritized subclasses, so I've placed it here
  virtual bool HasCameraMoved();

  // Description:
  // Determines view priority of an object relative to camera frustum
  // Must call Is Restart prior to calling this.
  // This is common to prioritized subclasses, so I've placed it here
  double CalculateViewPriority(double *bbox);

  // Description:
  // For diagnostic mode rendering, copies renwins back buffer to its front
  void CopyBackBufferToFront();

  bool ManualStart;
  bool ManualFinish;

  int CacheSize;
  int DisplayFrequency;

private:
  vtkStreamingDriver(const vtkStreamingDriver&);  // Not implemented.
  void operator=(const vtkStreamingDriver&);  // Not implemented.

  class Internals;
  Internals *Internal;
};

#endif
