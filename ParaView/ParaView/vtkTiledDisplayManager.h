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
class vtkTiledDisplaySchedule;
class vtkFloatArray;
class vtkUnsignedCharArray;

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
  // Set the total number of processes.
  // Will be replaced by MPI num procs eventually.
  vtkSetMacro(NumberOfProcesses,int);
  vtkGetMacro(NumberOfProcesses,int);


//BTX
  enum Tags {
    RENDER_RMI_TAG=12721,
    WIN_INFO_TAG=22134,
    REN_INFO_TAG=22135
  };
//ETX



  //============================================================
  // New compositing methods.

  // Description:
  // This create all of the buffer objects as well as create the 
  // composite schedule.  Set TileDimensions, TileProcesses and
  // NumberOfProcesses before calling this method.
  void InitializeBuffers();

  // Description:
  // Tile processes are probably going to be 0, 1, 2 ...
  //void SetTileProcesses(int idx, int id);


  // This keeps processes 0 out of the rendering composite group.
  vtkSetMacro(ZeroEmpty, int);
  vtkGetMacro(ZeroEmpty, int);
  vtkBooleanMacro(ZeroEmpty, int);

  void InitializeSchedule();
  int ShuffleLevel(int level, int numTiles, 
                   vtkTiledDisplaySchedule** tileSchedules);




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

  // Any processes can display the tiles.
  // There can be more processes than tiles.
  int TileDimensions[2];
  //vtkIntArray* TileProcesses;
  int NumberOfProcesses;
  // We have a z and color buffer for each tile.
  int NumberOfTiles;
  vtkFloatArray** TileZData;
  vtkUnsignedCharArray** TilePData;

  // We have two spare set of buffers for receiving and compositing.
  vtkFloatArray* ZData;
  vtkUnsignedCharArray*  PData;  
  vtkFloatArray* ZData2;
  vtkUnsignedCharArray*  PData2;  

  void SetPDataSize(int x, int y);
  int PDataSize[2];

  vtkTiledDisplaySchedule* Schedule;
  int ZeroEmpty;

  // On: Composite, Off, assume geometry copied to all tile procs.
  int CompositeFlag;

  void Composite();

private:
  vtkTiledDisplayManager(const vtkTiledDisplayManager&); // Not implemented
  void operator=(const vtkTiledDisplayManager&); // Not implemented
};

#endif
