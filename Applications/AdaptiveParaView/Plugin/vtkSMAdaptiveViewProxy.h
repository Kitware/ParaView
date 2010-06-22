/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAdaptiveViewProxy - implementation for RenderView that updates
// its render window and renderers in a streamed fashion.
// .SECTION Description
// vtkSMtreamingViewProxy is a 3D view updates its 3D geometry in a streamed
// fashion to keep the memory footprint small while displaying big data sets.

#ifndef __vtkSMAdaptiveViewProxy_h
#define __vtkSMAdaptiveViewProxy_h

#include "vtkSMViewProxy.h"

class vtkUnsignedCharArray;
class vtkSMRenderViewProxy;
class vtkTimerLog;
class vtkSMAdaptiveViewHelper;
class vtkSMAdaptiveOptionsProxy;
class vtkImageData;

class VTK_EXPORT vtkSMAdaptiveViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMAdaptiveViewProxy* New();
  vtkTypeMacro(vtkSMAdaptiveViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSMRenderViewProxy *GetRootView();

  // Description:
  // Checks if color depth is sufficient to support selection.
  // If not, will return 0 and any calls to SelectVisibleCells will 
  // quietly return an empty selection.
  virtual bool IsSelectionAvailable() { return false;}

  void AddRepresentation(vtkSMRepresentationProxy* repr);
  void RemoveRepresentation(vtkSMRepresentationProxy* repr);
  void RemoveAllRepresentations();
  void StillRender();
  void InteractiveRender();

  // Description:
  // Updates the data pipelines for all visible representations.
  virtual void UpdateAllRepresentations();

  // Description:
  // Performs actual rendering. Called for both still render and interactive 
  // render.
  virtual void PerformRender();

  // Description:
  // Create a default representation for the given output port of source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int opport);

  // Description:
  // Generally each view type is different class of view eg. bar char view, line
  // plot view etc. However in some cases a different view types are indeed the
  // same class of view the only different being that each one of them works in
  // a different configuration eg. "RenderView" in builin mode, 
  // "IceTDesktopRenderView" in remote render mode etc. This method is used to
  // determine what type of view needs to be created for the given class. When
  // user requests the creation of a view class, the application can call this
  // method on a prototype instantaiated for the requested class and the
  // determine the actual xmlname for the view to create.
  // Overridden to choose the correct type of render view.
  virtual const char* GetSuggestedViewType(vtkIdType connectionID);

  // Description:
  // Get the streaming helper proxy.
  static vtkSMAdaptiveOptionsProxy* GetAdaptiveOptionsProxy();

  // Description:
  // Generate a screenshot from the render window.
  // The file format is determined from the extension of the file to write.
  int WriteImage(const char* filename, const char* writerName)
    { return this->WriteImage(filename, writerName, 1); }
  int WriteImage(const char* filename, const char* writerName, int magnification);
  int WriteImage(const char* filename, int magnification);

  // Description:
  // Returns an image data that contains a "screenshot" of the window.
  // It is the responsibility of the caller to delete the image data.
  virtual vtkImageData* CaptureWindow(int magnification);


  // Description:
  // Views keep this false as long as it needs to do more render passes.
  virtual int GetDisplayDone();

  // Description:
  // Tells it to stop what it is doing to let user interact.
  virtual void Interrupt();

  // Description:
//BTX
  enum {MANUAL, AUTOMATIC_REFINE, AUTOMATIC_COARSEN};
  enum {COARSEN, STAY, REFINE};
//ETX
  virtual void SetRefinementMode(int i);
  vtkGetMacro(RefinementMode, int);
  virtual void Refine();
  virtual void Coarsen();

//BTX
protected:
  vtkSMAdaptiveViewProxy();
  ~vtkSMAdaptiveViewProxy();

  // Description:
  // Called at the start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();
  virtual void EndCreateVTKObjects();

  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested.
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(int dataType);

  // Description:
  // For progressive rendering and streaming, this lets you render into the 
  // back buffer, see the results of each pass in the front buffer, and keep 
  // the back buffer intact for the next pass.
  void CopyBackBufferToFrontBuffer();

  // Description:
  // Make incremental rendering happen.
  virtual void PrepareRenderPass();
  virtual void FinalizeRenderPass();

  // Description:
  // Called to check if view point has been changed since last call.
  // If so, multipass rendering has to start over.
  // Also used to obtain view frustum and camera location for view prioritization
  bool CameraChanged();


  vtkTimerLog *RenderTimer;
  bool IsSerial;
  vtkSMAdaptiveViewHelper *RenderViewHelper;
  vtkUnsignedCharArray *PixelArray;

  class vtkInternals;
  vtkInternals* Internals;

  int DisplayDone;
  int WendDone;

  int RefinementMode; 
  int AdvanceCommand;

  int Pass;

  bool ProgrammaticRestart;
private:

  vtkSMAdaptiveOptionsProxy* options;

  vtkSMAdaptiveViewProxy(const vtkSMAdaptiveViewProxy&); // Not implemented.
  void operator=(const vtkSMAdaptiveViewProxy&); // Not implemented.

//ETX
};


#endif

