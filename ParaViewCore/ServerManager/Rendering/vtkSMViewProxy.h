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

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkImageData;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSMRepresentationProxy;
class vtkSMSourceProxy;
class vtkView;

namespace vtkSMViewProxyNS
{
class WindowToImageFilter;
}

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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

  /**
   * Captures a image from this view. Default implementation returns NULL.
   * Subclasses should override CaptureWindowInternal() to do the actual image
   * capture.
   */
  vtkImageData* CaptureWindow(int magnification);

  /**
   * Returns the client-side vtkView, if any.
   */
  vtkView* GetClientSideView();

  /**
   * Saves a screenshot of the view to disk. The writerName argument specifies
   * the vtkImageWriter subclass to use.
   */
  int WriteImage(const char* filename, const char* writerName, int magnification = 1);

  /**
   * Return true any internal representation is dirty. This can be usefull to
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

protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy();

  /**
   * Capture an image from the view's render window. Default implementation
   * simply captures the image from the render window for the view. Subclasses
   * may override this for cases where that's not sufficient.
   *
   * @param[in] magnification The magnification factor to use for generating the image.
   * @returns A new vtkImageData instance or nullptr. Caller is responsible for
   *          calling `vtkImageData::Delete()` on the returned non-null value.
   */
  virtual vtkImageData* CaptureWindowInternal(int magnification);

  /**
   * This method is called whenever the view wants to render to during image
   * capture. The default implementation simply calls this->StillRender().
   */
  virtual void RenderForImageCapture() { this->StillRender(); }

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
  virtual void CreateVTKObjects() VTK_OVERRIDE;

  /**
   * Read attributes from an XML element.
   */
  virtual int ReadXMLAttributes(
    vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) VTK_OVERRIDE;

  /**
   * Convenience method to call
   * vtkPVView::SafeDownCast(this->GetClientSideObject())->GetLocalProcessSupportsInteraction();
   */
  bool GetLocalProcessSupportsInteraction();

  vtkSetStringMacro(DefaultRepresentationName);
  char* DefaultRepresentationName;

  bool Enable;

private:
  vtkSMViewProxy(const vtkSMViewProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMViewProxy&) VTK_DELETE_FUNCTION;

  static bool TransparentBackground;

  // When view's time changes, there's no way for the client-side proxies to
  // know that they may re-execute and their data info is invalid. So mark those
  // dirty explicitly.
  void ViewTimeChanged();

  // Actual logic for taking a screenshot.
  vtkImageData* CaptureWindowSingle(int magnification);
  class vtkRendererSaveInfo;
  vtkRendererSaveInfo* PrepareRendererBackground(vtkRenderer*, double, double, double, bool);
  void RestoreRendererBackground(vtkRenderer*, vtkRendererSaveInfo*);

  friend class vtkSMViewProxyNS::WindowToImageFilter;
};

#endif
