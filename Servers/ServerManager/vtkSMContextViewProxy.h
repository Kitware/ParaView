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
// vtkSMContextViewProxy is an abstract base class for all vtkQtChartView
// subclasses. These are the Qt-based charting views.

#ifndef __vtkSMContextViewProxy_h
#define __vtkSMContextViewProxy_h

#include "vtkSMViewProxy.h"

class vtkContextView;
class vtkChart;
class vtkImageData;
class vtkRenderWindow;

class VTK_EXPORT vtkSMContextViewProxy : public vtkSMViewProxy
{
public:
  vtkTypeMacro(vtkSMContextViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Saves a screenshot of the view to disk.  The writerName argument specifies
  // the vtkImageWriter subclass to use.
  int WriteImage(const char* filename, const char* writerName, int magnification);

//BTX
  // Description:
  // Provides access to the vtk chart view.
  vtkContextView* GetChartView();

  // Description:
  // Provides access to the vtk chart.
  virtual vtkChart* GetChart();
//ETX

  // Description:
  // Capture the contents of the window at the specified magnification level.
  vtkImageData* CaptureWindow(int magnification);

  // Description:
  // Return the render window from which offscreen rendering and interactor can
  // be accessed
  vtkRenderWindow* GetRenderWindow();

//BTX
protected:
  vtkSMContextViewProxy();
  ~vtkSMContextViewProxy();

  // Description:
  // Called once in CreateVTKObjects() to create a new chart view.
  virtual vtkContextView* NewChartView()=0;

  // Description:
  virtual void CreateVTKObjects();

  // Description:
  // Performs the actual rendering. This method is called by
  // both InteractiveRender() and StillRender().
  // Default implementation is empty.
  virtual void PerformRender();

  // Description:
  // The context view that is used for all context derived charts.
  vtkContextView* ChartView;

  // Description:
  // Private storage object.
  class Private;
  Private *Storage;

private:
  vtkSMContextViewProxy(const vtkSMContextViewProxy&); // Not implemented
  void operator=(const vtkSMContextViewProxy&); // Not implemented
//ETX
};

#endif

