/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTiledDisplayManager.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTiledDisplayManager - Duplicates data on each node.
// .SECTION Description
// vtkTiledDisplayManager operates in multiple processes.  Each process
// (except 0) is responsible for rendering to one tile of a large display.
// Process 0 is reserved for interaction and directing the view of the 
// large display.

// .SECTION see also
// vtkMultiProcessController vtkRenderWindow vtkCompositeManager.

#ifndef __vtkTiledDisplayManager_h
#define __vtkTiledDisplayManager_h

#include "vtkObject.h"

class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkMultiProcessController;
class vtkRenderer;

class VTK_EXPORT vtkTiledDisplayManager : public vtkObject
{
public:
  static vtkTiledDisplayManager *New();
  vtkTypeRevisionMacro(vtkTiledDisplayManager,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the RenderWindow to use for compositing.
  // We add a start and end observer to the window.
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // Callbacks that initialize and finish the compositing.
  void StartInteractor();
  void ExitInteractor();
  virtual void StartRender();
  virtual void EndRender();
  virtual void SatelliteStartRender();
  virtual void SatelliteEndRender();
  void RenderRMI();
  
  // Description:
  // If the user wants to handle the event loop, then they must call this
  // method to initialize the RMIs.
  virtual void InitializeRMIs();
  
  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Methods that are not used at the moment.
  virtual void SetRenderView(vtkObject*);

  // Description:
  // Set/Get dimensions (in number of displays) of the
  // display array.  There has to be NxM+1 processes.
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);

  // Description:
  // This ivar is to help control the rescaling of the tiled display.
  // If the tiled display has an overall aspect ratio different that
  // each of its displays, then you may need to customize the display size.
  // Large scales make objects look bigger.
  vtkSetMacro(Scale, float);
  vtkGetMacro(Scale, float);

//BTX
  enum Tags {
    RENDER_RMI_TAG=12721,
    WIN_INFO_TAG=22134,
    REN_INFO_TAG=22135
  };
//ETX

protected:
  vtkTiledDisplayManager();
  ~vtkTiledDisplayManager();
  
  vtkRenderWindow* RenderWindow;
  vtkRenderWindowInteractor* RenderWindowInteractor;
  vtkMultiProcessController* Controller;

  unsigned long StartInteractorTag;
  unsigned long EndInteractorTag;
  unsigned long StartTag;
  unsigned long EndTag;
  
  // Convenience method used internally. It set up the start observer
  // and allows the render window's interactor to be set before or after
  // the compositer's render window (not exactly true).
  void SetRenderWindowInteractor(vtkRenderWindowInteractor *iren);

  vtkObject *RenderView;

  int TileDimensions[2];

  float Scale;
  
private:
  vtkTiledDisplayManager(const vtkTiledDisplayManager&); // Not implemented
  void operator=(const vtkTiledDisplayManager&); // Not implemented
};

#endif
