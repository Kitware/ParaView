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

#include "vtkNew.h"                 // needed for vtkInteractorObserver.
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMViewProxy.h"

class vtkAbstractContextItem;
class vtkContextView;
class vtkEventForwarderCommand;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSelection;
class vtkSMViewProxyInteractorHelper;

class VTKREMOTINGVIEWS_EXPORT vtkSMContextViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMContextViewProxy* New();
  vtkTypeMacro(vtkSMContextViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  vtkRenderWindow* GetRenderWindow() override;

  /**
   * A client process need to set the interactor to enable interactivity. Use
   * this method to set the interactor and initialize it as needed by the
   * RenderView. This include changing the interactor style as well as
   * overriding VTK rendering to use the Proxy/ViewProxy API instead.
   */
  void SetupInteractor(vtkRenderWindowInteractor* iren) override;

  /**
   * Returns the interactor.
   */
  vtkRenderWindowInteractor* GetInteractor() override;

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
  bool CanDisplayData(vtkSMSourceProxy* producer, int outputPort) override;

  /**
   * Overridden to create `ChartTextRepresentation` for text sources.
   */
  const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort) override;

  vtkSelection* GetCurrentSelection();

protected:
  vtkSMContextViewProxy();
  ~vtkSMContextViewProxy() override;

  void CreateVTKObjects() override;

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
  void PostRender(bool interactive) override;

  /**
   * Overridden to process the "skip_plotable_check" attribute.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  /**
   * Overridden to return a location of vtkPVSession::CLIENT unless in
   * tile-display mode, in which case
   * `vtkPVSession::CLIENT |vtkPVSession::RENDER_SERVER` is returned.
   */
  vtkTypeUInt32 PreRender(bool interactive) override;

  /**
   * The context view that is used for all context derived charts.
   */
  vtkContextView* ChartView;

  /**
   * To avoid showing large datasets in context views, that typically rely on
   * delivering all data to the client side (or cloning it), by default make
   * extra checks for data type and hints in CanDisplayData(). Certain views
   * types however, (e.g. XYHistogramView) can show any type of data without
   * this limitation. For such views, we set this flag to true using XML
   * attribute "skip_plotable_check".
   */
  bool SkipPlotableCheck;

  /**
   * Flag automatically set when Bottom and Right custom
   * axes related property are present in this proxy.
   * Used to know if these must be set or not
   */
  bool XYChartViewBase4Axes;

private:
  vtkSMContextViewProxy(const vtkSMContextViewProxy&) = delete;
  void operator=(const vtkSMContextViewProxy&) = delete;

  /**
   * Copies axis ranges from each of the vtkAxis on the vtkChartXY to the
   * SMproperties.
   */
  void CopyAxisRangesFromChart();

  vtkNew<vtkSMViewProxyInteractorHelper> InteractorHelper;
  vtkNew<vtkEventForwarderCommand> EventForwarder;
};

#endif
