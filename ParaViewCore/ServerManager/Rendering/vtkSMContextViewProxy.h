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
  // The context view that is used for all context derived charts.
  vtkContextView* ChartView;

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

