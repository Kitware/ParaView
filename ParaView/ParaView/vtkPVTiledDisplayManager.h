/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTiledDisplayManager.h
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
// .NAME vtkPVTiledDIsplayManager - Duplicates data on each node.
// .SECTION Description
// vtkPVTiledDIsplayManager operates in multiple processes.  Each process
// (except 0) is responsible for rendering to one tile of a large display.
// Process 0 is reserved for interaction and directing the view of the 
// large display.

// .SECTION see also
// vtkMultiProcessController vtkRenderWindow vtkCompositeManager.

#ifndef __vtkPVTiledDisplayManager_h
#define __vtkPVTiledDisplayManager_h

#include "vtkObject.h"

class vtkRenderWindow;
class vtkMultiProcessController;
class vtkRenderer;
class vtkPVTiledDisplaySchedule;
class vtkPVCompositeUtilities;
class vtkFloatArray;
class vtkUnsignedCharArray;

class VTK_EXPORT vtkPVTiledDisplayManager : public vtkObject
{
public:
  static vtkPVTiledDisplayManager *New();
  vtkTypeRevisionMacro(vtkPVTiledDisplayManager,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the RenderWindow to use for compositing.
  // We add a start and end observer to the window.
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // Callbacks that initialize and finish the compositing.
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

//BTX
  enum Tags {
    RENDER_RMI_TAG=12721,
    WIN_INFO_TAG=22134,
    REN_INFO_TAG=22135
  };
//ETX

  //============================================================
  // New compositing methods.

  void InitializeSchedule();
  int ShuffleLevel(int level, int numTiles, 
                   vtkPVTiledDisplaySchedule** tileSchedules);

  // Description:
  // This flag is ignored (Not used at the moment).
  // It will switch between compositing the tiled displays
  // and distributing the polydata to each tile to render.
  vtkSetMacro(UseCompositing, int);
  vtkGetMacro(UseCompositing, int);
  vtkBooleanMacro(UseCompositing, int);

  // Description:
  // This value is only used for interactive rendering.
  // Reduction factor = 1 means normal (full sized) rendering
  // and compositing.  When ReductionFactor > 1, a small window
  // is rendered (subsampled) and composited.
  vtkSetMacro(LODReductionFactor, int);
  vtkGetMacro(LODReductionFactor, int);

  // Description:
  // Just used for debugging.
  vtkGetObjectMacro(CompositeUtilities,vtkPVCompositeUtilities);

protected:
  vtkPVTiledDisplayManager();
  ~vtkPVTiledDisplayManager();
  
  vtkRenderWindow* RenderWindow;
  vtkMultiProcessController* Controller;

  // For managing buffers.
  vtkPVCompositeUtilities* CompositeUtilities;

  int ImageReductionFactor;
  int LODReductionFactor;

  unsigned long StartTag;
  unsigned long EndTag;
  
  vtkObject *RenderView;

  // Any processes can display the tiles.
  // There can be more processes than tiles.
  int TileDimensions[2];
  int NumberOfProcesses;

  vtkPVTiledDisplaySchedule* Schedule;
  int ZeroEmpty;

  // On: Composite, Off, assume geometry copied to all tile procs.
  int UseCompositing;

  void Composite();

private:
  vtkPVTiledDisplayManager(const vtkPVTiledDisplayManager&); // Not implemented
  void operator=(const vtkPVTiledDisplayManager&); // Not implemented
};

#endif
