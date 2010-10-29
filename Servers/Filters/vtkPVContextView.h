/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVContextView
// .SECTION Description
// vtkPVContextView adopts vtkContextView so that it can be used in ParaView
// configurations.

#ifndef __vtkPVContextView_h
#define __vtkPVContextView_h

#include "vtkPVView.h"

class vtkContextView;
class vtkRenderWindow;
class vtkChart;

class VTK_EXPORT vtkPVContextView : public vtkPVView
{
public:
  vtkTypeMacro(vtkPVContextView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Triggers a high-resolution render.
  // @CallOnAllProcessess
  virtual void StillRender();

  // Description:
  // Triggers a interactive render. Based on the settings on the view, this may
  // result in a low-resolution rendering or a simplified geometry rendering.
  // @CallOnAllProcessess
  virtual void InteractiveRender();

  // Description:
  // Get the context view.
  vtkGetObjectMacro(ContextView, vtkContextView);

  // Description:
  // Get the chart.
  virtual vtkChart* GetChart()=0;

  // Description:
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

//BTX
protected:
  vtkPVContextView();
  ~vtkPVContextView();

  // Description:
  // Actual rendering implementation.
  virtual void Render(bool interactive);

  vtkContextView* ContextView;
  vtkRenderWindow* RenderWindow;

private:
  vtkPVContextView(const vtkPVContextView&); // Not implemented
  void operator=(const vtkPVContextView&); // Not implemented
//ETX
};

#endif
