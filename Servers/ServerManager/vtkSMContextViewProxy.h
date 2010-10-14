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
  static vtkSMContextViewProxy* New();
  vtkTypeMacro(vtkSMContextViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Provides access to the vtk chart view.
  vtkContextView* GetChartView();

  // Description:
  // Provides access to the vtk chart.
  virtual vtkChart* GetChart();
//ETX

  // Description:
  // Return the render window from which offscreen rendering and interactor can
  // be accessed
  vtkRenderWindow* GetRenderWindow();

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
  // The context view that is used for all context derived charts.
  vtkContextView* ChartView;

private:
  vtkSMContextViewProxy(const vtkSMContextViewProxy&); // Not implemented
  void operator=(const vtkSMContextViewProxy&); // Not implemented

  // Description:
  // Private storage object.
  class Private;
  Private *Storage;

//ETX
};

#endif

