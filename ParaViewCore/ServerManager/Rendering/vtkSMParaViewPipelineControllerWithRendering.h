/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParaViewPipelineControllerWithRendering.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMParaViewPipelineControllerWithRendering
// .SECTION Description
// vtkSMParaViewPipelineControllerWithRendering overrides
// vtkSMParaViewPipelineController to add support for initializing rendering
// proxies appropriately. vtkSMParaViewPipelineControllerWithRendering uses
// vtkObjectFactory mechanisms to override vtkSMParaViewPipelineController's
// creation. One should not need to create or use this class directly (excepting
// when needing to subclass). Simply create vtkSMParaViewPipelineController. If
// the application is linked with the rendering module, then this class will be
// instantiated instead of vtkSMParaViewPipelineController automatically.
//
// vtkSMParaViewPipelineControllerWithRendering also adds new API to control
// representation visibility and manage creation of views. To use that API
// clients can instantiate vtkSMParaViewPipelineControllerWithRendering. Just
// like vtkSMParaViewPipelineController, this class also uses vtkObjectFactory
// mechanisms to enable overriding by clients at compile time.

#ifndef __vtkSMParaViewPipelineControllerWithRendering_h
#define __vtkSMParaViewPipelineControllerWithRendering_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMParaViewPipelineController.h"

class vtkSMSourceProxy;
class vtkSMViewLayoutProxy;
class vtkSMViewProxy;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMParaViewPipelineControllerWithRendering :
  public vtkSMParaViewPipelineController
{
public:
  static vtkSMParaViewPipelineControllerWithRendering* New();
  vtkTypeMacro(vtkSMParaViewPipelineControllerWithRendering, vtkSMParaViewPipelineController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Show the output data in the view. If data cannot be shown in the view,
  // returns NULL. If \c view is NULL, this simply calls ShowInPreferredView().
  virtual vtkSMProxy* Show(
    vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view);

  // Description:
  // Opposite of Show(). Locates the representation for the producer and then
  // hides it, if found. Returns that representation, if found.
  virtual vtkSMProxy* Hide(
    vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view);

  // Description:
  // Same as above, except that when we already have the representation located.
  virtual void Hide(vtkSMProxy* repr, vtkSMViewProxy* view);

  // Description:
  // Alternative method to call Show and Hide using a visibility flag.
  vtkSMProxy* SetVisibility(
    vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view, bool visible)
    {
    return (visible?
      this->Show(producer, outputPort, view):
      this->Hide(producer, outputPort, view));
    }

  // Description:
  // Returns whether the producer/port are shown in the given view.
  virtual bool GetVisibility(
    vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view);

  // Description:
  // Same as Show() except that if the \c view is NULL or not the "preferred"
  // view for the producer's output, this method will create a new view and show
  // the data in that new view. Returns the view in which the data ends up being
  // shown, if any. It may return NULL if the \c view is not the "preferred"
  // view and "preferred" view could not be determined or created.
  virtual vtkSMViewProxy* ShowInPreferredView(
    vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view);

  // Description:
  // Returns the name for the preferred view type, if there is any.
  virtual const char* GetPreferredViewType(vtkSMSourceProxy* producer, int outputPort);

  // Description:
  // Overridden to create color and opacity transfer functions if applicable.
  // While it is tempting to add any default property setup logic in such
  // overrides, we must keep such overrides to a minimal and opting for domains
  // that set appropriate defaults where as much as possible.
  virtual bool RegisterRepresentationProxy(vtkSMProxy* proxy);

  // Description:
  // Control how scalar bar visibility is updated by the Hide call.
  static void SetHideScalarBarOnHide(bool);

  // Description:
  // Control whether representations try to maintain properties from an input
  // representation, if present. e.g. if you "Transform" the representation for
  // a source, then any filter that you connect to it should be transformed as
  // well.
  static void SetInheritRepresentationProperties(bool);
  static bool GetInheritRepresentationProperties();

  // Description:
  // Methods to save/capture images from views.
  virtual bool WriteImage(vtkSMViewProxy* view,
      const char* filename, int magnification, int quality);
  virtual bool WriteImage(vtkSMViewLayoutProxy* layout,
      const char* filename, int magnification, int quality);

  // Description:
  // Overridden to handle default ColorArrayName for representations correctly.
  virtual bool PostInitializeProxy(vtkSMProxy* proxy);

  // Description:
  // Overridden to place the view in a layout on creation.
  virtual bool RegisterViewProxy(vtkSMProxy* proxy, const char* proxyname);
  using Superclass::RegisterViewProxy;

  // Description:
  // Register layout proxy.
  virtual bool RegisterLayoutProxy(vtkSMProxy* proxy, const char* proxyname=NULL);

//BTX
protected:
  vtkSMParaViewPipelineControllerWithRendering();
  ~vtkSMParaViewPipelineControllerWithRendering();

  virtual void UpdatePipelineBeforeDisplay(
    vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view);

private:
  vtkSMParaViewPipelineControllerWithRendering(const vtkSMParaViewPipelineControllerWithRendering&); // Not implemented
  void operator=(const vtkSMParaViewPipelineControllerWithRendering&); // Not implemented
  static bool HideScalarBarOnHide;
  static bool InheritRepresentationProperties;
//ETX
};

#endif
