/*=========================================================================

  Program:   ParaView
  Module:    vtkPVView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVView
 * @brief   baseclass for all ParaView views.
 *
 * vtkPVView adds API to vtkView for ParaView specific views. Typically, one
 * writes a simple vtkView subclass for their custom view. Then one subclasses
 * vtkPVView to use their own vtkView subclass with added support for
 * parallel rendering, tile-displays and client-server. Even if the view is
 * client-only view, it needs to address these other configuration gracefully.
*/

#ifndef vtkPVView_h
#define vtkPVView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkView.h"
#include "vtkWeakPointer.h" // for vtkWeakPointer

class vtkBoundingBox;
class vtkInformation;
class vtkInformationObjectBaseKey;
class vtkInformationRequestKey;
class vtkInformationVector;
class vtkPVDataDeliveryManager;
class vtkPVDataRepresentation;
class vtkPVSession;
class vtkRenderWindow;
class vtkViewLayout;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVView : public vtkView
{
public:
  vtkTypeMacro(vtkPVView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static void SetEnableStreaming(bool);
  static bool GetEnableStreaming();

  //@{
  /**
   * Set the position on this view in the multiview configuration.
   * \note CallOnAllProcesses
   */
  virtual void SetPosition(int, int);
  vtkGetVector2Macro(Position, int);
  //@}

  //@{
  /**
   * Set the size of this view in the multiview configuration.
   * \note CallOnAllProcesses
   */
  virtual void SetSize(int, int);
  vtkGetVector2Macro(Size, int);
  //@}

  /**
   * Description:
   * Set the screen PPI.
   * \note CallOnAllProcesses
   */
  virtual void SetPPI(int);
  vtkGetMacro(PPI, int);

  /**
   * Triggers a high-resolution render.
   * \note CallOnAllProcesses
   */
  virtual void StillRender() = 0;

  /**
   * Triggers a interactive render. Based on the settings on the view, this may
   * result in a low-resolution rendering or a simplified geometry rendering.
   * \note CallOnAllProcesses
   */
  virtual void InteractiveRender() = 0;

  //@{
  /**
   * Get/Set the time this view is showing.
   *
   * \note CallOnAllProcesses
   */
  virtual void SetViewTime(double value);
  vtkGetMacro(ViewTime, double);
  //@}

  //@{
  /**
   * Get/Set the cache key. When caching is enabled, this key is used to
   * identify what geometry cache to use for the current render. It is passed on
   * to the representations in vtkPVView::Update(). The CacheKey is respected
   * only when UseCache is true.
   * \note CallOnAllProcesses
   */
  vtkSetMacro(CacheKey, double);
  vtkGetMacro(CacheKey, double);
  //@}

  //@{
  /**
   * Get/Set whether caching is enabled.
   * \note CallOnAllProcesses
   */
  vtkSetMacro(UseCache, bool);
  vtkGetMacro(UseCache, bool);
  //@}

  //@{
  /**
   * These methods are used to setup the view for capturing screen shots.
   */
  virtual void PrepareForScreenshot();
  virtual void CleanupAfterScreenshot();
  //@}

  /**
   * Key used to pass the vtkPVView pointer to the representation during any of
   * the view passes such as REQUEST_UPDATE(), REQUEST_UPDATE_LOD(),
   * REQUEST_RENDER(), etc.
   */
  static vtkInformationObjectBaseKey* VIEW();

  /**
   * This is a Update-Data pass. All representations are expected to update
   * their inputs and prepare geometries for rendering. All heavy work that has
   * to happen only when input-data changes can be done in this pass.
   * This is the first pass.
   */
  static vtkInformationRequestKey* REQUEST_UPDATE();

  /**
   * This is a Update-LOD-Data pass. All representations are expected to update
   * their lod-data, if any. This is assured to be called only after
   * REQUEST_UPDATE() pass.
   */
  static vtkInformationRequestKey* REQUEST_UPDATE_LOD();

  /**
   * This is a render pass. This is called for every render, hence
   * representations should not do any work that doesn't depend on things that
   * could change every render.
   */
  static vtkInformationRequestKey* REQUEST_RENDER();

  /**
   * Overridden to not call Update() directly on the input representations,
   * instead use ProcessViewRequest() for all vtkPVDataRepresentations.
   */
  void Update() override;

  /**
   * Returns true if the application is currently in tile display mode.
   */
  bool InTileDisplayMode();

  /**
   * Returns true if the application is currently in cave/immersive display
   * mode.
   */
  bool InCaveDisplayMode();

  /**
   * Returns true if the local process can support interaction. This will return
   * true only on the client node e.g. Qt client (or pvpython)
   * when connected to builtin or remote server. On server nodes this will return false.
   * CAVEAT: Currently this returns true on root node on batch and false on all
   * other nodes. In reality batch processes should not support interaction. Due
   * to a bug in vtkPVAxesWidget, if there's no interactor, the batch mode ends
   * up missing the orientation widget and hence rendering differently than
   * pvpython. To avoid that, this method currently returns true on the root
   * node in batch mode. This will however change in the future once
   * vtkPVAxesWidget has been cleaned up.
   */
  bool GetLocalProcessSupportsInteraction();

  /**
   * If this view needs a render window (not all views may use one),
   * this method can be used to get the render window associated with this view
   * on the current process. Note that this window may be shared with other
   * views depending on the process on which this is called and the
   * configuration ParaView is running under.
   */
  vtkRenderWindow* GetRenderWindow() { return this->RenderWindow; }

  //@{
  /**
   * Use this to indicate that the process should use
   * vtkGenericOpenGLRenderWindow rather than vtkRenderWindow when creating an
   * new render window.
   */
  static void SetUseGenericOpenGLRenderWindow(bool val);
  static bool GetUseGenericOpenGLRenderWindow();
  //@}

  //@{
  /**
   * When saving screenshots with tiling, these methods get called.
   * Not to be confused with tile scale and viewport setup on tile display.
   *
   * @sa vtkViewLayout::UpdateLayoutForTileDisplay
   */
  void SetTileScale(int x, int y);
  void SetTileViewport(double x0, double y0, double x1, double y1);
  //@}

  //@{
  /**
   * This is solely intended to simplify debugging and use for any other purpose
   * is vehemently discouraged.
   */
  virtual void SetLogName(const std::string& name) { this->LogName = name; }
  const std::string& GetLogName() const { return this->LogName; }
  //@}

  /**
   * vtkViewLayout calls this method to update the total viewport available for
   * this view. Generally, views can assume viewport is [0, 0, 1, 1] i.e. the
   * view has control over the complete window. However, in tile display mode,
   * this may not be the case, and hence a reduced viewport may be passed.
   * Generally, subclasses don't need to do much more than scale viewport for
   * each renderer they create within the provided viewport.
   *
   * Default implementation iterates over all renderers in the render window and
   * scales each assuming reach render's viewport is [0, 0, 1, 1]. Subclasses
   * may want to override to update renderers for which that is not the case.
   */
  virtual void ScaleRendererViewports(const double viewport[4]);

  /**
   * Provides access to the time when Update() was last called.
   */
  vtkMTimeType GetUpdateTimeStamp() { return this->UpdateTimeStamp; }

  //@{
  /**
   * Provides access to data delivery & cache manager for this view.
   */
  void SetDeliveryManager(vtkPVDataDeliveryManager*);
  vtkGetObjectMacro(DeliveryManager, vtkPVDataDeliveryManager);
  //@}

  static void SetPiece(vtkInformation* info, vtkPVDataRepresentation* repr, vtkDataObject* data,
    unsigned long trueSize = 0, int port = 0);
  static vtkDataObject* GetPiece(vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);
  static vtkDataObject* GetDeliveredPiece(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);

  static void SetPieceLOD(vtkInformation* info, vtkPVDataRepresentation* repr, vtkDataObject* data,
    unsigned long trueSize = 0, int port = 0);
  static vtkDataObject* GetPieceLOD(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);
  static vtkDataObject* GetDeliveredPieceLOD(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);

  /**
   * Called on all processes to request data-delivery for the list of
   * representations. Note this method has to be called on all processes or it
   * may lead to deadlock.
   */
  virtual void Deliver(int use_lod, unsigned int size, unsigned int* representation_ids);

  /**
   * Called in `vtkPVDataRepresentation::ProcessViewRequest` to check if the
   * representation already has cached data. If so, the representation may
   * choose to not update itself.
   */
  virtual bool IsCached(vtkPVDataRepresentation*);

  /**
   * Called by `vtkPVDataRepresentation` whenever
   * `vtkPVDataRepresentation::MarkModified` is called. Subclasses may use this
   * method to clear internal caches, if needed.
   */
  virtual void ClearCache(vtkPVDataRepresentation*);

protected:
  vtkPVView(bool create_render_window = true);
  ~vtkPVView() override;

  /**
   * Subclasses should use this method to create new render windows instead of
   * directly creating a new one.
   */
  vtkRenderWindow* NewRenderWindow();

  /**
   * Subclasses can use this method to set the render window created for this
   * view.
   */
  void SetRenderWindow(vtkRenderWindow*);

  /**
   * Reduce bounding box between all participating processes.
   */
  void AllReduce(const vtkBoundingBox& source, vtkBoundingBox& dest);

  /**
   * Reduce between all participating processes using the operation
   * (vtkCommunicator::StandardOperations) specified. Currently only
   * vtkCommunicator::MIN_OP, vtkCommunicator::MAX_OP, and
   * vtkCommunicator::SUM_OP are supported.
   */
  void AllReduce(
    const vtkTypeUInt64 source, vtkTypeUInt64& dest, int operation, bool skip_data_server = false);

  //@{
  /**
   * Overridden to assign IDs to each representation. This assumes that
   * representations will be added/removed in a consistent fashion across
   * processes even in multi-client modes. The only exception is
   * vtk3DWidgetRepresentation. However, since vtk3DWidgetRepresentation never
   * does any data-delivery, we don't assign IDs for these, nor affect the ID
   * uniquifier when a vtk3DWidgetRepresentation is added.
   */
  void AddRepresentationInternal(vtkDataRepresentation* rep) override;
  void RemoveRepresentationInternal(vtkDataRepresentation* rep) override;
  //@}

  //@{
  /**
   * These are passed as arguments to
   * vtkDataRepresentation::ProcessViewRequest(). This avoid repeated creation
   * and deletion of vtkInformation objects.
   */
  vtkInformation* RequestInformation;
  vtkInformationVector* ReplyInformationVector;
  //@}

  //@{
  /**
   * Subclasses can use this method to trigger a pass on all representations.
   * @returns the count for representations that processed the call and returned
   * success.
   */
  int CallProcessViewRequest(
    vtkInformationRequestKey* passType, vtkInformation* request, vtkInformationVector* reply);
  //@}

  vtkPVSession* GetSession();

  /**
   * Flag set to true between calls to `PrepareForScreenshot` and
   * `CleanupAfterScreenshot`.
   */
  vtkGetMacro(InCaptureScreenshot, bool);

  double ViewTime;
  double CacheKey;
  bool UseCache;

  int Size[2];
  int Position[2];
  int PPI;

  /**
   * Keeps track of the time when vtkPVRenderView::Update() was called.
   */
  vtkTimeStamp UpdateTimeStamp;

  static vtkPVDataDeliveryManager* GetDeliveryManager(vtkInformation* info);

private:
  vtkPVView(const vtkPVView&) = delete;
  void operator=(const vtkPVView&) = delete;

  /**
   * Called in Update() to sync HasTemporalPipeline flags between
   * representations on all processes.
   */
  void SynchronizeRepresentationTemporalPipelineStates();

  vtkRenderWindow* RenderWindow;
  bool ViewTimeValid;
  static bool EnableStreaming;
  vtkWeakPointer<vtkPVSession> Session;
  std::string LogName;

  static bool UseGenericOpenGLRenderWindow;

  bool InCaptureScreenshot;

  vtkPVDataDeliveryManager* DeliveryManager;
};

#endif
