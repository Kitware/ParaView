/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRenderViewProxy - implementation for View that includes
// render window and renderers.
// .SECTION Description
// vtkSMRenderViewProxy is a 3D view consisting for a render window and two
// renderers: 1 for 3D geometry and 1 for overlayed 2D geometry.

#ifndef __vtkSMRenderViewProxy_h
#define __vtkSMRenderViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMViewProxy.h"

class vtkCamera;
class vtkCollection;
class vtkIntArray;
class vtkPVGenericRenderWindowInteractor;
class vtkRenderer;
class vtkRenderWindow;
class vtkSMDataDeliveryManager;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMRenderViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMRenderViewProxy* New();
  vtkTypeMacro(vtkSMRenderViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes a new selection source proxy.
  bool SelectSurfaceCells(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectSurfacePoints(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectFrustumCells(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectFrustumPoints(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectPolygonPoints(vtkIntArray* polygon,
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);
  bool SelectPolygonCells(vtkIntArray* polygon,
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections=false);

  // Description:
  // Convenience method to pick a location. Internally uses SelectSurfaceCells
  // to locate the picked object. In future, we can make this faster.
  vtkSMRepresentationProxy* Pick(int x, int y);

  // Description:
  // Convenience method to pick a block in a multi-block data set. Will return
  // the selected representation. Furthermore, if it is a multi-block data set
  // the flat index of the selected block will be returned in flatIndex.
  vtkSMRepresentationProxy* PickBlock(int x, int y, unsigned int &flatIndex);

  // Description:
  // Given a location is display coordinates (pixels), tries to compute and
  // return the world location on a surface, if possible. Returns true if the
  // conversion was successful, else returns false.
  bool ConvertDisplayToPointOnSurface(
    const int display_position[2], double world_position[3]);

  // Description:
  // Checks if color depth is sufficient to support selection.
  // If not, will return 0 and any calls to SelectVisibleCells will
  // quietly return an empty selection.
  virtual bool IsSelectionAvailable();

  // Description:
  // For backwards compatibility in Python scripts.
  void ResetCamera()
    { this->InvokeCommand("ResetCamera"); }
  void ResetCamera(double bounds[6]);

  // Description:
  // Convenience method for zooming to a representation.
  void ZoomTo(vtkSMProxy* representation);

  // Description:
  // Similar to IsSelectionAvailable(), however, on failure returns the
  // error message otherwise 0.
  virtual const char* IsSelectVisibleCellsAvailable();
  virtual const char* IsSelectVisiblePointsAvailable();

  // Description:
  // Returns the interactor.
  vtkPVGenericRenderWindowInteractor* GetInteractor();

  // Description:
  // Returns the client-side renderer (composited or 3D).
  vtkRenderer* GetRenderer();

  // Description:
  // Returns the client-side camera object.
  vtkCamera* GetActiveCamera();

  // Description:
  // This method calls UpdateInformation on the Camera Proxy
  // and sets the Camera properties according to the info
  // properties.
  // This approach is a bit lame. We need to ensure that camera properties are
  // always/automatically synchronized. Camera properties cannot be treated same
  // way as other properties.
  void SynchronizeCameraProperties();

  // Description:
  // Returns true if the most recent render indeed employed low-res rendering.
  virtual bool LastRenderWasInteractive();

  // Description:
  // Returns the Z-buffer value at the given location in this view.
  double GetZBufferValue(int x, int y);

  // Description:
  // Called vtkPVView::Update on the server-side. Overridden to update the state
  // of NeedsUpdateLOD flag.
  virtual void Update();

  // Description:
  // Called to render a streaming pass. Returns true if the view "streamed" some
  // geometry.
  bool StreamingUpdate(bool render_if_needed);

  // Description:
  // Overridden to check through the various representations that this view can
  // create.
  virtual const char* GetRepresentationType(
    vtkSMSourceProxy* producer, int outputPort);

  // Description:
  // Returns the render window used by this view.
  virtual vtkRenderWindow* GetRenderWindow();

//BTX
protected:
  vtkSMRenderViewProxy();
  ~vtkSMRenderViewProxy();


  // Description:
  // Calls UpdateLOD() on the vtkPVRenderView.
  void UpdateLOD();

  // Description:
  // Overridden to ensure that we clean up the selection cache on the server
  // side.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

  // Description:
  // Subclasses should override this method to do the actual image capture.
  virtual vtkImageData* CaptureWindowInternal(int magnification);
  virtual void CaptureWindowInternalRender();

  bool SelectFrustumInternal(int region[4],
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections,
    int fieldAssociation);
  bool SelectPolygonInternal(vtkIntArray* polygon,
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    bool multiple_selections,
    const char* method);

  virtual vtkTypeUInt32 PreRender(bool interactive);
  virtual void PostRender(bool interactive);

  // Description:
  // Fetches the LastSelection from the data-server and then converts it to a
  // selection source proxy and returns that.
  bool FetchLastSelection(bool multiple_selections,
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources);

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void CreateVTKObjects();

  bool IsSelectionCached;
  void ClearSelectionCache(bool force = false);

  // Internal fields for the observer mechanism that is used to invalidate
  // the cache of selection when the current user became master
  unsigned long NewMasterObserverId;
  void NewMasterCallback(vtkObject* src, unsigned long event, void* data);

  vtkSMDataDeliveryManager* DeliveryManager;
  bool NeedsUpdateLOD;

private:
  vtkSMRenderViewProxy(const vtkSMRenderViewProxy&); // Not implemented
  void operator=(const vtkSMRenderViewProxy&); // Not implemented
//ETX
};

#endif
