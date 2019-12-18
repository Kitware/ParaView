/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMComparativeViewProxy
 * @brief   view for comparative visualization/
 * film-strips.
 *
 * vtkSMComparativeViewProxy is the view used to generate/view comparative
 * visualizations/film-strips. vtkSMComparativeViewProxy works together with
 * vtkPVComparativeView -- the vtk-object for which this represents the proxy.
 * vtkPVComparativeView is a client-side VTK object which literally uses the
 * view and representation proxies to simulate the comparative view. Refer to
 * vtkPVComparativeView for details.
*/

#ifndef vtkSMComparativeViewProxy_h
#define vtkSMComparativeViewProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMViewProxy.h"

class vtkCollection;

class VTKREMOTINGVIEWS_EXPORT vtkSMComparativeViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMComparativeViewProxy* New();
  vtkTypeMacro(vtkSMComparativeViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the data pipelines for all visible representations.
   */
  void Update() override;

  /**
   * Get all the internal views. The views should only be used to be laid out
   * by the GUI. It's not recommended to directly change the properties of the
   * views.
   */
  void GetViews(vtkCollection* collection);

  /**
   * Returns the root view proxy.
   */
  vtkSMViewProxy* GetRootView();

  /**
   * Dirty means this algorithm will execute during next update.
   * This all marks all consumers as dirty.
   */
  void MarkDirty(vtkSMProxy* modifiedProxy) override;

  /**
   * Overridden to forward the call to the internal root view proxy.
   */
  const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort) override;

  /**
   * Returns the render-window used by the root view, if any.
   */
  vtkRenderWindow* GetRenderWindow() override
  {
    return this->GetRootView() ? this->GetRootView()->GetRenderWindow() : NULL;
  }
  vtkRenderWindowInteractor* GetInteractor() override
  {
    return this->GetRootView() ? this->GetRootView()->GetInteractor() : NULL;
  }

  /**
   * To avoid misuse of this method for comparative views, this method will
   * raise an error. Client code should set up interactor for each of the
   * internal views explicitly.
   */
  void SetupInteractor(vtkRenderWindowInteractor* iren) override;

  /**
   * Overridden to call MakeRenderWindowInteractor() on each of the internal
   * views.
   */
  bool MakeRenderWindowInteractor(bool quiet = false) override;

protected:
  vtkSMComparativeViewProxy();
  ~vtkSMComparativeViewProxy() override;

  /**
   * Overridden to do the capturing of images from each of the internal views
   * and then stitching them together.
   */
  vtkImageData* CaptureWindowInternal(int magX, int magY) override;

  void InvokeConfigureEvent();

  void CreateVTKObjects() override;

private:
  vtkSMComparativeViewProxy(const vtkSMComparativeViewProxy&) = delete;
  void operator=(const vtkSMComparativeViewProxy&) = delete;
};

#endif
