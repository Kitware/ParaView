/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMViewProxy
 * @brief   Superclass for all view proxies.
 *
 * vtkSMViewProxy is a superclass for all view proxies. A view proxy
 * abstracts the logic to take one or more representation proxies and show then
 * in some viewport such as vtkRenderWindow.
 * This class may directly be used as the view proxy for views that do all the
 * rendering work at the GUI level. The VTKObject corresponding to this class
 * has to be a vtkView subclass.
 * @par Events:
 * \li vtkCommand::StartEvent(callData: int:0) -- start of StillRender().
 * \li vtkCommand::EndEvent(callData: int:0) -- end of StillRender().
 * \li vtkCommand::StartEvent(callData: int:1) -- start of InteractiveRender().
 * \li vtkCommand::EndEvent(callData: int:1) -- end of InteractiveRender().
*/

#ifndef vtkSMViewProxy_h
#define vtkSMViewProxy_h

#include "vtkCommand.h"             // needed for vtkCommand.
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkImageData;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSMDataDeliveryManagerProxy;
class vtkSMRepresentationProxy;
class vtkSMSourceProxy;
class vtkView;

namespace vtkSMViewProxyNS
{
class WindowToImageFilter;
class CaptureHelper;
}

class VTKREMOTINGVIEWS_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Enable/Disable a view.
   */
  vtkSetMacro(Enable, bool);
  vtkGetMacro(Enable, bool);
  vtkBooleanMacro(Enable, bool);
  //@}

  /**
   * Renders the view using full resolution.
   */
  virtual void StillRender();

  /**
   * Renders the view using lower resolution is possible.
   */
  virtual void InteractiveRender();

  /**
   * Called vtkPVView::Update on the server-side.
   */
  virtual void Update();

  /**
   * Returns true if the view can display the data produced by the producer's
   * port. Internally calls GetRepresentationType() and returns true only if the
   * type is valid a representation proxy of that type can be created.
   */
  virtual bool CanDisplayData(vtkSMSourceProxy* producer, int outputPort);

  /**
   * Create a default representation for the given source proxy.
   * Returns a new proxy.
   * In version 4.1 and earlier, subclasses overrode this method. Since 4.2, the
   * preferred way is to simply override GetRepresentationType(). That
   * ensures that CreateDefaultRepresentation() and CanDisplayData() both
   * work as expected.
   */
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);

  /**
   * Returns the xml name of the representation proxy to create to show the data
   * produced in this view, if any. Default implementation checks if the
   * producer has any "Hints" that define the representation to create in this
   * view and if so, returns that.
   * Or if this->DefaultRepresentationName is set and its Input property
   * can accept the data produced, returns this->DefaultRepresentationName.
   * Subclasses should override this method.
   */
  virtual const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort);

  /**
   * Finds the representation proxy showing the data produced by the provided
   * producer, if any. Note the representation may not necessarily be visible.
   */
  virtual vtkSMRepresentationProxy* FindRepresentation(vtkSMSourceProxy* producer, int outputPort);

  //@{
  /**
   * Captures a image from this view. Default implementation returns NULL.
   * Subclasses should override CaptureWindowInternal() to do the actual image
   * capture.
   */
  vtkImageData* CaptureWindow(int magnification)
  {
    return this->CaptureWindow(magnification, magnification);
  }
  vtkImageData* CaptureWindow(int magnificationX, int magnificationY);
  //@}
  /**
   * Returns the client-side vtkView, if any.
   */
  vtkView* GetClientSideView();

  /**
   * Saves a screenshot of the view to disk. The writerName argument specifies
   * the vtkImageWriter subclass to use.
   */
  int WriteImage(const char* filename, const char* writerName, int magnification = 1);
  int WriteImage(
    const char* filename, const char* writerName, int magnificationX, int magnificationY);

  /**
   * Return true any internal representation is dirty. This can be useful to
   * know if the internal geometry has changed.
   * DEPRECATED: Use GetNeedsUpdate() instead.
   */
  virtual bool HasDirtyRepresentation() { return this->GetNeedsUpdate(); }

  //@{
  /**
   * Returns true if the subsequent call to Update() will result in an actual
   * update. If returned true, it means that the view thinks its rendering is
   * obsolete and needs to be re-generated.
   */
  vtkGetMacro(NeedsUpdate, bool);
  //@}

  /**
   * Return the vtkRenderWindow used by this view, if any. Note, views like
   * vtkSMComparativeViewProxy can have more than 1 render window in play, in
   * which case, using this method alone may yield incorrect results. Also,
   * certain views don't use a vtkRenderWindow at all (e.g. Spreadsheet View),
   * in which case, this method will return NULL. Default implementation returns
   * NULL.
   */
  virtual vtkRenderWindow* GetRenderWindow() { return NULL; }

  /**
   * Returns the interactor. Note, that views may not use vtkRenderWindow at all
   * in which case they will not have any interactor and will return NULL.
   * Default implementation returns NULL.
   */
  virtual vtkRenderWindowInteractor* GetInteractor() { return NULL; }

  /**
   * A client process need to set the interactor to enable interactivity. Use
   * this method to set the interactor and initialize it as needed by the
   * RenderView. This include changing the interactor style as well as
   * overriding VTK rendering to use the Proxy/ViewProxy API instead.
   * Default implementation does nothing. Views that support interaction using
   * vtkRenderWindowInteractor should override this method to set the interactor
   * up.
   */
  virtual void SetupInteractor(vtkRenderWindowInteractor* iren) { (void)iren; }

  /**
   * Creates a default render window interactor for the vtkRenderWindow and sets
   * it up on the local process if the local process supports interaction. This
   * should not be used when putting the render window in a QVTKWidget as that
   * may cause issues. One should let the QVTKWidget create the interactor and
   * then call SetupInteractor().
   */
  virtual bool MakeRenderWindowInteractor(bool quiet = false);

  //@{
  /**
   * Sets whether screenshots have a transparent background.
   */
  static void SetTransparentBackground(bool val);
  static bool GetTransparentBackground();
  //@}

  //@{
  /**
   * Method used to hide other representations if the view has a
   * `<ShowOneRepresentationAtATime/>` hint.
   * This only affects other representations that have data inputs, not non-data
   * representations.
   *
   * @returns true if any representations were hidden by this call, otherwise
   *         returns false.
   */
  virtual bool HideOtherRepresentationsIfNeeded(vtkSMProxy* repr);
  static bool HideOtherRepresentationsIfNeeded(vtkSMViewProxy* self, vtkSMProxy* repr)
  {
    return self ? self->HideOtherRepresentationsIfNeeded(repr) : false;
  }
  //@}

  //@{
  /**
   * Certain views maintain properties (or other state) that should be updated
   * when visibility of representations is changed e.g. SpreadSheetView needs to
   * update the value of the "FieldAssociation" when a new data representation
   * is being shown in the view. Subclasses can override this method to perform
   * such updates to View properties. This is called explicitly by the
   * `vtkSMParaViewPipelineControllerWithRendering` after changing
   * representation visibility. Changes to representation visibility outside of
   * `vtkSMParaViewPipelineControllerWithRendering` will require calling this
   * method explicitly.
   *
   * Default implementation does not do anything.
   */
  virtual void RepresentationVisibilityChanged(vtkSMProxy* repr, bool new_visibility);
  static void RepresentationVisibilityChanged(
    vtkSMViewProxy* self, vtkSMProxy* repr, bool new_visibility)
  {
    if (self)
    {
      self->RepresentationVisibilityChanged(repr, new_visibility);
    }
  }
  //@}

  /**
   * Helper method to locate a view to which the representation has been added.
   */
  static vtkSMViewProxy* FindView(vtkSMProxy* repr, const char* reggroup = "views");

  enum
  {
    /**
     * Fired in `IsContextReadyForRendering` if the context is not already ready
     * for rendering.
     */
    PrepareContextForRendering = vtkCommand::UserEvent + 1,
  };

protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy() override;

  /**
   * Capture an image from the view's render window. Default implementation
   * simply captures the image from the render window for the view. Subclasses
   * may override this for cases where that's not sufficient.
   *
   * @param[in] magnificationX The X magnification factor to use for generating the image.
   * @param[in] magnificationY The Y magnification factor to use for generating the image.
   * @returns A new vtkImageData instance or nullptr. Caller is responsible for
   *          calling `vtkImageData::Delete()` on the returned non-null value.
   */
  virtual vtkImageData* CaptureWindowInternal(int magnificationX, int magnificationY);

  /**
   * This method is called whenever the view wants to render to during image
   * capture. The default implementation simply calls this->StillRender().
   */
  virtual void RenderForImageCapture() { this->StillRender(); }

  /**
   * This method is called before executing code that could cause a render on
   * the underlying vtkPVView. This is the method where subclasses can ensure
   * that the data for rendering is made available ranks that will be doing the
   * rendering and then return the location where the rendering will happen.
   *
   * Thus vtkSMViewProxy can send the render request to only those processes
   * that will be doing rendering avoiding unnecessary communication to
   * non-participating ranks.
   */
  virtual vtkTypeUInt32 PreRender(bool vtkNotUsed(interactive)) { return this->GetLocation(); }

  virtual void PostRender(bool vtkNotUsed(interactive)) {}

  /**
   * Subclasses should override this method and return false if the rendering
   * context is not ready for rendering at this moment. This method is called in
   * StillRender() and InteractiveRender() calls before the actual render to
   * ensure that we don't attempt to render when the rendering context is not
   * ready.
   * Default implementation uses this->GetRenderWindow() and checks if that
   * window is drawable.
   */
  virtual bool IsContextReadyForRendering();

  /**
   * Called at the end of CreateVTKObjects().
   */
  void CreateVTKObjects() override;

  /**
   * Read attributes from an XML element.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  /**
   * Convenience method to call
   * vtkPVView::SafeDownCast(this->GetClientSideObject())->GetLocalProcessSupportsInteraction();
   */
  bool GetLocalProcessSupportsInteraction();

  /**
   * Provides access to the delivery manager proxy, if any.
   */
  vtkGetObjectMacro(DeliveryManager, vtkSMDataDeliveryManagerProxy);

  vtkSetStringMacro(DefaultRepresentationName);
  char* DefaultRepresentationName;

  bool Enable;

private:
  vtkSMViewProxy(const vtkSMViewProxy&) = delete;
  void operator=(const vtkSMViewProxy&) = delete;

  vtkSMDataDeliveryManagerProxy* DeliveryManager;
  static bool TransparentBackground;

  // Actual logic for taking a screenshot.
  vtkImageData* CaptureWindowSingle(int magnificationX, int magnificationY);

  friend class vtkSMViewProxyNS::WindowToImageFilter;
  friend class vtkSMViewProxyNS::CaptureHelper;
};

#endif
