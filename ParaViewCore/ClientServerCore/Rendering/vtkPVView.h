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

class vtkInformation;
class vtkInformationObjectBaseKey;
class vtkInformationRequestKey;
class vtkInformationVector;
class vtkPVSynchronizedRenderWindows;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVView : public vtkView
{
public:
  vtkTypeMacro(vtkPVView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static void SetEnableStreaming(bool);
  static bool GetEnableStreaming();

  enum
  {
    ViewTimeChangedEvent = 9000
  };

  /**
   * Initialize the view with an identifier. Unless noted otherwise, this method
   * must be called before calling any other methods on this class.
   * \note CallOnAllProcesses
   */
  virtual void Initialize(unsigned int id);

  //@{
  /**
   * Set the position on this view in the multiview configuration.
   * This can be called only after Initialize().
   * \note CallOnAllProcesses
   */
  virtual void SetPosition(int, int);
  vtkGetVector2Macro(Position, int);
  //@}

  //@{
  /**
   * Set the size of this view in the multiview configuration.
   * This can be called only after Initialize().
   * \note CallOnAllProcesses
   */
  virtual void SetSize(int, int);
  vtkGetVector2Macro(Size, int);
  //@}

  /**
   * Description:
   * Set the screen PPI.
   * This can be called only after Initialize().
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
   * This encapsulates a whole lot of logic for
   * communication between processes. Given the ton of information this class
   * keeps, it can easily aid vtkViews to synchronize information such as bounds/
   * data-size among all processes efficiently. This can be achieved by using
   * these methods.
   * Note that these methods should be called on all processes at the same time
   * otherwise we will have deadlocks.
   * We may make this API generic in future, for now this works.
   */
  bool SynchronizeBounds(double bounds[6]);
  bool SynchronizeSize(double& size);
  bool SynchronizeSize(unsigned int& size);
  //@}

  //@{
  /**
   * Get/Set the time this view is showing. Whenever time is changed, this fires
   * a ViewTimeChangedEvent event.
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
  virtual void Update() VTK_OVERRIDE;

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
   * pvpython. To avoid that, this method curretly returns true on the root
   * node in batch mode. This will however change in the future once
   * vtkPVAxesWidget has been cleaned up.
   */
  bool GetLocalProcessSupportsInteraction();

  vtkGetMacro(Identifier, unsigned int);

protected:
  vtkPVView();
  ~vtkPVView();

  /**
   * Overridden to check that the representation has View setup properly. Older
   * implementations of vtkPVDataRepresentations::AddToView() subclasses didn't
   * call the superclass implementations. We check that that's not the case and
   * warn.
   */
  virtual void AddRepresentationInternal(vtkDataRepresentation* rep) VTK_OVERRIDE;

  // vtkPVSynchronizedRenderWindows is used to ensure that this view participates
  // in tile-display configurations. Even if your view subclass a simple
  // Qt-based client-side view that does not render anything on the
  // tile-display, it needs to be "registered with the
  // vtkPVSynchronizedRenderWindows so that the layout on the tile-displays for
  // other views shows up correctly. Ideally you'd want to paste some image on
  // the tile-display, maybe just a capture of the image rendering on the
  // client.
  // If your view needs a vtkRenderWindow, don't directly create it, always get
  // using vtkPVSynchronizedRenderWindows::NewRenderWindow().
  vtkPVSynchronizedRenderWindows* SynchronizedWindows;

  /**
   * Every view gets a unique identifier that it uses to register itself with
   * the SynchronizedWindows. This is set in Initialize().
   */
  unsigned int Identifier;

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
   */
  void CallProcessViewRequest(
    vtkInformationRequestKey* passType, vtkInformation* request, vtkInformationVector* reply);
  double ViewTime;
  //@}

  double CacheKey;
  bool UseCache;

  int Size[2];
  int Position[2];
  int PPI;

private:
  vtkPVView(const vtkPVView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVView&) VTK_DELETE_FUNCTION;

  class vtkInternals;

  bool ViewTimeValid;
  bool LastRenderOneViewAtATime;

  static bool EnableStreaming;
};

#endif
