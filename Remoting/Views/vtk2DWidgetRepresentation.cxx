// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtk2DWidgetRepresentation.h"

#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtk2DWidgetRepresentation);

//------------------------------------------------------------------------------
vtk2DWidgetRepresentation::vtk2DWidgetRepresentation()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtk2DWidgetRepresentation::~vtk2DWidgetRepresentation()
{
  if (this->View && this->ContextItem)
  {
    this->View->GetContextView()->GetScene()->RemoveItem(this->ContextItem);
  }
}

//------------------------------------------------------------------------------
bool vtk2DWidgetRepresentation::AddToView(vtkView* view)
{
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(view);
  if (pvview)
  {
    this->View = pvview;
    vtkContextScene* scene = this->View->GetContextView()->GetScene();

    // let's find and apply transforms to the widget if needed
    auto itemsCount = scene->GetNumberOfItems();
    for (unsigned int i = 0; i < itemsCount; ++i)
    {
      vtkAbstractContextItem* item = scene->GetItem(i);
      if (vtkChartXY* chart = vtkChartXY::SafeDownCast(item))
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

//------------------------------------------------------------------------------
bool vtk2DWidgetRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(view);
  if (pvview)
  {
    vtkContextScene* scene = this->View->GetContextView()->GetScene();
    scene->RemoveItem(this->ContextItem);
    this->View = nullptr;

    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtk2DWidgetRepresentation::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled:" << this->Enabled << std::endl;
  os << indent << "View:";
  if (this->View)
  {
    os << std::endl;
    this->View->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr" << std::endl;
  }
  os << indent << "ContextItem:";
  if (this->ContextItem)
  {
    os << std::endl;
    this->ContextItem->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr" << std::endl;
  }
}
