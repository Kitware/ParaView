/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVContextView
 *
 * vtkPVContextView adopts vtkContextView so that it can be used in ParaView
 * configurations.
*/

#ifndef vtkPVContextView_h
#define vtkPVContextView_h

#include "vtkNew.h"                               // needed for vtkNew.
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVView.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkAbstractContextItem;
class vtkChart;
class vtkChartRepresentation;
class vtkPVContextInteractorStyle;
class vtkContextView;
class vtkCSVExporter;
class vtkInformationIntegerKey;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSelection;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVContextView : public vtkPVView
{
public:
  vtkTypeMacro(vtkPVContextView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Triggers a high-resolution render.
   * \note CallOnAllProcesses
   */
  void StillRender() VTK_OVERRIDE;

  /**
   * Triggers a interactive render. Based on the settings on the view, this may
   * result in a low-resolution rendering or a simplified geometry rendering.
   * \note CallOnAllProcesses
   */
  void InteractiveRender() VTK_OVERRIDE;

  //@{
  /**
   * Get the context view.
   */
  vtkGetObjectMacro(ContextView, vtkContextView);
  //@}

  /**
   * Get the context item.
   */
  virtual vtkAbstractContextItem* GetContextItem() = 0;

  //@{
  vtkRenderWindow* GetRenderWindow() VTK_OVERRIDE { return this->RenderWindow; }
  //@}

  //@{
  /**
   * Set the interactor. Client applications must set the interactor to enable
   * interactivity. Note this method will also change the interactor styles set
   * on the interactor.
   */
  virtual void SetupInteractor(vtkRenderWindowInteractor*);
  vtkRenderWindowInteractor* GetInteractor();
  //@}

  /**
   * Initialize the view with an identifier. Unless noted otherwise, this method
   * must be called before calling any other methods on this class.
   * \note CallOnAllProcesses
   */
  void Initialize(unsigned int id) VTK_OVERRIDE;

  /**
   * Overridden to ensure that in multi-client configurations, same set of
   * representations are "dirty" on all processes to avoid race conditions.
   */
  void Update() VTK_OVERRIDE;

  /**
   * Representations can use this method to set the selection for a particular
   * representation. Subclasses override this method to pass on the selection to
   * the chart using annotation link. Note this is meant to pass selection for
   * the local process alone. The view does not manage data movement for the
   * selection.
   */
  virtual void SetSelection(vtkChartRepresentation* repr, vtkSelection* selection) = 0;

  /**
   * Get the current selection created in the view. This will call
   * this->MapSelectionToInput() to transform the selection every time a new
   * selection is available. Subclasses should override MapSelectionToInput() to
   * convert the selection, as appropriate.
   */
  vtkSelection* GetSelection();

  /**
   * Export the contents of this view using the exporter.
   * Called vtkChartRepresentation::Export() on all visible representations.
   * This is expected to called only on the client side after a render/update.
   * Thus all data is expected to available on the local process.
   */
  virtual bool Export(vtkCSVExporter* exporter);

protected:
  vtkPVContextView();
  ~vtkPVContextView() override;

  /**
   * Actual rendering implementation.
   */
  virtual void Render(bool interactive);

  /**
   * Called to transform the selection. This is only called on the client-side.
   * Subclasses should transform the selection in place as needed. Default
   * implementation simply goes to the first visible representation and asks it
   * to transform (by calling vtkChartRepresentation::MapSelectionToInput()).
   * We need to extend the infrastructrue to work properly when making
   * selections in views showing multiple representations, but until that
   * happens, this naive approach works for most cases.
   * Return false on failure.
   */
  virtual bool MapSelectionToInput(vtkSelection*);

  //@{
  /**
   * Callbacks called when the primary "renderer" in the vtkContextView
   * starts/ends rendering. Note that this is called on the renderer, hence
   * before the rendering cleanup calls like SwapBuffers called by the
   * render-window.
   */
  void OnStartRender();
  void OnEndRender();
  //@}

  vtkContextView* ContextView;
  vtkRenderWindow* RenderWindow;

private:
  vtkPVContextView(const vtkPVContextView&) = delete;
  void operator=(const vtkPVContextView&) = delete;

  // Used in GetSelection to avoid modifying the selection obtained from the
  // annotation link.
  vtkSmartPointer<vtkSelection> SelectionClone;
  vtkNew<vtkPVContextInteractorStyle> InteractorStyle;

  template <class T>
  vtkSelection* GetSelectionImplementation(T* chart);
};

#endif
