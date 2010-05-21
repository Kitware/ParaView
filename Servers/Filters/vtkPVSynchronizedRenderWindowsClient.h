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
// .NAME vtkPVSynchronizedRenderWindowsClient
// .SECTION Description
// vtkPVSynchronizedRenderWindowsClient is the class used to synchronize render
// windows in client-server or client-render-server mode. This class is to be
// instantiated only on the client side or the [render-]server-root node.
// One doesn't have to be bogged down by
// creating the right class on the right processes. TODO: We will provide some
// factory or something to automatically create the right type.

#ifndef __vtkPVSynchronizedRenderWindowsClient_h
#define __vtkPVSynchronizedRenderWindowsClient_h

#include "vtkSynchronizedRenderWindows.h"

class VTK_EXPORT vtkPVSynchronizedRenderWindowsClient : public vtkSynchronizedRenderWindows
{
public:
  static vtkPVSynchronizedRenderWindowsClient* New();
  vtkTypeRevisionMacro(vtkPVSynchronizedRenderWindowsClient, vtkSynchronizedRenderWindows);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns a render window for use (possibly new).
  // FIXME: this API must be defined in a PV specific superclass.
  virtual vtkRenderWindow* NewRenderWindow();

  // Description:
  // Register/UnRegister a window.
  // FIXME: this API must be defined in a PV specific superclass.
  virtual void AddRenderWindow(unsigned int id, vtkRenderWindow*);
  virtual void RemoveRenderWindow(vtkRenderWindow*);
  virtual void RemoveRenderWindow(unsigned int id);

  // Description:
  // The views are not supposed to updated the render window position or size
  // directly. They should always go through this API to update the window sizes
  // and positions. This makes it possible to provide a consistent API
  // irrespective of the mode ParaView is running in.
  virtual void SetWindowSize(unsigned int id, int width, int height);
  virtual void SetWindowPosition(unsigned int id, int posx, int posy);
  virtual const int* GetWindowSize(unsigned int id);
  virtual const int* GetWindowPosition(unsigned int id);

  // Description:
  // Saves the information about all the windows known to this class and how
  // they are laid out. For this to work as expected, it is essential that the
  // client sets the WindowSize and WindowPosition correctly for all the render
  // windows using the API on this class.
  void SaveWindowLayout(vtkMultiProcessStream& stream);

//BTX
  enum
    {
    SYNC_MULTI_RENDER_WINDOW_TAG = 15002,
    };
protected:
  vtkPVSynchronizedRenderWindowsClient();
  ~vtkPVSynchronizedRenderWindowsClient();

  // These methods are called on all processes as a consequence of corresponding
  // events being called on the render window.
  virtual void HandleStartRender(vtkRenderWindow*);
  virtual void HandleEndRender(vtkRenderWindow*) {}
  virtual void HandleAbortRender(vtkRenderWindow*) {}

  virtual void MasterStartRender();
  virtual void SlaveStartRender();

private:
  vtkPVSynchronizedRenderWindowsClient(const vtkPVSynchronizedRenderWindowsClient&); // Not implemented
  void operator=(const vtkPVSynchronizedRenderWindowsClient&); // Not implemented

  virtual void SetRenderWindow(vtkRenderWindow* win)
    {
    if (win)
      {
      vtkErrorMacro("This method is not supported.");
      return;
      }
    return this->Superclass::SetRenderWindow(win);
    }
//ETX
};

#endif
