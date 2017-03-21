/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMContextViewProxy
 * @brief   abstract base class for all Chart Views.
 *
 * vtkSMContextViewProxy is an abstract base class for all vtkContextView
 * subclasses.
*/

#ifndef vtkSMContextViewProxy_h
#define vtkSMContextViewProxy_h

#include "vtkNew.h"                            // needed for vtkInteractorObserver.
#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMViewProxy.h"

class vtkAbstractContextItem;
class vtkContextView;
class vtkEventForwarderCommand;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSelection;
class vtkSMViewProxyInteractorHelper;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMContextViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMContextViewProxy* New();
  vtkTypeMacro(vtkSMContextViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Provides access to the vtk chart view.
   */
  vtkContextView* GetContextView();

  /**
   * Provides access to the vtk chart.
   */
  virtual vtkAbstractContextItem* GetContextItem();

  /**
   * Return the render window from which offscreen rendering and interactor can
   * be accessed
   */
  virtual vtkRenderWindow* GetRenderWindow() VTK_OVERRIDE;

  /**
   * A client process need to set the interactor to enable interactivity. Use
   * this method to set the interactor and initialize it as needed by the
   * RenderView. This include changing the interactor style as well as
   * overriding VTK rendering to use the Proxy/ViewProxy API instead.
   */
  virtual void SetupInteractor(vtkRenderWindowInteractor* iren) VTK_OVERRIDE;

  /**
   * Returns the interactor.
   */
  virtual vtkRenderWindowInteractor* GetInteractor() VTK_OVERRIDE;

  /**
   * Resets the zoom level to 100%
   */
  virtual void ResetDisplay();

  /**
   * Overridden to report to applications that producers producing non-table
   * datasets are only viewable if they have the "Plottable" hint. This avoid
   * applications from inadvertently showing large data in charts.
   * CreateDefaultRepresentation() will still work without regard for this
   * Plottable hint.
   */
  virtual bool CanDisplayData(vtkSMSourceProxy* producer, int outputPort) VTK_OVERRIDE;

  vtkSelection* GetCurrentSelection();

protected:
  vtkSMContextViewProxy();
  ~vtkSMContextViewProxy();

  virtual void CreateVTKObjects() VTK_OVERRIDE;

  /**
   * Used to update the axis range properties on each interaction event.
   * This also fires the vtkCommand::InteractionEvent.
   */
  void OnInteractionEvent();

  /**
   * Forwards vtkCommand::StartInteractionEvent and
   * vtkCommand::EndInteractionEvent
   * from the vtkRenderWindowInteractor
   */
  void OnForwardInteractionEvent(vtkObject*, unsigned long, void*);

  /**
   * Used to update the legend position on interaction event.
   * This also fires the vtkCommand::InteractionEvent.
   */
  void OnLeftButtonReleaseEvent();

  /**
   * Overridden to update ChartAxes ranges on every render. This ensures that
   * the property's values are up-to-date.
   */
  virtual void PostRender(bool interactive) VTK_OVERRIDE;

  /**
   * Overridden to process the "skip_plotable_check" attribute.
   */
  virtual int ReadXMLAttributes(
    vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) VTK_OVERRIDE;

  /**
   * The context view that is used for all context derived charts.
   */
  vtkContextView* ChartView;

  //@{
  /**
   * To avoid showing large datasets in context views, that typically rely on
   * delivering all data to the client side (or cloning it), by default make
   * extra checks for data type and hints in CanDisplayData(). Certain views
   * types however, (e.g. XYHistogramView) can show any type of data without
   * this limitation. For such views, we set this flag to true using XML
   * attribute "skip_plotable_check".
   */
  bool SkipPlotableCheck;

private:
  vtkSMContextViewProxy(const vtkSMContextViewProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMContextViewProxy&) VTK_DELETE_FUNCTION;
  //@}

  /**
   * Copies axis ranges from each of the vtkAxis on the vtkChartXY to the
   * SMproperties.
   */
  void CopyAxisRangesFromChart();

  vtkNew<vtkSMViewProxyInteractorHelper> InteractorHelper;
  vtkNew<vtkEventForwarderCommand> EventForwarder;
};

#endif
