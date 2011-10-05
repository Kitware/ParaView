/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotMatrixView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkPVPlotMatrixView_h
#define __vtkPVPlotMatrixView_h

#include "vtkPVContextView.h"

#include "vtkScatterPlotMatrix.h"

class VTK_EXPORT vtkPVPlotMatrixView : public vtkPVContextView
{
public:
  static vtkPVPlotMatrixView* New();
  vtkTypeMacro(vtkPVPlotMatrixView, vtkPVContextView);
  void PrintSelf(ostream &os, vtkIndent indent);

  vtkAbstractContextItem* GetContextItem() { return this->PlotMatrix; }

protected:
  vtkPVPlotMatrixView();
  ~vtkPVPlotMatrixView();

private:
  vtkPVPlotMatrixView(const vtkPVPlotMatrixView&); // Not implemented.
  void operator=(const vtkPVPlotMatrixView&); // Not implemented.

  vtkScatterPlotMatrix *PlotMatrix;
};

#endif
