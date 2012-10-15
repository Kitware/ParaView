/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderView - Render View for ParaView.
// .SECTION Description
// vtkRenderView equivalent that is specialized for ParaView. vtkRenderView
// handles polygonal rendering for ParaView in all the different modes of
// operation. vtkPVRenderView instance must be created on all involved
// processes. vtkPVRenderView uses the information about what process it has
// been created on to decide what part of the "rendering" happens on the
// process.
#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVView.h"
#include "vtkBoundingBox.h" // needed for iVar

class vtkAlgorithmOutput;
class vtkCamera;
class vtkCameraManipulator;
class vtkExtentTranslator;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInteractorStyleRubberBand3D;
class vtkInteractorStyleRubberBandZoom;
class vtkInteractorStyleDrawPolygon;
class vtkLight;
class vtkLightKit;
class vtkMatrix4x4;
class vtkProp;
class vtkPVAxesWidget;
class vtkPVCenterAxesActor;
class vtkPVDataDeliveryManager;
class vtkPVDataRepresentation;
class vtkPVGenericRenderWindowInteractor;
class vtkPVHardwareSelector;
class vtkPVInteractorStyle;
class vtkPVSynchronizedRenderer;
class vtkRenderer;
class vtkRenderViewBase;
class vtkRenderWindow;
class vtkTexture;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVRenderView : public vtkPVView
{
  //*****************************************************************
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum InteractionModes
    {
    INTERACTION_MODE_3D=0,
    INTERACTION_MODE_2D, // not implemented yet.
    INTERACTION_MODE_SELECTION,
    INTERACTION_MODE_ZOOM,
    INTERACTION_MODE_POLYGON
    };

  // Description:
  // Get/Set the interaction mode. Default is INTERACTION_MODE_3D. If
  // INTERACTION_MODE_SELECTION is
  // selected, then whenever the user drags and creates a selection region, this
  // class will fire a vtkCommand::SelectionChangedEvent event with the
  // selection region as the argument.
  // @CallOnAllProcessess - this must be called on all processes, however it will
  // have any effect only the driver processes i.e. the process with the
  // interactor.
  virtual void SetInteractionMode(int mode);
  vtkGetMacro(InteractionMode, int);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

  // Description:
  // Gets the non-composited renderer for this view. This is typically used for
  // labels, 2D annotations etc.
  // @CallOnAllProcessess
  vtkGetObjectMacro(NonCompositedRenderer, vtkRenderer);
  vtkRenderer* GetRenderer();

  // Description:
  // Get/Set the active camera. The active camera is set on both the composited
  // and non-composited renderer.
  vtkCamera* GetActiveCamera();
  virtual void SetActiveCamera(vtkCamera*);

  // Description:
  // Returns the render window.
  vtkRenderWindow* GetRenderWindow();

  // Description:
  // Returns the interactor. .
  vtkGetObjectMacro(Interactor, vtkPVGenericRenderWindowInteractor);

  // Description:
  // Returns the interactor style.
  vtkGetObjectMacro(InteractorStyle, vtkPVInteractorStyle);

  // Description:
  // Resets the active camera using collective prop-bounds.
  // @CallOnAllProcessess
  void ResetCamera();
  void ResetCamera(double bounds[6]);

  // Description:
  // Triggers a high-resolution render.
  // @CallOnAllProcessess
  virtual void StillRender();

  // Description:
  // Triggers a interactive render. Based on the settings on the view, this may
  // result in a low-resolution rendering or a simplified geometry rendering.
  // @CallOnAllProcessess
  virtual void InteractiveRender();

  // Description:
  // Get/Set the reduction-factor to use when for StillRender(). This is
  // typically set to 1, but in some cases with terrible connectivity or really
  // large displays, one may want to use a sub-sampled image even for
  // StillRender(). This is set it number of pixels to be sub-sampled by.
  // Note that image reduction factors have no effect when in built-in mode.
  // @CallOnAllProcessess
  vtkSetClampMacro(StillRenderImageReductionFactor, int, 1, 20);
  vtkGetMacro(StillRenderImageReductionFactor, int);

  // Description:
  // Get/Set the reduction-factor to use when for InteractiveRender().
  // This is set it number of pixels to be sub-sampled by.
  // Note that image reduction factors have no effect when in built-in mode.
  // @CallOnAllProcessess
  vtkSetClampMacro(InteractiveRenderImageReductionFactor, int, 1, 20);
  vtkGetMacro(InteractiveRenderImageReductionFactor, int);

  // Description:
  // Get/Set the data-size in megabytes above which remote-rendering should be
  // used, if possible.
  // @CallOnAllProcessess
  vtkSetMacro(RemoteRenderingThreshold, double);
  vtkGetMacro(RemoteRenderingThreshold, double);

  // Description:
  // Get/Set the data-size in megabytes above which LOD rendering should be
  // used, if possible.
  // @CallOnAllProcessess
  vtkSetMacro(LODRenderingThreshold, double);
  vtkGetMacro(LODRenderingThreshold, double);

  // Description:
  // Get/Set the LOD resolution. This affects the size of the grid used for
  // quadric clustering, for example. 1.0 implies maximum resolution while 0
  // implies minimum resolution.
  // @CallOnAllProcessess
  vtkSetClampMacro(LODResolution, double, 0.0, 1.0);
  vtkGetMacro(LODResolution, double);

  // Description:
  // When set to true, instead of using simplified geometry for LOD rendering,
  // uses outline, if possible. Note that not all representations support this
  // mode and hence one may still see non-outline data being rendering when this
  // flag is ON and LOD is being used.
  vtkSetMacro(UseOutlineForLODRendering, bool);
  vtkGetMacro(UseOutlineForLODRendering, bool);

  // Description:
  // Passes the compressor configuration to the client-server synchronizer, if
  // any. This affects the image compression used to relay images back to the
  // client.
  // See vtkPVClientServerSynchronizedRenderers::ConfigureCompressor() for
  // details.
  // @CallOnAllProcessess
  void ConfigureCompressor(const char* configuration);

  // Description:
  // Resets the clipping range. One does not need to call this directly ever. It
  // is called periodically by the vtkRenderer to reset the camera range.
  virtual void ResetCameraClippingRange();

  // Description:
  // Enable/Disable light kit.
  // @CallOnAllProcessess
  void SetUseLightKit(bool enable);
  vtkGetMacro(UseLightKit, bool);
  vtkBooleanMacro(UseLightKit, bool);

  // Description:
  void StreamingUpdate(const double view_planes[24]);
  void DeliverStreamedPieces(unsigned int size, unsigned int *representation_ids);

  // Description:
  // USE_LOD indicates if LOD is being used for the current render/update.
  static vtkInformationIntegerKey* USE_LOD();

  // Description:
  // Indicates the LOD resolution in REQUEST_UPDATE_LOD() pass.
  static vtkInformationDoubleKey* LOD_RESOLUTION();

  // Description:
  // Indicates the LOD must use outline if possible in REQUEST_UPDATE_LOD()
  // pass.
  static vtkInformationIntegerKey* USE_OUTLINE_FOR_LOD();

  // Description:
  // Representation can publish this key in their REQUEST_INFORMATION()
  // pass to indicate that the representation needs to disable
  // IceT's empty image optimization. This is typically only needed
  // if a painter will make use of MPI global collective communications.
  static vtkInformationIntegerKey* RENDER_EMPTY_IMAGES();

  // Description:
  // Representation can publish this key in their REQUEST_INFORMATION() pass to
  // indicate that the representation needs ordered compositing.
  static vtkInformationIntegerKey* NEED_ORDERED_COMPOSITING();

  // Description:
  // Key used to pass meta-data about the view frustum in REQUEST_STREAMING_UPDATE()
  // pass. The value is a double vector with exactly 24 values.
  static vtkInformationDoubleVectorKey* VIEW_PLANES();

  // Description:
  // Streaming pass request.
  static vtkInformationRequestKey* REQUEST_STREAMING_UPDATE();

  // Description:
  // Pass to relay the streamed "piece" to the representations.
  static vtkInformationRequestKey* REQUEST_PROCESS_STREAMED_PIECE();

  // Description:
  // Make a selection. This will result in setting up of this->LastSelection
  // which can be accessed using GetLastSelection().
  // @CallOnAllProcessess
  void SelectCells(int region[4]);
  void SelectCells(int region0, int region1, int region2, int region3)
    {
    int r[4] = {region0, region1, region2, region3};
    this->SelectCells(r);
    }
  void SelectPoints(int region[4]);
  void SelectPoints(int region0, int region1, int region2, int region3)
    {
    int r[4] = {region0, region1, region2, region3};
    this->SelectPoints(r);
    }
  void Select(int field_association, int region[4]);

  // Description:
  // Make a selection with a polygon. The polygon2DArray should contain
  // the polygon points in display units of (x, y) tuples, and arrayLen
  // is the total length of polygon2DArray.
  // This will result in setting up of this->LastSelection
  // which can be accessed using GetLastSelection().
  // @CallOnAllProcessess
  void SelectPolygonPoints(int* polygon2DArray, vtkIdType arrayLen);
  void SelectPolygonCells(int* polygon2DArray, vtkIdType arrayLen);
  void SelectPolygon(int field_association, int* polygon2DArray, vtkIdType arrayLen);

  // Description:
  // Provides access to the last selection.
  vtkGetObjectMacro(LastSelection, vtkSelection);

  // Description:
  // Set or get whether capture should be done as
  // StillRender or InteractiveRender when capturing screenshots.
  vtkSetMacro(UseInteractiveRenderingForSceenshots, bool);
  vtkBooleanMacro(UseInteractiveRenderingForSceenshots, bool);
  vtkGetMacro(UseInteractiveRenderingForSceenshots, bool);

  // Description:
  // Set or get whether offscreen rendering should be used during
  // CaptureWindow calls. On Apple machines, this flag has no effect.
  vtkSetMacro(UseOffscreenRenderingForScreenshots, bool);
  vtkBooleanMacro(UseOffscreenRenderingForScreenshots, bool);
  vtkGetMacro(UseOffscreenRenderingForScreenshots, bool);

  // Description:
  // Get/Set whether to use offscreen rendering for all rendering. This is
  // merely a suggestion. If --use-offscreen-rendering command line option is
  // specified, then setting this flag to 0 on that process has no effect.
  // Setting it to true, however, will ensure that even is
  // --use-offscreen-rendering is not specified, it will use offscreen
  // rendering.
  virtual void SetUseOffscreenRendering(bool);
  vtkBooleanMacro(UseOffscreenRendering, bool);
  vtkGetMacro(UseOffscreenRendering, bool);

  // Description:
  // Returns if remote-rendering is possible on the current group of processes.
  vtkGetMacro(RemoteRenderingAvailable, bool);
  void RemoteRenderingAvailableOff()
    { this->RemoteRenderingAvailable = false; }

  // Description:
  // Returns true if the most recent render used LOD.
  vtkGetMacro(UsedLODForLastRender, bool);

  // Description:
  // Invalidates cached selection. Called explicitly when view proxy thinks the
  // cache may have become obsolete.
  // @CallOnAllProcessess
  void InvalidateCachedSelection();

  // Description:
  // Returns the z-buffer value at the given location.
  // @CallOnClientOnly
  double GetZbufferDataAtPoint(int x, int y);

  // Description:
  // Convenience methods used by representations to pass represented data.
  static void SetPiece(vtkInformation* info,
    vtkPVDataRepresentation* repr, vtkDataObject* data);
  static vtkAlgorithmOutput* GetPieceProducer(vtkInformation* info,
    vtkPVDataRepresentation* repr);
  static void SetPieceLOD(vtkInformation* info,
    vtkPVDataRepresentation* repr, vtkDataObject* data);
  static vtkAlgorithmOutput* GetPieceProducerLOD(vtkInformation* info,
    vtkPVDataRepresentation* repr);
  static void MarkAsRedistributable(
    vtkInformation* info, vtkPVDataRepresentation* repr, bool value=true);
  static void SetGeometryBounds(vtkInformation* info,
    double bounds[6], vtkMatrix4x4* transform = NULL);
  static void SetStreamable(
    vtkInformation* info, vtkPVDataRepresentation* repr, bool streamable);
  static void SetNextStreamedPiece(
    vtkInformation* info, vtkPVDataRepresentation* repr, vtkDataObject* piece);
  static vtkDataObject* GetCurrentStreamedPiece(
    vtkInformation* info, vtkPVDataRepresentation* repr);

  // Description:
  // Requests the view to deliver the pieces produced by the \c repr to all
  // processes after a gather to the root node to merge the datasets generated
  // by each process.
  static void SetDeliverToAllProcesses(
    vtkInformation* info, vtkPVDataRepresentation* repr, bool clone);

  // Description:
  // Requests the view to deliver the data to the client always. This is
  // essential for representation that render in the non-composited views e.g.
  // the text-source representation. If SetDeliverToAllProcesses() is true, this
  // is redundant. \c gather_before_delivery can be used to indicate if the data
  // on the server-nodes must be gathered to the root node before shipping to
  // the client. If \c gather_before_delivery is false, only the data from the
  // root node will be sent to the client without any parallel communication.
  static void SetDeliverToClientAndRenderingProcesses(
    vtkInformation* info, vtkPVDataRepresentation* repr,
    bool deliver_to_client, bool gather_before_delivery);

  // Description:
  // Pass the structured-meta-data for determining rendering order for ordered
  // compositing.
  static void SetOrderedCompositingInformation(
    vtkInformation* info, vtkPVDataRepresentation* repr,
    vtkExtentTranslator* translator,
    const int whole_extents[6], const double origin[3], const double spacing[3]);

  // Description:
  // Representations that support hardware (render-buffer based) selection,
  // should register the prop that they use for selection rendering. They can do
  // that in the vtkPVDataRepresentation::AddToView() implementation.
  void RegisterPropForHardwareSelection(
    vtkPVDataRepresentation* repr, vtkProp* prop);
  void UnRegisterPropForHardwareSelection(
    vtkPVDataRepresentation* repr, vtkProp* prop);

  // Description:
  // Turn on/off the default light in the 3D renderer.
  void SetLightSwitch(bool enable);
  bool GetLightSwitch();
  vtkBooleanMacro(LightSwitch, bool);

  //*****************************************************************
  // Forwarded to orientation axes widget.
  virtual void SetOrientationAxesInteractivity(bool);
  virtual void SetOrientationAxesVisibility(bool);
  void SetOrientationAxesLabelColor(double r, double g, double b);
  void SetOrientationAxesOutlineColor(double r, double g, double b);

  //*****************************************************************
  // Forwarded to center axes.
  virtual void SetCenterAxesVisibility(bool);

  //*****************************************************************
  // Forward to vtkPVGenericRenderWindowInteractor.
  void SetCenterOfRotation(double x, double y, double z);
  void SetNonInteractiveRenderDelay(double seconds);

  //*****************************************************************
  // Forward to vtkLightKit.
  void SetKeyLightWarmth(double val);
  void SetKeyLightIntensity(double val);
  void SetKeyLightElevation(double val);
  void SetKeyLightAzimuth(double val);
  void SetFillLightWarmth(double val);
  void SetKeyToFillRatio(double val);
  void SetFillLightElevation(double val);
  void SetFillLightAzimuth(double val);
  void SetBackLightWarmth(double val);
  void SetKeyToBackRatio(double val);
  void SetBackLightElevation(double val);
  void SetBackLightAzimuth(double val);
  void SetHeadLightWarmth(double val);
  void SetKeyToHeadRatio(double val);
  void SetMaintainLuminance(int val);

  //*****************************************************************
  // Forward to 3D renderer.
  virtual void SetUseDepthPeeling(int val);
  virtual void SetMaximumNumberOfPeels(int val);
  virtual void SetBackground(double r, double g, double b);
  virtual void SetBackground2(double r, double g, double b);
  virtual void SetBackgroundTexture(vtkTexture* val);
  virtual void SetGradientBackground(int val);
  virtual void SetTexturedBackground(int val);

  //*****************************************************************
  // Forward to vtkLight.
  void SetAmbientColor(double r, double g, double b);
  void SetSpecularColor(double r, double g, double b);
  void SetDiffuseColor(double r, double g, double b);
  void SetIntensity(double val);
  void SetLightType(int val);

  //*****************************************************************
  // Forward to vtkRenderWindow.
  void SetStereoCapableWindow(int val);
  void SetStereoRender(int val);
  void SetStereoType(int val);
  void SetMultiSamples(int val);
  void SetAlphaBitPlanes(int val);
  void SetStencilCapable(int val);

  //*****************************************************************
  // Forwarded to vtkPVInteractorStyle if present on local processes.
  virtual void Add2DManipulator(vtkCameraManipulator* val);
  virtual void RemoveAll2DManipulators();
  virtual void Add3DManipulator(vtkCameraManipulator* val);
  virtual void RemoveAll3DManipulators();

  // Description:
  // Overridden to synchronize information among processes whenever data
  // changes. The vtkSMViewProxy ensures that this method is called only when
  // something has changed on the view-proxy or one of its representations or
  // their inputs. Hence it's okay to do some extra inter-process communication
  // here.
  virtual void Update();

  // Description:
  // Asks representations to update their LOD geometries.
  virtual void UpdateLOD();

  // Description:
  // Returns whether the view will use LOD rendering for the next
  // InteractiveRender() call based on the geometry sizes determined by the most
  // recent call to Update().
  vtkGetMacro(UseLODForInteractiveRender, bool);

  // Description:
  // Returns whether the view will use distributed rendering for the next
  // StillRender() call based on the geometry sizes determined by the most
  // recent call to Update().
  vtkGetMacro(UseDistributedRenderingForStillRender, bool);

  // Description:
  // Returns whether the view will use distributed rendering for the next
  // InteractiveRender() call based on the geometry sizes determined by the most
  // recent calls to Update() and UpdateLOD().
  vtkGetMacro(UseDistributedRenderingForInteractiveRender, bool);

  // Description:
  // Returns the processes (vtkPVSession::ServerFlags) that are to be involved
  // in the next StillRender() call based on the decisions made during the most
  // recent Update().
  vtkGetMacro(StillRenderProcesses, vtkTypeUInt32);

  // Description:
  // Returns the processes (vtkPVSession::ServerFlags) that are to be involved
  // in the next InteractiveRender() call based on the decisions made during the most
  // recent Update() and UpdateLOD().
  vtkGetMacro(InteractiveRenderProcesses, vtkTypeUInt32);

  // Description:
  // Returns the data distribution mode to use.
  int GetDataDistributionMode(bool use_remote_rendering);

  // Description:
  // Provides access to the geometry storage for this view.
  vtkPVDataDeliveryManager* GetDeliveryManager();

  // Description:
  // Called on all processes to request data-delivery for the list of
  // representations. Note this method has to be called on all processes or it
  // may lead to deadlock.
  void Deliver(int use_lod,
    unsigned int size, unsigned int *representation_ids);

  // Description:
  // Returns true when ordered compositing is needed on the current group of
  // processes. Note that unlike most other functions, this may return different
  // values on different processes e.g.
  // \li always false on client and dataserver
  // \li true on pvserver or renderserver if opacity < 1 or volume present, else
  //     false
  bool GetUseOrderedCompositing();

  // Description:
  // Returns true when the compositor should not use the empty
  // images optimization.
  bool GetRenderEmptyImages();

  // Description:
  // Provides access to the time when Update() was last called.
  unsigned long GetUpdateTimeStamp()
    { return this->UpdateTimeStamp; }

  // Description:
  // Copy internal fields that are used for rendering decision such as
  // remote/local rendering, composite and so on. This method was introduced
  // for the quad view so internal views could use the decision that were made
  // in the main view.
  void CopyViewUpdateOptions(vtkPVRenderView* otherView);
//BTX
protected:
  vtkPVRenderView();
  ~vtkPVRenderView();

  // Description:
  // Overridden to assign IDs to each representation. This assumes that
  // representations will be added/removed in a consistent fashion across
  // processes even in multi-client modes. The only exception is
  // vtk3DWidgetRepresentation. However, since vtk3DWidgetRepresentation never
  // does any data-delivery, we don't assign IDs for these, nor affect the ID
  // uniquifier when a vtk3DWidgetRepresentation is added.
  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

  // Description:
  // These methods are used to setup the view for capturing screen shots.
  // In batch mode, since the server-side has just 1 render window, we need to
  // make sure that the right interactor is activated, otherwise, we end up
  // capturing images from the wrong view.
  virtual void PrepareForScreenshot();

  // Description:
  // Actual render method.
  virtual void Render(bool interactive, bool skip_rendering);

  // Description:
  // Returns true if distributed rendering should be used based on the geometry
  // size.
  bool ShouldUseDistributedRendering(double geometry_size);

  // Description:
  // Returns true if LOD rendering should be used based on the geometry size.
  bool ShouldUseLODRendering(double geometry);

  // Description:
  // Synchronizes bounds information on all nodes.
  // @CallOnAllProcessess
  void SynchronizeGeometryBounds();

  // Description:
  // Set the last selection object.
  void SetLastSelection(vtkSelection*);

  // Description:
  // UpdateCenterAxes().
  // Updates CenterAxes's scale and position.
  void UpdateCenterAxes();

  // Description
  // Returns true if the local process is doing to do actual render or
  // displaying an image in a viewport.
  bool GetLocalProcessDoesRendering(bool using_distributed_rendering);

  // Description:
  // In multi-clients mode, ensures that all processes are in the same "state"
  // as far as the view is concerned. Returns false if that's not the case.
  bool TestCollaborationCounter();

  // Description:
  // Synchronizes remote-rendering related parameters for collaborative
  // rendering in multi-clients mode.
  void SynchronizeForCollaboration();

  // Description:
  // SynchronizationCounter is used in multi-clients mode to ensure that the
  // views on two different clients are in the same state as the server side.
  vtkGetMacro(SynchronizationCounter, unsigned int);

  // Description:
  // Returns true is currently generating a selection.
  vtkGetMacro(MakingSelection, bool);

  // Description:
  // Prepare for selection.
  // Returns false if it is currently generating a selection.
  bool PrepareSelect(int fieldAssociation);

  // Description:
  // Post process after selection.
  void PostSelect(vtkSelection* sel);

  vtkLight* Light;
  vtkLightKit* LightKit;
  vtkRenderViewBase* RenderView;
  vtkRenderer* NonCompositedRenderer;
  vtkPVSynchronizedRenderer* SynchronizedRenderers;
  vtkPVGenericRenderWindowInteractor* Interactor;
  vtkInteractorStyleRubberBand3D* RubberBandStyle;
  vtkInteractorStyleRubberBandZoom* RubberBandZoom;
  vtkInteractorStyleDrawPolygon* PolygonStyle;
  vtkPVCenterAxesActor* CenterAxes;
  vtkPVAxesWidget* OrientationWidget;
  vtkPVHardwareSelector* Selector;
  vtkSelection* LastSelection;

  int StillRenderImageReductionFactor;
  int InteractiveRenderImageReductionFactor;
  int InteractionMode;

  // 2D and 3D interactor style
  vtkPVInteractorStyle* TwoDInteractorStyle;
  vtkPVInteractorStyle* ThreeDInteractorStyle;

  // Active interactor style either [TwoDInteractorStyle, ThreeDInteractorStyle]
  vtkPVInteractorStyle* InteractorStyle;

  // Used in collaboration mode to ensure that views are in the same state
  // (as far as representations added/removed goes) before rendering.
  unsigned int SynchronizationCounter;

  // In mega-bytes.
  double RemoteRenderingThreshold;
  double LODRenderingThreshold;
  vtkBoundingBox GeometryBounds;

  bool UseOffscreenRendering;
  bool UseOffscreenRenderingForScreenshots;
  bool UseInteractiveRenderingForSceenshots;
  bool NeedsOrderedCompositing;
  bool RenderEmptyImages;

  double LODResolution;
  bool UseLightKit;

  bool UsedLODForLastRender;
  bool UseLODForInteractiveRender;
  bool UseOutlineForLODRendering;
  bool UseDistributedRenderingForStillRender;
  bool UseDistributedRenderingForInteractiveRender;

  vtkTypeUInt32 StillRenderProcesses;
  vtkTypeUInt32 InteractiveRenderProcesses;

  // Description:
  // Keeps track of the time when vtkPVRenderView::Update() was called.
  vtkTimeStamp UpdateTimeStamp;

  // Description:
  // Keeps track of the time when the priority-queue for streaming was
  // generated.
  vtkTimeStamp PriorityQueueBuildTimeStamp;
private:
  vtkPVRenderView(const vtkPVRenderView&); // Not implemented
  void operator=(const vtkPVRenderView&); // Not implemented

  bool MakingSelection;
  void OnSelectionChangedEvent();
  void OnPolygonSelectionEvent();
  void FinishSelection(vtkSelection*);

  // This flag is set to false when not all processes cannot render e.g. cannot
  // open the DISPLAY etc.
  bool RemoteRenderingAvailable;

  int PreviousParallelProjectionStatus;

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
