/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSynchronizedRenderWindows
// .SECTION Description
// vtkPVSynchronizedRenderWindows is used by ParaView to synchronize render
// windows among all the processes for client-server and/or parallel rendering.
// It uses vtkSynchronizedRenderWindows internally. To use, one simply
// instantiates vtkPVSynchronizedRenderWindows on all processes and setting it
// up with the same id and a render window local to that processes. It also
// supports multi-view configurations.
// .SECTION See Also
// vtkSynchronizedRenderWindows, vtkSynchronizedRenderers

#ifndef __vtkPVSynchronizedRenderWindows_h
#define __vtkPVSynchronizedRenderWindows_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"

class VTK_EXPORT vtkPVSynchronizedRenderWindows : public vtkObject
{
public:
  static vtkPVSynchronizedRenderWindows* New();
  vtkTypeRevisionMacro(vtkPVSynchronizedRenderWindows, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // It's acceptable to have multiple instances on vtkPVSynchronizedRenderWindows
  // on each processes to synchronize different render windows. In that case
  // there's no way to each of the vtkPVSynchronizedRenderWindows instance to know
  // how they correspond across processes. To enable that identification, a
  // vtkPVSynchronizedRenderWindows can be assigned a unique id. All
  // vtkPVSynchronizedRenderWindows across different processes that have the same
  // id are "linked" together for synchronization. It's critical that the id is
  // set before any rendering happens.
  void SetIdentifier(unsigned int id);
  vtkGetMacro(Identifier, unsigned int);

  // Description:
  // Get/Set the render window.
  // FIXME: vtkSMRenderViewProxy will need logic to ensure that there's only 1
  // render window on the server (or batch) processes. Only the client processes
  // can create multiple render windows for each view in a multi-view
  // configuration.
  void SetRenderWindow(vtkRenderWindow*);
  vtkGetObjectMacro(vtkRenderWindow*);

  // Description:
  // Add/remove renderers to be synchronized.
  void AddSynchronizedRenderer(vtkRenderer*);
  void RemoveAllSynchronizedRenderers();

  // Description:
  // Add/remove renderers that are not synchronized across processes, but are
  // still rendered separately on all the processes (these typically are the
  // non-composited renderers).
  void AddRenderer(vtkRenderer*);
  void RemoveAllRenderers();

  // Description:
  // Enable/Disable remote rendering.
  vtkSetMacro(RemoteRendering, bool);
  vtkGetMacro(RemoteRendering, bool);
  vtkBooleanMacro(RemoteRendering, bool);

  // Description
  // Turns on/off render event propagation.  When on (the default) and
  // ParallelRendering is on, process 0 will send an RMI call to all remote
  // processes to perform a synchronized render.  When off, render must be
  // manually called on each process.
  vtkSetMacro(RenderEventPropagation, bool);
  vtkGetMacro(RenderEventPropagation, bool);
  vtkBooleanMacro(RenderEventPropagation, bool);

  // Description:
  // Get/Set the image reduction factor.
  vtkSetMacro(ImageReductionFactor, int);
  vtkGetMacro(ImageReductionFactor, int);

  // TODO: This will have other ivars related to compression types and settings.

  // This class will automatically figure out if we are running in tile-display
  // mode or not, so no need to worry about remote display or tile dimensions
  // etc.

//BTX
protected:
  vtkPVSynchronizedRenderWindows();
  ~vtkPVSynchronizedRenderWindows();

  vtkSmartPointer<vtkSynchronizedRenderWindows> SyncWindowsP;
  vtkSmartPointer<vtkSynchronizedRenderers> SyncRenderersP;

  vtkSmartPointer<vtkSynchronizedRenderWindows> SyncWindowsCS;
  vtkSmartPointer<vtkSynchronizedRenderers> SyncRenderersCS;

  unsigned int Identifier;
  vtkRenderWindow* RenderWindow;

  bool RemoteRendering;
  bool RenderEventPropagation;
  int ImageReductionFactor;

private:
  vtkPVSynchronizedRenderWindows(const vtkPVSynchronizedRenderWindows&); // Not implemented
  void operator=(const vtkPVSynchronizedRenderWindows&); // Not implemented
//ETX
};

#endif
