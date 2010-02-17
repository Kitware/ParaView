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
class vtkImageData;
//BTX
class QVTKWidget;
//ETX

class VTK_EXPORT vtkSMContextViewProxy : public vtkSMViewProxy
{
public:
  vtkTypeRevisionMacro(vtkSMContextViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Saves the chart view as an image file.  See vtkQtChartView::SaveImage().
  // Returns true on success.
  bool WriteImage(const char* filename);

//BTX
  // Description:
  // Provides access to the chart view's widget.
  QVTKWidget* GetChartWidget();

  // Description:
  // Provides access to the vtk chart view.
  vtkContextView* GetChartView();
//ETX

  // Description:
  // Capture the contents of the window at the specified magnification level.
  vtkImageData* CaptureWindow(int magnification);

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

