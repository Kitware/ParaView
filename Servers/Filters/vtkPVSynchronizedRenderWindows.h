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
// .NAME vtkPVSynchronizedRenderWindows - synchronizes render-windows among
// processes in ParaView configurations.
// .SECTION Description
// vtkPVSynchronizedRenderWindows is the class used to synchronize render
// windows in ParaView. This class can be instantiated on all processes in all
// modes, it automatically discovers the configuration and adapts its behavior
// accordingly. The role of this class is to set up the render windows on all
// processes and then synchronize renders. It does not manage compositing or
// image delivery. All it does is synchronize render windows and their layouts
// among processes.
//
// If the application is managing calling of vtkRenderWindow::Render() on all
// processes, then one should disable RenderEventPropagation flag.

#ifndef __vtkPVSynchronizedRenderWindows_h
#define __vtkPVSynchronizedRenderWindows_h

#include "vtkObject.h"
#include "vtkMultiProcessController.h" // for vtkRMIFunctionType

class vtkRenderWindow;
class vtkRenderer;
class vtkMultiProcessController;
class vtkMultiProcessStream;
class vtkSelection;

class VTK_EXPORT vtkPVSynchronizedRenderWindows : public vtkObject
{
public:
  static vtkPVSynchronizedRenderWindows* New();
  vtkTypeMacro(vtkPVSynchronizedRenderWindows, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns a render window for use (possibly new).
  virtual vtkRenderWindow* NewRenderWindow();

  // Description:
  // Register/UnRegister a window.
  virtual void AddRenderWindow(unsigned int id, vtkRenderWindow*);
  virtual void RemoveRenderWindow(unsigned int id);
  vtkRenderWindow* GetRenderWindow(unsigned int id);

  // Description:
  // Register/UnRegister the renderers. One can add multiple renderers for the
  // same id. The id must be same as that specified when adding the
  // corresponding render window.
  virtual void AddRenderer(unsigned int id, vtkRenderer*);
  virtual void RemoveAllRenderers(unsigned int id);

  // Description:
  // The views are not supposed to updated the render window position or size
  // directly. They should always go through this API to update the window sizes
  // and positions. This makes it possible to provide a consistent API
  // irrespective of the mode ParaView is running in.
  // These methods only need to be called on the "driver" node. (No harm in
  // calling on all nodes). By driver node, we mean the CLIENT in
  // client-server mode and the root node in batch mode.
  virtual void SetWindowSize(unsigned int id, int width, int height);
  virtual void SetWindowPosition(unsigned int id, int posx, int posy);
  virtual const int* GetWindowSize(unsigned int id);
  virtual const int* GetWindowPosition(unsigned int id);

  // Description:
  // Enable/Disable parallel rendering.
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);

  // Description:
  // Enable/Disable propagation of the render event. This is typically true,
  // unless the application is managing calling Render() on all processes
  // involved.
  vtkSetMacro(RenderEventPropagation, bool);
  vtkGetMacro(RenderEventPropagation, bool);
  vtkBooleanMacro(RenderEventPropagation, bool);

  // Description:
  // This method returns true if the local process is the 'driver' process. In
  // client-server configurations, client is the driver. In batch
  // configurations, root-node is the driver. In built-in mode, this always
  // returns true.
  bool GetLocalProcessIsDriver();

  // Description:
  // vtkPVSynchronizedRenderWindows encapsulates a whole lot of logic for
  // communication between processes. Given the ton of information this class
  // keeps, it can easily aid vtkViews to synchronize information such as bounds/
  // data-size among all processes efficiently. This can be achieved by using
  // these methods.
  // Note that these methods should be called on all processes at the same time
  // otherwise we will have deadlocks.
  // We may make this API generic in future, for now this works.
  bool SynchronizeBounds(double bounds[6]);
  bool SynchronizeSize(unsigned long &size);
  bool BroadcastToDataServer(vtkSelection* selection);

  // Description:
  // Convenience method to trigger an RMI call from the client/root node.
  void TriggerRMI(vtkMultiProcessStream& stream, int tag);
  unsigned long AddRMICallback(vtkRMIFunctionType, void* localArg, int tag);
  bool RemoveRMICallback(unsigned long id);

//BTX
  enum
    {
    SYNC_MULTI_RENDER_WINDOW_TAG = 15002,
    };

  // Internal-callback-method
  void Render(unsigned int);

  // Description:
  vtkGetObjectMacro(ParallelController, vtkMultiProcessController);
  vtkGetObjectMacro(ClientServerController, vtkMultiProcessController);
  vtkGetObjectMacro(ClientDataServerController, vtkMultiProcessController);

  // Description:
  // By default, this class uses the same render window for all views on the
  // server processes and then arranges the renderers by adjusting their
  // viewports. However, this does not work well when doing image capture with
  // magnification. In those cases, you can force this class to simply render
  // the active view as the sole view in the window on the server side by
  // setting this flag to true.
  vtkSetMacro(RenderOneViewAtATime, bool);
  vtkGetMacro(RenderOneViewAtATime, bool);
  vtkBooleanMacro(RenderOneViewAtATime, bool);

  // Description:
  // Called before starting render. This is needed in batch mode since all views
  // share the same render window.
  void BeginRender(unsigned int id);
protected:
  vtkPVSynchronizedRenderWindows();
  ~vtkPVSynchronizedRenderWindows();

  // Description:
  // Set/Get the controller used for communication among parallel processes.
  void SetParallelController(vtkMultiProcessController*);

  // Description:
  // Set/Get the controller used for client-server communication.
  void SetClientServerController(vtkMultiProcessController*);

  // Description:
  // Set/Get the controller used for client-data-server communication.
  void SetClientDataServerController(vtkMultiProcessController*);

  // Description:
  // Saves the information about all the windows known to this class and how
  // they are laid out. For this to work as expected, it is essential that the
  // client sets the WindowSize and WindowPosition correctly for all the render
  // windows using the API on this class. It also saves some information about
  // the active render window.
  void SaveWindowAndLayout(vtkRenderWindow*, vtkMultiProcessStream& stream);
  void LoadWindowAndLayout(vtkRenderWindow*, vtkMultiProcessStream& stream);

  // Description:
  // Using the meta-data saved about the render-windows and their positions and
  // sizes, this updates the renderers/window-sizes etc. This have different
  // response on different processes types.
  void UpdateWindowLayout();

  // These methods are called on all processes as a consequence of corresponding
  // events being called on the render window.
  virtual void HandleStartRender(vtkRenderWindow*);
  virtual void HandleEndRender(vtkRenderWindow*);
  virtual void HandleAbortRender(vtkRenderWindow*) {}

  virtual void ClientStartRender(vtkRenderWindow*);
  virtual void RootStartRender(vtkRenderWindow*);
  virtual void SatelliteStartRender(vtkRenderWindow*);

  // Description:
  // This method should only be called on RENDER_SERVER or BATCH processes.
  // Returns true if in tile display mode and fills up tile_dims with the tile
  // dimensions.
  bool GetTileDisplayParameters(int tile_dims[2]);

  // Description:
  // Shrinks gaps between views, rather grows the views to reduce gaps. Only
  // used in tile-display mode to avoid gaps on the server side.
  void ShinkGaps();

  enum ModeEnum
    {
    INVALID,
    BUILTIN,
    CLIENT,
    RENDER_SERVER,
    DATA_SERVER,
    BATCH
    };


  ModeEnum Mode;
  vtkMultiProcessController* ParallelController;
  vtkMultiProcessController* ClientServerController;
  vtkMultiProcessController* ClientDataServerController;
  unsigned long ClientServerRMITag;
  unsigned long ParallelRMITag;
  bool Enabled;
  bool RenderEventPropagation;
  bool RenderOneViewAtATime;

private:
  vtkPVSynchronizedRenderWindows(const vtkPVSynchronizedRenderWindows&); // Not implemented
  void operator=(const vtkPVSynchronizedRenderWindows&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

  class vtkObserver;
  vtkObserver* Observer;

//ETX
};

#endif
