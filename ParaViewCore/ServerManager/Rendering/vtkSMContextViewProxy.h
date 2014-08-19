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
// .NAME vtkSMContextViewProxy - abstract base class for all Chart Views.
// .SECTION Description
// vtkSMContextViewProxy is an abstract base class for all vtkContextView
// subclasses.

#ifndef __vtkSMContextViewProxy_h
#define __vtkSMContextViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMViewProxy.h"

class vtkAbstractContextItem;
class vtkContextView;
class vtkImageData;
class vtkRenderWindow;
class vtkSelection;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMContextViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMContextViewProxy* New();
  vtkTypeMacro(vtkSMContextViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Provides access to the vtk chart view.
  vtkContextView* GetContextView();

  // Description:
  // Provides access to the vtk chart.
  virtual vtkAbstractContextItem* GetContextItem();
//ETX

  // Description:
  // Resets the zoom level to 100%
  virtual void ResetDisplay();

  // Description:
  // Return the render window from which offscreen rendering and interactor can
  // be accessed
  virtual vtkRenderWindow* GetRenderWindow();

  // Description:
  // Overridden to report to applications that producers producing non-table
  // datasets are only viewable if they have the "Plottable" hint. This avoid
  // applications from inadvertently showing large data in charts.
  // CreateDefaultRepresentation() will still work without regard for this
  // Plottable hint.
  virtual bool CanDisplayData(vtkSMSourceProxy* producer, int outputPort);

  vtkSelection* GetCurrentSelection();


//BTX
protected:
  vtkSMContextViewProxy();
  ~vtkSMContextViewProxy();

  // Description:
  // Subclasses should override this method to do the actual image capture.
  virtual vtkImageData* CaptureWindowInternal(int magnification);

  // Description:
  virtual void CreateVTKObjects();

  // Description:
  // Used to update the axis range properties on each interaction event.
  // This also fires the vtkCommand::InteractionEvent.
  void OnInteractionEvent();

  // Description:
  // Overridden to update ChartAxes ranges on every render. This ensures that
  // the property's values are up-to-date.
  virtual void PostRender(bool interactive);

  // Description:
  // Overridden to process the "skip_plotable_check" attribute.
  virtual int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element);

  // Description:
  // The context view that is used for all context derived charts.
  vtkContextView* ChartView;

  // Description:
  // To avoid showing large datasets in context views, that typically rely on
  // delivering all data to the client side (or cloning it), by default make
  // extra checks for data type and hints in CanDisplayData(). Certain views
  // types however, (e.g. XYHistogramView) can show any type of data without
  // this limitation. For such views, we set this flag to true using XML
  // attribute "skip_plotable_check".
  bool SkipPlotableCheck;
private:
  vtkSMContextViewProxy(const vtkSMContextViewProxy&); // Not implemented
  void operator=(const vtkSMContextViewProxy&); // Not implemented

  // Description:
  // Copies axis ranges from each of the vtkAxis on the vtkChartXY to the
  // SMproperties.
  void CopyAxisRangesFromChart();
//ETX
};

#endif

