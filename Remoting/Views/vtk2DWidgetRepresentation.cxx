/*=========================================================================

  Program:   ParaView
  Module:    vtk2DWidgetRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk2DWidgetRepresentation.h"

#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPVXYChartViewInteractive.h"

vtkStandardNewMacro(vtk2DWidgetRepresentation);

vtk2DWidgetRepresentation::vtk2DWidgetRepresentation()
  : ContextItem(nullptr)
  , Enabled(false)
  , View(nullptr)
  , ObserverTag(0)
{
  this->SetNumberOfInputPorts(0);
}

vtk2DWidgetRepresentation::~vtk2DWidgetRepresentation()
{
  if (this->View)
  {
    this->View->GetContextView()->GetScene()->RemoveItem(this->ContextItem);
    this->View = nullptr;
    this->ContextItem = nullptr;
  }
}

bool vtk2DWidgetRepresentation::AddToView(vtkView* view)
{
  vtkPVXYChartViewInteractive* pvview = vtkPVXYChartViewInteractive::SafeDownCast(view);
  if (pvview)
  {
    this->View = pvview;
    vtkContextScene* scene = this->View->GetContextView()->GetScene();
    auto itemsCount = scene->GetNumberOfItems();
    for (unsigned int i = 0; i < itemsCount; ++i)
    {
      vtkAbstractContextItem* item = scene->GetItem(i);
      vtkChartXY* chart = vtkChartXY::SafeDownCast(item);
      if (chart)
      {
        auto transforms = chart->GetTransforms();
        this->ContextItem->SetTransform(transforms.at(0));
        break;
      }
    }

    scene->RemoveItem(this->ContextItem);
    scene->AddItem(this->ContextItem);

    return true;
  }

  return false;
}

bool vtk2DWidgetRepresentation::RemoveFromView(vtkView*)
{
  // TODO: see for details
  // https://gitlab.kitware.com/paraview/paraview/-/merge_requests/3546#note_955876
  //    vtkPVXYChartViewInteractive* pvview = vtkPVXYChartViewInteractive::SafeDownCast(view);
  //    if (pvview)
  //    {
  //      this->View = pvview;
  //      vtkContextScene* scene = this->View->GetContextView()->GetScene();
  //      scene->RemoveItem(this->ContextItem);
  //      return true;
  //    }
  //    return false;
  return true;
}

void vtk2DWidgetRepresentation::OnContextItemModified()
{
  if (this->View != nullptr)
  {
    this->View->Update();
  }
}

void vtk2DWidgetRepresentation::PrintSelf(std::ostream& os, vtkIndent indent)
{
  vtkDataRepresentation::PrintSelf(os, indent);
}

void vtk2DWidgetRepresentation::SetContextItem(vtkContextItem* item)
{
  if (this->ContextItem == item)
  {
    return;
  }

  if (this->ContextItem)
  {
    this->ContextItem->RemoveObserver(this->ObserverTag);
    this->ObserverTag = 0;
  }

  this->ContextItem = item;

  if (this->ContextItem)
  {
    this->ObserverTag = this->ContextItem->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtk2DWidgetRepresentation::OnContextItemModified);
  }
}

void vtk2DWidgetRepresentation::SetEnabled(bool enabled)
{
  this->Enabled = enabled;
}
