/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBatchCompositeManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBatchCompositeManager - Simple version for batch files.
// .SECTION Description
// vtkPVBatchCompositeManager does none of the synchronization.
// It just takes buffers and composites them to node 0
// .SECTION see also
// vtkMultiProcessController vtkRenderWindow.

#ifndef __vtkPVBatchCompositeManager_h
#define __vtkPVBatchCompositeManager_h

#include "vtkImageSource.h"

class vtkTimerLog;
class vtkFloatArray;
class vtkDataArray;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkMultiProcessController;
class vtkRenderer;
class vtkCompositer;
class vtkUnsignedCharArray;

class VTK_EXPORT vtkPVBatchCompositeManager : public vtkImageSource
{
public:
  static vtkPVBatchCompositeManager *New();
  vtkTypeRevisionMacro(vtkPVBatchCompositeManager,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the RenderWindow to use for compositing.
  // We add a start and end observer to the window.
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // This method sets the piece and number of pieces for each
  // actor with a polydata mapper. The user can skip this
  // and partition manually if wanted (task parallelism).
  void InitializePieces();

  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // All processes must call composite explicitly.
  // Do this before you update the source.
  void Composite();

  // Description:
  // This methods allows the user to select different compositing algorithms.
  // A vtkCompressCompositer is created as a default value.
  virtual void SetCompositer(vtkCompositer*);
  vtkGetObjectMacro(Compositer, vtkCompositer);

protected:
  vtkPVBatchCompositeManager();
  ~vtkPVBatchCompositeManager();
  
  void ExecuteInformation();
  void Execute();
  void Execute(vtkImageData*) {this->Execute();}
  
  vtkRenderWindow* RenderWindow;
  vtkMultiProcessController* Controller;
  
  // This object does the parallel communication for compositing.
  vtkCompositer *Compositer;
  int NumberOfProcesses;

  // Arrays for compositing.
  vtkDataArray *PData;
  vtkFloatArray *ZData;
  vtkDataArray *LocalPData;
  vtkFloatArray *LocalZData;
  // Store value to se if size hase changed.
  int RendererSize[2];

  // Makes sure buffers are large enough.
  void SetRendererSize(int x, int y);

private:
  vtkPVBatchCompositeManager(const vtkPVBatchCompositeManager&); // Not implemented
  void operator=(const vtkPVBatchCompositeManager&); // Not implemented
};

#endif
