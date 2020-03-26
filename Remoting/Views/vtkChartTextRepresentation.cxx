/*=========================================================================

  Program:   ParaView
  Module:    vtkChartTextRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkChartTextRepresentation.h"

#include "vtkBlockItem.h"
#include "vtkBrush.h"
#include "vtkChart.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVXYChartView.h"
#include "vtkPen.h"
#include "vtkTable.h"
#include "vtkVariant.h"

vtkStandardNewMacro(vtkChartTextRepresentation);
//----------------------------------------------------------------------------
vtkChartTextRepresentation::vtkChartTextRepresentation()
  : Position{ 0, 0 }
  , LabelLocation(vtkChartTextRepresentation::AnyLocation)
{
  // don't render any background.
  this->BlockItem->GetBrush()->SetColorF(1.0, 1.0, 1.0, 0.0);
  this->BlockItem->GetPen()->SetLineType(vtkPen::NO_PEN);

  this->BlockItem->SetInteractive(false);
  this->BlockItem->SetLabel("Testing");
  this->BlockItem->SetDimensions(120, 200, 80, 46);
  this->BlockItem->SetAutoComputeDimensions(true);
  this->BlockItem->SetHorizontalAlignment(vtkBlockItem::CENTER);
  this->BlockItem->AddObserver(
    vtkCommand::InteractionEvent, this, &vtkChartTextRepresentation::OnInteractionEvent);
}

//----------------------------------------------------------------------------
vtkChartTextRepresentation::~vtkChartTextRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkChartTextRepresentation::OnInteractionEvent()
{
  float dims[4];
  this->BlockItem->GetDimensions(dims);
  this->SetPosition(dims[0], dims[1]);
  this->InvokeEvent(vtkCommand::InteractionEvent);
}

//----------------------------------------------------------------------------
void vtkChartTextRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->BlockItem->SetVisible(val);
}

//----------------------------------------------------------------------------
void vtkChartTextRepresentation::SetTextProperty(vtkTextProperty* tp)
{
  this->BlockItem->SetLabelProperties(tp);
}

//----------------------------------------------------------------------------
void vtkChartTextRepresentation::SetInteractivity(bool val)
{
  this->BlockItem->SetInteractive(val);
}

//----------------------------------------------------------------------------
void vtkChartTextRepresentation::SetLabelLocation(int location)
{
  this->LabelLocation = location;
  switch (location)
  {
    case LowerLeftCorner:
      this->BlockItem->SetHorizontalAlignment(vtkBlockItem::LEFT);
      this->BlockItem->SetVerticalAlignment(vtkBlockItem::BOTTOM);
      break;

    case LowerRightCorner:
      this->BlockItem->SetHorizontalAlignment(vtkBlockItem::RIGHT);
      this->BlockItem->SetVerticalAlignment(vtkBlockItem::BOTTOM);
      break;

    case LowerCenter:
      this->BlockItem->SetHorizontalAlignment(vtkBlockItem::CENTER);
      this->BlockItem->SetVerticalAlignment(vtkBlockItem::BOTTOM);
      break;

    case UpperLeftCorner:
      this->BlockItem->SetHorizontalAlignment(vtkBlockItem::LEFT);
      this->BlockItem->SetVerticalAlignment(vtkBlockItem::TOP);
      break;
    case UpperRightCorner:
      this->BlockItem->SetHorizontalAlignment(vtkBlockItem::RIGHT);
      this->BlockItem->SetVerticalAlignment(vtkBlockItem::TOP);
      break;
    case UpperCenter:
      this->BlockItem->SetHorizontalAlignment(vtkBlockItem::CENTER);
      this->BlockItem->SetVerticalAlignment(vtkBlockItem::TOP);
      break;
    case AnyLocation:
    default:
      this->BlockItem->SetHorizontalAlignment(vtkBlockItem::CUSTOM);
      this->BlockItem->SetVerticalAlignment(vtkBlockItem::CUSTOM);
      break;
  }
}

//----------------------------------------------------------------------------
int vtkChartTextRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkChartTextRepresentation::AddToView(vtkView* view)
{
  if (auto chartView = vtkPVContextView::SafeDownCast(view))
  {
    chartView->GetContextView()->GetScene()->AddItem(this->BlockItem);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkChartTextRepresentation::RemoveFromView(vtkView* view)
{
  if (auto chartView = vtkPVContextView::SafeDownCast(view))
  {
    chartView->GetContextView()->GetScene()->RemoveItem(this->BlockItem);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
int vtkChartTextRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->PreparedData->Initialize();

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkTable* input = vtkTable::GetData(inputVector[0], 0);
    if (input->GetNumberOfRows() > 0 && input->GetNumberOfColumns() > 0)
    {
      this->PreparedData->ShallowCopy(input);
    }
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkChartTextRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVContextView::SetPiece(inInfo, this, this->PreparedData);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    auto piece = vtkTable::SafeDownCast(vtkPVContextView::GetDeliveredPiece(inInfo, this));
    if (piece && piece->GetNumberOfColumns() > 0 && piece->GetNumberOfRows() > 0)
    {
      const auto value = piece->GetValue(0, 0);
      this->BlockItem->SetLabel(value.ToString());
    }
    else
    {
      this->BlockItem->SetLabel("[unknown]");
    }
    if (this->LabelLocation == AnyLocation)
    {
      this->BlockItem->SetDimensions(
        static_cast<float>(this->Position[0]), static_cast<float>(this->Position[1]), 10, 10);
    }
    auto pvview = vtkPVContextView::SafeDownCast(this->GetView());
    if (auto chart = (pvview ? vtkChart::SafeDownCast(pvview->GetContextItem()) : nullptr))
    {
      // propagate margins so that the text block is placed inside the axes
      // rendered in vtkChart subclasses.
      vtkRectf chartRect = chart->GetSize();

      int margin[2];
      switch (this->BlockItem->GetHorizontalAlignment())
      {
        case vtkBlockItem::LEFT:
          margin[0] = chart->GetPoint1()[0] - chartRect.GetLeft();
          break;
        case vtkBlockItem::RIGHT:
          margin[0] = chartRect.GetRight() - chart->GetPoint2()[0];
          break;
        default:
          margin[0] = 0;
      }

      switch (this->BlockItem->GetVerticalAlignment())
      {
        case vtkBlockItem::BOTTOM:
          margin[1] = chart->GetPoint1()[1] - chartRect.GetBottom();
          break;
        case vtkBlockItem::TOP:
          margin[1] = chartRect.GetTop() - chart->GetPoint2()[1];
          break;
        default:
          margin[1] = 0;
      }

      this->BlockItem->SetMargins(margin);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkChartTextRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
