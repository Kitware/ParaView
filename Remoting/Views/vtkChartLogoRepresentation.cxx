/*=========================================================================

  Program:   ParaView
  Module:    vtkChartLogoRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkChartLogoRepresentation.h"

#include "vtkChart.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkImageData.h"
#include "vtkImageItem.h"
#include "vtkImageResize.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVXYChartView.h"
#include "vtkPen.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkChartLogoRepresentation);
//----------------------------------------------------------------------------
vtkChartLogoRepresentation::vtkChartLogoRepresentation()
{
  this->ImageItem->AddObserver(
    vtkCommand::InteractionEvent, this, &vtkChartLogoRepresentation::OnInteractionEvent);
}

//----------------------------------------------------------------------------
vtkChartLogoRepresentation::~vtkChartLogoRepresentation() = default;

//----------------------------------------------------------------------------
void vtkChartLogoRepresentation::OnInteractionEvent()
{
  float dims[4];
  this->ImageItem->GetPosition(dims);
  this->SetPosition(dims[0], dims[1]);
  this->InvokeEvent(vtkCommand::InteractionEvent);
}

//----------------------------------------------------------------------------
void vtkChartLogoRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->ImageItem->SetVisible(val);
}

//----------------------------------------------------------------------------
int vtkChartLogoRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkChartLogoRepresentation::AddToView(vtkView* view)
{
  if (auto chartView = vtkPVContextView::SafeDownCast(view))
  {
    chartView->GetContextView()->GetScene()->AddItem(this->ImageItem);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkChartLogoRepresentation::RemoveFromView(vtkView* view)
{
  if (auto chartView = vtkPVContextView::SafeDownCast(view))
  {
    chartView->GetContextView()->GetScene()->RemoveItem(this->ImageItem);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
int vtkChartLogoRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->PreparedData->Initialize();

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
    if (input)
    {
      this->PreparedData->ShallowCopy(input);
    }
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkChartLogoRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVContextView::SetPiece(inInfo, this, this->PreparedData);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    auto piece = vtkImageData::SafeDownCast(vtkPVContextView::GetDeliveredPiece(inInfo, this));
    if (!piece)
    {
      vtkErrorMacro("No image was correctly set");
      return 0;
    }

    this->ImageItem->SetImage(piece);

    auto pvview = vtkPVContextView::SafeDownCast(this->GetView());
    if (auto chart = (pvview ? vtkChart::SafeDownCast(pvview->GetContextItem()) : nullptr))
    {
      int dims[3];
      vtkRectf chartRect = chart->GetSize();
      this->ImageItem->GetImage()->GetDimensions(dims);
      if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
      {
        return 1;
      }

      // Rescale logo
      double logoWidth = 0.075 * chartRect.GetRight();
      double logoHeight = 0.075 * chartRect.GetTop();

      double ratio = static_cast<double>(dims[0]) / static_cast<double>(dims[1]);
      if (ratio > 1)
      {
        dims[0] = logoWidth;
        dims[1] = dims[0] / ratio;
      }
      else
      {
        dims[1] = logoHeight;
        dims[0] = dims[1] * ratio;
      }

      vtkNew<vtkImageResize> resize;
      resize->SetInputData(this->ImageItem->GetImage());
      resize->SetResizeMethod(vtkImageResize::OUTPUT_DIMENSIONS);
      resize->SetOutputDimensions(dims);
      resize->Update();
      this->ImageItem->SetImage(resize->GetOutput(0));

      float position[2] = { 0, 0 };
      if (this->LogoLocation == AnyLocation)
      {
        position[0] = this->Position[0] * chartRect.GetRight();
        position[1] = this->Position[1] * chartRect.GetTop();
      }
      else
      {
        switch (this->LogoLocation)
        {
          case LowerLeftCorner:
            position[0] = chart->GetPoint1()[0] - chartRect.GetLeft();
            position[1] = chart->GetPoint1()[1] - chartRect.GetBottom();
            break;
          case LowerRightCorner:
            position[0] =
              (chart->GetPoint2()[0] - chart->GetPoint1()[0]) - static_cast<float>(dims[0]) / 2;
            position[1] = chart->GetPoint1()[1] - chartRect.GetBottom();
            break;
          case LowerCenter:
            position[0] = static_cast<float>(chart->GetPoint2()[0] - chart->GetPoint1()[0]) / 2 -
              static_cast<float>(dims[0]) / 2;
            position[1] = chart->GetPoint1()[1] - chartRect.GetBottom();
            break;
          case UpperLeftCorner:
            position[0] = chart->GetPoint1()[0] - chartRect.GetLeft();
            position[1] =
              (chart->GetPoint2()[1] - chart->GetPoint1()[1]) - static_cast<float>(dims[1]) / 2;
            break;
          case UpperRightCorner:
            position[0] =
              (chart->GetPoint2()[0] - chart->GetPoint1()[0]) - static_cast<float>(dims[0]) / 2;
            position[1] =
              (chart->GetPoint2()[1] - chart->GetPoint1()[1]) - static_cast<float>(dims[1]) / 2;
            break;
          case UpperCenter:
            position[0] = static_cast<float>(chart->GetPoint2()[0] - chart->GetPoint1()[0]) / 2 -
              static_cast<float>(dims[0]) / 2;
            position[1] =
              (chart->GetPoint2()[1] - chart->GetPoint1()[1]) - static_cast<float>(dims[1]) / 2;
            break;
          case AnyLocation:
            position[0] = this->Position[0];
            position[1] = this->Position[1];
          default:
            break;
        }
      }
      this->ImageItem->SetPosition(position);

      // Also update the Position member variable so calls to GetPosition() return valid info
      this->Position[0] = position[0] / chartRect.GetRight();
      this->Position[1] = position[1] / chartRect.GetTop();
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkChartLogoRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
