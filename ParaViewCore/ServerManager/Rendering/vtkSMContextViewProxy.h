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

#include "vtkSMViewProxy.h"

class vtkAbstractContextItem;
class vtkContextView;
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
  // Used to update the axis range properties on each interaction event.
  // This also fires the vtkCommand::InteractionEvent.
  void OnInteractionEvent();

  // Description:
  // The context view that is used for all context derived charts.
  vtkContextView* ChartView;

private:
  vtkSMContextViewProxy(const vtkSMContextViewProxy&); // Not implemented
  void operator=(const vtkSMContextViewProxy&); // Not implemented
//ETX
};

#endif

