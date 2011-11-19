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

#include "vtkPVPlotMatrixView.h"

#include "vtkObjectFactory.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkPVPlotMatrixView);

//----------------------------------------------------------------------------
vtkPVPlotMatrixView::vtkPVPlotMatrixView()
{
  this->PlotMatrix = vtkScatterPlotMatrix::New();
  this->PlotMatrix->AddObserver(
    vtkCommand::SelectionChangedEvent, this,
    &vtkPVPlotMatrixView::PlotMatrixSelectionCallback);

  this->ContextView->GetScene()->AddItem(this->PlotMatrix);
}

//----------------------------------------------------------------------------
vtkPVPlotMatrixView::~vtkPVPlotMatrixView()
{
  this->PlotMatrix->Delete();
}

//----------------------------------------------------------------------------
vtkAbstractContextItem* vtkPVPlotMatrixView::GetContextItem()
{
  return this->PlotMatrix;
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::PlotMatrixSelectionCallback(vtkObject*,
  unsigned long event, void*)
{
  // forward the SelectionChangedEvent
  this->InvokeEvent(event);
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixView::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
