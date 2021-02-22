/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarBarActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkPVScalarBarActor.h"
#include "vtkScalarBarActorInternal.h"

#include "vtkAxis.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContextScene.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindow.h"

#include <math.h>

#include <algorithm>
#include <sstream>

#include <stdio.h> // for snprintf

#if defined(_WIN32) && !defined(__CYGWIN__)
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

#define COLOR_TEXTURE_MAP_SIZE 256

#define MY_ABS(x) ((x) < 0 ? -(x) : (x))

//=============================================================================
vtkStandardNewMacro(vtkPVScalarBarActor);

//=============================================================================
vtkPVScalarBarActor::vtkPVScalarBarActor()
{
  this->AspectRatio = 20.0;
  this->AutomaticLabelFormat = 1;
  this->DrawTickMarks = 1;
  this->DrawSubTickMarks = 1;
  this->AddRangeLabels = 1;
  this->RangeLabelFormat = nullptr;
  this->SetRangeLabelFormat("%4.3e");
  this->TitleJustification = VTK_TEXT_CENTERED;
  this->AddRangeAnnotations = 1;
  this->AnnotationTextScaling = 1;
  this->SetVerticalTitleSeparation(4);
  this->AutomaticAnnotations = 0;

  this->ScalarBarTexture = vtkTexture::New();

  this->TickMarks = vtkPolyData::New();
  this->TickMarksMapper = vtkPolyDataMapper2D::New();
  this->TickMarksMapper->SetInputData(this->TickMarks);
  this->TickMarksActor = vtkActor2D::New();
  this->TickMarksActor->SetMapper(this->TickMarksMapper);
  this->TickMarksActor->GetPositionCoordinate()->SetReferenceCoordinate(this->PositionCoordinate);

  this->TickLayoutHelper->SetBehavior(vtkAxis::FIXED);
  this->TickLayoutHelper->SetScene(this->TickLayoutHelperScene.GetPointer());
}

//-----------------------------------------------------------------------------
vtkPVScalarBarActor::~vtkPVScalarBarActor()
{
  this->ScalarBarTexture->Delete();

  this->TickMarks->Delete();
  this->TickMarksMapper->Delete();
  this->TickMarksActor->Delete();

  if (this->ComponentTitle)
  {
    delete[] this->ComponentTitle;
    this->ComponentTitle = nullptr;
  }

  delete[] this->RangeLabelFormat;
  this->RangeLabelFormat = nullptr;
}

//-----------------------------------------------------------------------------
void vtkPVScalarBarActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AspectRatio: " << this->AspectRatio << endl;
  os << indent << "AutomaticLabelFormat: " << this->AutomaticLabelFormat << endl;
  os << indent << "DrawTickMarks: " << this->DrawTickMarks << endl;
  os << indent << "DrawSubTickMarks: " << this->DrawSubTickMarks << endl;
  os << indent << "AddRangeLabels: " << this->AddRangeLabels << endl;
  os << indent
     << "RangeLabelFormat: " << (this->RangeLabelFormat ? this->RangeLabelFormat : "(null)")
     << endl;
  os << indent << "ScalarBarTexture: ";
  if (this->ScalarBarTexture)
  {
    this->ScalarBarTexture->PrintSelf(os << "\n", indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
  os << indent << "TickMarks: ";
  if (this->TickMarks)
  {
    this->TickMarks->PrintSelf(os << "\n", indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
  os << indent << "TickMarksMapper: ";
  if (this->TickMarksMapper)
  {
    this->TickMarksMapper->PrintSelf(os << "\n", indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
  os << indent << "TickMarksActor: ";
  if (this->TickMarksActor)
  {
    this->TickMarksActor->PrintSelf(os << "\n", indent.GetNextIndent());
  }
  else
  {
    os << "(null)\n";
  }
  os << indent << "LabelSpace: " << this->LabelSpace << endl;
  os << indent << "TitleJustification: " << this->TitleJustification << endl;
  os << indent << "AddRangeAnnotations: " << this->AddRangeAnnotations << endl;
  os << indent << "AutomaticAnnotations: " << this->AutomaticAnnotations << endl;
}

//-----------------------------------------------------------------------------
void vtkPVScalarBarActor::ReleaseGraphicsResources(vtkWindow* window)
{
  this->ScalarBarTexture->ReleaseGraphicsResources(window);

  for (unsigned int i = 0; i < this->P->TextActors.size(); i++)
  {
    this->P->TextActors[i]->ReleaseGraphicsResources(window);
  }

  this->TickMarksActor->ReleaseGraphicsResources(window);

  this->Superclass::ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
int vtkPVScalarBarActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  // This ensures that tile scaling can be accounted for in the tick layout:
  if (vtkRenderer* renderer = vtkRenderer::SafeDownCast(viewport))
  {
    this->TickLayoutHelperScene->SetRenderer(renderer);
  }

  return this->Superclass::RenderOpaqueGeometry(viewport);
}

//----------------------------------------------------------------------------
int vtkPVScalarBarActor::RenderOverlay(vtkViewport* viewport)
{
  int renderedSomething = this->Superclass::RenderOverlay(viewport);
  if (this->LookupTable && this->LookupTable->GetIndexedLookup())
  {
    return renderedSomething;
  }

  if (this->DrawTickMarks)
  {
    renderedSomething += this->TickMarksActor->RenderOverlay(viewport);
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkPVScalarBarActor::CreateLabel(
  double value, int minDigits, int targetWidth, int targetHeight, vtkViewport* viewport)
{
  char string[1024];

  vtkNew<vtkTextActor> textActor;
  textActor->GetProperty()->DeepCopy(this->GetProperty());
  textActor->GetPositionCoordinate()->SetReferenceCoordinate(this->PositionCoordinate);

  // Copy the text property here so that the size of the label prop is not
  // affected by the automatic adjustment of its text mapper's size.
  textActor->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
  if (this->P->Viewport)
  {
    textActor->SetTextScaleModeToViewport();
    textActor->ComputeScaledFont(this->P->Viewport);
  }

// One Visual Studio prior 2015, formats with exponents have three digits by default
// whereas on other systems, exponents have two digits. Set to two
// digits on Windows for consistent behavior.
#if defined(_MSC_VER) && (_MSC_VER < 1900)
  unsigned int oldWin32ExponentFormat = _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  if (this->AutomaticLabelFormat)
  {
    // Iterate over all format lengths and find the highest precision that we
    // can represent without going over the target width.  If we cannot fit
    // within the target width, make the smallest possible text.
    int smallestFoundWidth = VTK_INT_MAX;
    bool foundValid = false;
    string[0] = '\0';
    for (int i = 1 + minDigits; i < 6; i++)
    {
      char format[512];
      char string2[1024];
      SNPRINTF(format, 511, "%%-0.%dg", i);
      SNPRINTF(string2, 1023, format, value);

      // we want the reduced size used so that we can get better fitting
      // Extra filter: Used to remove unwanted 0 after e+ or e-
      // i.e.: 1.23e+009 => 1.23e+9
      std::string strToFilter = string2;
      std::string ePlus = "e+0";
      std::string eMinus = "e-0";
      size_t pos = 0;
      while ((pos = strToFilter.find(ePlus)) != std::string::npos ||
        (pos = strToFilter.find(eMinus)) != std::string::npos)
      {
        strToFilter.erase(pos + 2, 1);
      }
      strcpy(string2, strToFilter.c_str());
      textActor->SetInput(string2);

      textActor->SetConstrainedFontSize(viewport, VTK_INT_MAX, targetHeight);
      double tsize[2];
      textActor->GetSize(viewport, tsize);
      if (tsize[0] < targetWidth)
      {
        // Found a string that fits.  Keep it unless we find something better.
        strcpy(string, string2);
        foundValid = true;
      }
      else if ((tsize[0] < smallestFoundWidth) && !foundValid)
      {
        // String does not fit, but it is the smallest so far.
        strcpy(string, string2);
        smallestFoundWidth = tsize[0];
      }
    }
  }
  else
  {
    // Potential of buffer overrun (onto the stack) here.
    SNPRINTF(string, 1023, this->LabelFormat, value);
  }

  // Set the txt label
  textActor->SetInput(string);

#if defined(_MSC_VER) && (_MSC_VER < 1900)
  _set_output_format(oldWin32ExponentFormat);
#endif

  // Size the font to fit in the targetHeight, which we are using
  // to size the font because it is (relatively?) constant.
  int fontSize = textActor->SetConstrainedFontSize(viewport, VTK_INT_MAX, targetHeight);
  int maxFontSize = this->LabelTextProperty->GetFontSize();
  if (fontSize > maxFontSize)
  {
    textActor->GetTextProperty()->SetFontSize(maxFontSize);
  }

  // Make sure that the string fits in the allotted space.
  double tsize[2];
  textActor->GetSize(viewport, tsize);
  if (tsize[0] > targetWidth)
  {
    fontSize = textActor->SetConstrainedFontSize(viewport, targetWidth, targetHeight);
  }

  this->P->TextActors.push_back(textActor.GetPointer());
  return static_cast<int>(this->P->TextActors.size()) - 1;
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::PrepareTitleText()
{
  // Let the superclass prepare the actor:
  this->Superclass::PrepareTitleText();

  // Set font scaling
  if (this->P->Viewport)
  {
    this->TitleActor->ComputeScaledFont(this->P->Viewport);
    this->TitleActor->SetTextScaleModeToViewport();
  }
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::ComputeScalarBarThickness()
{
  double aspectRatio = this->AspectRatio;
  if (aspectRatio <= 0.)
  {
    aspectRatio = 20.;
  }

  // Make the bar's thickness a fraction of its length.
  this->P->ScalarBarBox.Size[0] = static_cast<int>(ceil(this->P->Frame.Size[1] / aspectRatio));
  // Make tick marks half the thickness of the scalar bar (+1 to ensure non-zero size).
  this->LabelSpace = this->P->ScalarBarBox.Size[0] / 2 + 1;

  this->P->ScalarBarBox.Posn = this->P->Frame.Posn;

  // Force opacity to match lookup table
  vtkDiscretizableColorTransferFunction* lut =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
  if (lut)
  {
    this->SetUseOpacity(lut->IsOpaque() ? 0 : 1);
  }
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::LayoutTitle()
{
  if ((this->Title == nullptr || !strlen(this->Title)) &&
    (this->ComponentTitle == nullptr || !strlen(this->ComponentTitle)))
  {
    this->P->TitleBox.Size[0] = this->P->TitleBox.Size[1] = 0;
    return;
  }

  // Reset the text size and justification
  this->TitleActor->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
  this->TitleActor->GetTextProperty()->SetJustification(this->TitleJustification);
  this->TitleActor->GetTextProperty()->SetVerticalJustification(
    this->Orientation == VTK_ORIENT_VERTICAL
      ? VTK_TEXT_BOTTOM
      : (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar ? VTK_TEXT_BOTTOM
                                                                   : VTK_TEXT_TOP));

  double titleSize[2];
  // Get the actual size of the text.
  this->TitleActor->GetSize(this->P->Viewport, titleSize);
  // Now, determine how much space should be reserved for the title and its padding.
  // For the horizontal orientation, the font size is exactly as specified by the user.
  // In the vertical case, we limit the font size so that the remaining box has
  // at least some space (25%) for the scalar bar.
  if (this->Orientation == VTK_ORIENT_VERTICAL &&
    (1.5 * titleSize[1] + 3 * this->TextPad > 0.75 * this->P->Frame.Size[1]))
  { // title takes up 3/4 or more of the frame... better reduce font size
    this->TitleActor->SetConstrainedFontSize(
      this->P->Viewport, VTK_INT_MAX, 0.5 * this->P->Frame.Size[1] - 3 * this->TextPad);
    this->TitleActor->GetSize(this->P->Viewport, titleSize);
  }
  this->P->TitleBox.Size[this->P->TL[0]] = titleSize[0];
  this->P->TitleBox.Size[this->P->TL[1]] = 1.5 * (titleSize[1] + this->TextPad);

  // Position the title.
  switch (this->TitleActor->GetTextProperty()->GetJustification())
  {
    case VTK_TEXT_LEFT:
      this->P->TitleBox.Posn[0] = this->P->Frame.Posn[0] + this->TextPad;
      break;
    case VTK_TEXT_RIGHT:
      this->P->TitleBox.Posn[0] =
        this->P->Frame.Posn[0] + this->P->Frame.Size[this->P->TL[0]] - this->TextPad - titleSize[0];
      break;
    case VTK_TEXT_CENTERED:
    default:
      this->P->TitleBox.Posn[0] =
        this->P->Frame.Posn[0] + (this->P->Frame.Size[this->P->TL[0]] - titleSize[0]) / 2;
      break;
  }
  if (this->Orientation == VTK_ORIENT_VERTICAL)
  { // The title is stacked above the scalar bar.
    this->P->TitleBox.Posn[1] = this->P->Frame.Posn[1] + this->P->Frame.Size[this->P->TL[1]] -
      this->P->TitleBox.Size[this->P->TL[1]] / 1.5;
    this->P->ScalarBarBox.Size[this->P->TL[1]] -= 2 * this->TextPad;
  }
  else // VTK_ORIENT_HORIZONTAL
  {
    // The title is above or below the ticks,
    // which either precede or succeed the scalar bar.
    // We don't know the tick size yet, but we can push the title or scalar
    // bar away from the frame's origin as required. Then LayoutTicks can
    // further adjust positions.
    if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
    {
      this->P->TitleBox.Posn[1] = this->P->Frame.Posn[1];
      // push the scalar bar up by the title height.
      this->P->ScalarBarBox.Posn[1] += this->P->TitleBox.Size[0] + this->TextPad;
      this->P->NanBox.Posn[1] += this->TextPad + this->P->TitleBox.Size[0];
    }
    else
    {
      this->P->TitleBox.Posn[1] =
        this->P->Frame.Posn[1] + this->P->ScalarBarBox.Size[0] + this->TextPad;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::ComputeScalarBarLength()
{
  this->Superclass::ComputeScalarBarLength();
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::LayoutTicks()
{
  if (this->LookupTable->GetIndexedLookup())
  { // no tick marks in indexed lookup mode.
    this->NumberOfLabelsBuilt = 0;
    return;
  }

  // Figure out the precision to use based on the width of the scalar bar.
  vtkNew<vtkTextActor> dummyActor;
  dummyActor->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
  dummyActor->SetInput("()"); // parentheses are taller than numbers for all useful fonts.
  if (this->P->Viewport)
  {
    dummyActor->SetTextScaleModeToViewport();
    dummyActor->ComputeScaledFont(this->P->Viewport);
  }

  double tsize[2];
  dummyActor->GetSize(this->P->Viewport, tsize);
  // Now constrain the width of the text with our target height:
  int targetHeight = static_cast<int>(ceil(tsize[1]));

  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    this->P->TickBox.Size[0] = std::max(0, this->P->Frame.Size[0] - this->P->ScalarBarBox.Size[0]);
    this->P->TickBox.Size[1] = this->P->ScalarBarBox.Size[1];
    this->P->TickBox.Posn = this->P->ScalarBarBox.Posn;
    if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
    {
      this->P->ScalarBarBox.Posn[0] += this->P->TickBox.Size[0];
      this->P->NanBox.Posn[0] += this->P->TickBox.Size[0];
    }
    else
    {
      this->P->TickBox.Posn[0] += this->P->ScalarBarBox.Size[0];
    }

    /* FYI, this is how height is used in ConfigureTicks:
    int maxHeight = static_cast<int>(
      this->P->ScalarBarBox.Size[1] / this->NumberOfLabels);
    targetHeight = std::min(targetHeight, maxHeight);
    */
  }
  else
  {
    this->P->TickBox.Size[0] = this->LabelSpace + 2 + targetHeight;
    this->P->TickBox.Size[1] = this->P->ScalarBarBox.Size[1];
    if (this->TextPosition == PrecedeScalarBar)
    { // Push scalar bar and NaN swatch up to make room for ticks.
      this->P->TickBox.Posn = this->P->ScalarBarBox.Posn;
      this->P->ScalarBarBox.Posn[1] += this->P->TickBox.Size[0];
      this->P->NanBox.Posn[1] += this->P->TickBox.Size[0];
    }
    else
    {
      this->P->TickBox.Posn[0] = this->P->ScalarBarBox.Posn[0];
      this->P->TickBox.Posn[1] = this->P->TitleBox.Posn[1];
      this->P->TitleBox.Posn[1] += this->P->TickBox.Size[0];
    }
  }
}

void vtkPVScalarBarActor::ConfigureAnnotations()
{
  // Work around an issue in VTK.
  if (this->P->Labels.empty())
  {
    this->P->AnnotationBoxes->Initialize();
  }
  this->Superclass::ConfigureAnnotations();
}

void vtkPVScalarBarActor::ConfigureTitle()
{
  // This adjustment is necessary because this->TitleActor adjusts
  // where the text is rendered relative to the position of the
  // TitleActor based on the justification set on the TitleActor's
  // TextProperty.
  double texturePolyDataBounds[6];
  this->TexturePolyData->GetBounds(texturePolyDataBounds);

  double scalarBarMinX = texturePolyDataBounds[0];
  double scalarBarMaxX = texturePolyDataBounds[1];
  double x;
  switch (this->TitleActor->GetTextProperty()->GetJustification())
  {
    case VTK_TEXT_LEFT:
      if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
        x = scalarBarMaxX;
        x -= this->P->TitleBox.Size[this->P->TL[0]];
      }
      else
      {
        x = scalarBarMinX;
      }
      break;
    case VTK_TEXT_RIGHT:
      if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
        x = scalarBarMinX;
        x += this->P->TitleBox.Size[this->P->TL[0]];
      }
      else
      {
        x = scalarBarMaxX;
      }
      break;
    case VTK_TEXT_CENTERED:
      x = 0.5 * (scalarBarMinX + scalarBarMaxX);
      break;
    default:
      x = 0.;
      vtkErrorMacro(<< "Invalid text justification ("
                    << this->TitleActor->GetTextProperty()->GetJustification()
                    << " for scalar bar title.");
      break;
  }

  double y = this->TitleActor->GetTextProperty()->GetVerticalJustification() == VTK_TEXT_BOTTOM
    ? this->P->TitleBox.Posn[1]
    : this->P->TitleBox.Posn[1] + this->P->TitleBox.Size[this->P->TL[1]];

  this->TitleActor->SetPosition(x, y);
}

//----------------------------------------------------------------------------
void vtkPVScalarBarActor::ConfigureTicks()
{
  bool isLogTable = (this->LookupTable->UsingLogScale() != 0);
  double* range = this->LookupTable->GetRange();

  if (this->LookupTable->GetIndexedLookup())
  { // no tick marks in indexed lookup mode.
    return;
  }

  // Use vtkAxis to compute tick locations:
  vtkAxis::Location loc;
  vtkVector2f p1(this->P->TickBox.Posn.Cast<float>().GetData());
  vtkVector2f p2 = p1;
  if (this->Orientation == VTK_ORIENT_HORIZONTAL)
  {
    p2[0] += static_cast<float>(this->P->TickBox.Size[1]);
    loc = vtkAxis::BOTTOM;
  }
  else
  {
    p2[1] += static_cast<float>(this->P->TickBox.Size[1]);
    loc = vtkAxis::LEFT;
  }

  this->TickLayoutHelper->SetPosition(static_cast<int>(loc));
  this->TickLayoutHelper->SetPoint1(p1);
  this->TickLayoutHelper->SetPoint2(p2);
  this->TickLayoutHelper->SetLogScale(isLogTable);
  this->TickLayoutHelper->SetUnscaledRange(range);
  this->TickLayoutHelper->SetNumberOfTicks(this->NumberOfLabels);
  this->TickLayoutHelper->Update();

  vtkDoubleArray* tickArray = this->TickLayoutHelper->GetTickPositions();
  vtkStringArray* labelArray = this->TickLayoutHelper->GetTickLabels();

  // Process and filter the ticks:
  std::vector<double> ticks;
  ticks.reserve(tickArray->GetNumberOfTuples());
  for (vtkIdType i = 0; i < tickArray->GetNumberOfTuples(); ++i)
  {
    // The label will be an empty string if it's not supposed to be labeled.
    // Filter these out.
    if (labelArray->GetValue(i).empty())
    {
      continue;
    }

    ticks.push_back(isLogTable ? std::pow(10.0, tickArray->GetValue(i)) : tickArray->GetValue(i));
  }

  // minimum number of digits (base 10) to differentiate between tick marks.
  int minDigits = 1;

  // Compute the difference between min and max of scalar bar values.
  double delta = range[1] - range[0];
  if (delta != 0)
  {
    // See what digit of the decimal number the difference is contained in.
    double dmag = log10(delta);
    double emag = floor(dmag) - 1;
    double mag = pow(10.0, emag);
    if (1.1 * mag > delta)
    {
      mag /= 10.0;
    }
    int leastDig = floor(log10(fabs(mag)));

    // Find the the maximum number of digits in the range:
    double absMax = std::max(std::fabs(range[0]), std::fabs(range[1]));
    int leadDig = floor(log10(absMax));

    minDigits = leadDig - leastDig;
  }

  // Map from tick to label ID for tick
  std::vector<int> tickToLabelId(ticks.size(), -1);

  this->P->TextActors.reserve(ticks.size());

  vtkNew<vtkCellArray> tickCells;
  tickCells->Allocate(tickCells->EstimateSize(ticks.size() * 10, 2));

  vtkNew<vtkPoints> tickPoints;
  tickPoints->Allocate(ticks.size() * 20);

  double targetWidth = this->P->TickBox.Size[this->P->TL[0]];
  double targetHeight = this->P->TickBox.Size[this->P->TL[1]];
  if (this->Orientation == VTK_ORIENT_HORIZONTAL)
  {
    targetWidth = (targetWidth - (ticks.size() - 1) * this->TextPad) / (ticks.size() + 1.);
  }
  else // VTK_ORIENT_VERTICAL
  {
    targetHeight = (targetHeight - (ticks.size() - 1) * this->TextPad) / (ticks.size() + 1.);
    // Get the target height based on the selected font size. Set this
    // as the target height
    // int desiredFontSize = this->LabelTextProperty->GetFontSize();
    vtkNew<vtkTextActor> dummyActor;
    dummyActor->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
    dummyActor->SetInput("()"); // parentheses are taller than numbers for all useful fonts.
    if (this->P->Viewport)
    {
      dummyActor->SetTextScaleModeToViewport();
      dummyActor->ComputeScaledFont(this->P->Viewport);
    }

    double tsize[2];
    dummyActor->GetSize(this->P->Viewport, tsize);

    if (tsize[1] < targetHeight)
    {
      targetHeight = tsize[1];
    }
  }

  bool precede = this->TextPosition == vtkScalarBarActor::PrecedeScalarBar;
  int minimumFontSize = VTK_INT_MAX;
  for (int i = 0; i < static_cast<int>(ticks.size()); i++)
  {
    double val = ticks[i];

    // Do not create the label if it is already represented in the min or max
    // label.
    if (!((val - 1e-6 * MY_ABS(val + range[0]) > range[0]) &&
          (val + 1e-6 * MY_ABS(val + range[1]) < range[1])))
    {
      continue;
    }

    int labelIdx = this->CreateLabel(val, minDigits, targetWidth, targetHeight, this->P->Viewport);
    tickToLabelId[i] = labelIdx;
    vtkTextActor* textActor = this->P->TextActors[labelIdx];

    int labelFontSize = textActor->GetTextProperty()->GetFontSize();
    if (labelFontSize < minimumFontSize)
    {
      minimumFontSize = labelFontSize;
    }
  }

  // Now place the label actors
  for (size_t i = 0; i < ticks.size(); i++)
  {
    int labelIdx = tickToLabelId[i];
    if (labelIdx == -1)
    {
      // No label
      continue;
    }
    vtkTextActor* textActor = this->P->TextActors[labelIdx];

    // Make sure every text actor gets the smallest text size to fit
    // the constraints.
    textActor->GetTextProperty()->SetFontSize(minimumFontSize);

    double val = ticks[i];

    double normVal;
    if (isLogTable)
    {
      normVal = ((log10(val) - log10(range[0])) / (log10(range[1]) - log10(range[0])));
    }
    else
    {
      normVal = (val - range[0]) / (range[1] - range[0]);
    }

    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      double x =
        precede ? this->P->TickBox.Posn[0] + this->P->TickBox.Size[0] : this->P->TickBox.Posn[0];
      double y = normVal * this->P->TickBox.Size[1] + this->P->TickBox.Posn[1];
      double textSize[2];
      textActor->GetSize(this->P->Viewport, textSize);
      y -= textSize[1] / 2; // Adjust to center text.
      textActor->GetTextProperty()->SetJustification(precede ? VTK_TEXT_RIGHT : VTK_TEXT_LEFT);
      textActor->SetPosition(precede ? x - this->LabelSpace : x + this->LabelSpace, y);
    }
    else // this->Orientation == VTK_ORIENT_HORIZONTAL
    {
      double x = this->P->TickBox.Posn[0] + normVal * this->P->TickBox.Size[1];
      double y =
        precede ? this->P->TickBox.Posn[1] + this->P->TickBox.Size[0] : this->P->TickBox.Posn[1];
      textActor->GetTextProperty()->SetJustificationToCentered();
      textActor->GetTextProperty()->SetVerticalJustification(
        precede ? VTK_TEXT_TOP : VTK_TEXT_BOTTOM);
      textActor->SetPosition(x, precede ? y - this->LabelSpace : y + this->LabelSpace);
    }
  }

  // Create minor tick marks.
  int numTicks = static_cast<int>(ticks.size());
  if (numTicks > 1)
  {
    // Decide how many (maximum) minor ticks we want based on how many pixels
    // are available.
    double fractionOfRange;
    if (isLogTable)
    {
      double tickDelta;
      tickDelta = log10(ticks[numTicks - 1]) - log10(ticks[0]);
      fractionOfRange = (tickDelta) / (log10(range[1]) - log10(range[0]));
    }
    else
    {
      double tickDelta;
      tickDelta = ticks[numTicks - 1] - ticks[0];
      fractionOfRange = (tickDelta) / (range[1] - range[0]);
    }

    double pixelsAvailable = fractionOfRange * this->P->TickBox.Size[1];
    int maxNumMinorTicks = vtkMath::Floor(pixelsAvailable / 5);

    // This array lists valid minor to major tick ratios.
    const int minorRatios[] = { 10, 5, 2, 1 };
    const int numMinorRatios = static_cast<int>(sizeof(minorRatios) / sizeof(int));
    int minorRatio = 0;
    for (int r = 0; r < numMinorRatios; r++)
    {
      minorRatio = minorRatios[r];
      int numMinorTicks = (numTicks - 1) * minorRatio;
      if (numMinorTicks <= maxNumMinorTicks)
        break;
    }

    // Add "fake" major ticks so that the minor ticks extend to bar ranges.
    double fakeMin, fakeMax;
    if (!isLogTable)
    {
      fakeMin = 2 * ticks[0] - ticks[1];
      fakeMax = 2 * ticks[numTicks - 1] - ticks[numTicks - 2];
    }
    else
    {
      fakeMin = pow(10.0, 2 * log10(ticks[0]) - log10(ticks[1]));
      fakeMax = pow(10.0, 2 * log10(ticks[numTicks - 1]) - log10(ticks[numTicks - 2]));
    }
    ticks.insert(ticks.begin(), fakeMin);
    ticks.insert(ticks.end(), fakeMax);
    numTicks = static_cast<int>(ticks.size());

    for (int i = 0; this->DrawSubTickMarks && i < numTicks - 1; i++)
    {
      double minorTickRange[2];
      minorTickRange[0] = ticks[i];
      minorTickRange[1] = ticks[i + 1];
      for (int j = 0; j < minorRatio; j++)
      {
        double val =
          (((minorTickRange[1] - minorTickRange[0]) * j) / minorRatio + minorTickRange[0]);
        double normVal;
        if (isLogTable)
        {
          normVal = ((log10(val) - log10(range[0])) / (log10(range[1]) - log10(range[0])));
        }
        else
        {
          normVal = (val - range[0]) / (range[1] - range[0]);
        }

        // Do not draw ticks out of range.
        if ((normVal < 0.0) || (normVal > 1.0))
          continue;

        if (this->Orientation == VTK_ORIENT_VERTICAL)
        {
          double x = precede ? this->P->TickBox.Posn[0] + this->P->TickBox.Size[0]
                             : this->P->TickBox.Posn[0];
          double y = normVal * this->P->TickBox.Size[1] + this->P->TickBox.Posn[1];
          vtkIdType ids[2];
          ids[0] = tickPoints->InsertNextPoint(precede ? x - this->LabelSpace + 2 : x - 2, y, 0.0);
          ids[1] = tickPoints->InsertNextPoint(precede ? x + 2 : x + this->LabelSpace - 2, y, 0.0);
          tickCells->InsertNextCell(2, ids);
        }
        else // ths->Orientation == VTK_ORIENT_HORIZONTAL
        {
          double x = this->P->TickBox.Posn[0] + normVal * this->P->TickBox.Size[1];
          double y = precede ? this->P->TickBox.Posn[1] + this->P->TickBox.Size[0]
                             : this->P->TickBox.Posn[1];
          vtkIdType ids[2];
          ids[0] = tickPoints->InsertNextPoint(x, precede ? y + 2 : y + this->LabelSpace - 2, 0.0);
          ids[1] = tickPoints->InsertNextPoint(x, precede ? y - this->LabelSpace + 2 : y - 2, 0.0);
          tickCells->InsertNextCell(2, ids);
        }
      }
    }
  }

  this->TickMarks->SetLines(tickCells.GetPointer());
  this->TickMarks->SetPoints(tickPoints.GetPointer());

  // "Mute" the color of the tick marks.
  double color[3];
  // this->TickMarksActor->GetProperty()->GetColor(color);
  this->LabelTextProperty->GetColor(color);
  vtkMath::RGBToHSV(color, color);
  if (color[2] > 0.5)
  {
    color[2] -= 0.2;
  }
  else
  {
    color[2] += 0.2;
  }
  vtkMath::HSVToRGB(color, color);
  this->TickMarksActor->GetProperty()->SetColor(color);

  if (this->AddRangeLabels)
  {

    // Save state and set preferred parameters
    int previousAutomaticLabelFormat = this->GetAutomaticLabelFormat();
    this->SetAutomaticLabelFormat(0);

    std::string previousLabelFormat(this->GetLabelFormat());
    this->SetLabelFormat(this->RangeLabelFormat);

    this->CreateLabel(range[0], minDigits, targetWidth, targetHeight, this->P->Viewport);
    this->CreateLabel(range[1], minDigits, targetWidth, targetHeight, this->P->Viewport);

    // Restore state
    this->AutomaticLabelFormat = previousAutomaticLabelFormat;
    this->SetLabelFormat(previousLabelFormat.c_str());

    // Now change the font size of the min/max text actors to the minimum
    // font size of all the text actors.
    for (size_t i = this->P->TextActors.size() - 2; i < this->P->TextActors.size(); ++i)
    {
      vtkTextActor* textActor = this->P->TextActors[i];

      // Keep min/max labels the same size as the rest of the labels
      textActor->GetTextProperty()->SetFontSize(minimumFontSize);

      double val = range[i - (this->P->TextActors.size() - 2)];

      double normVal;
      if (isLogTable)
      {
        normVal = ((log10(val) - log10(range[0])) / (log10(range[1]) - log10(range[0])));
      }
      else
      {
        normVal = (val - range[0]) / (range[1] - range[0]);
      }

      if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
        double x =
          precede ? this->P->TickBox.Posn[0] + this->P->TickBox.Size[0] : this->P->TickBox.Posn[0];
        double y = normVal * this->P->TickBox.Size[1] + this->P->TickBox.Posn[1];
        double textSize[2];
        textActor->GetSize(this->P->Viewport, textSize);
        y -= textSize[1] / 2; // Adjust to center text.
        textActor->GetTextProperty()->SetJustification(precede ? VTK_TEXT_RIGHT : VTK_TEXT_LEFT);
        textActor->SetPosition(precede ? x - this->LabelSpace : x + this->LabelSpace, y);
      }
      else // this->Orientation == VTK_ORIENT_HORIZONTAL
      {
        double x = this->P->TickBox.Posn[0] + normVal * this->P->TickBox.Size[1];
        double y =
          precede ? this->P->TickBox.Posn[1] + this->P->TickBox.Size[0] : this->P->TickBox.Posn[1];
        textActor->GetTextProperty()->SetJustificationToCentered();
        textActor->GetTextProperty()->SetVerticalJustification(
          precede ? VTK_TEXT_TOP : VTK_TEXT_BOTTOM);
        textActor->SetPosition(x, precede ? y - this->LabelSpace : y + this->LabelSpace);
      }

      // Turn off visibility of any labels that overlap the min/max labels
      double bbox[4];
      textActor->GetBoundingBox(this->P->Viewport, bbox);
      double* pos = textActor->GetPosition();
      bbox[0] += pos[0];
      bbox[1] += pos[0];
      bbox[2] += pos[1];
      bbox[3] += pos[1];

      for (size_t j = 0; j < tickToLabelId.size(); ++j)
      {
        int labelIdx = tickToLabelId[j];
        if (labelIdx == -1)
        {
          // No label
          continue;
        }
        vtkTextActor* labelActor = this->P->TextActors[labelIdx];

        double labelbbox[4];
        double* labelpos;
        labelActor->GetBoundingBox(this->P->Viewport, labelbbox);
        labelpos = labelActor->GetPosition();
        labelbbox[0] += labelpos[0];
        labelbbox[1] += labelpos[0];
        labelbbox[2] += labelpos[1];
        labelbbox[3] += labelpos[1];

        // Does label bounding box intersect min/max label bounding box?
        bool xoverlap = !((labelbbox[0] < bbox[0] && labelbbox[1] < bbox[0]) ||
          (labelbbox[0] > bbox[1] && labelbbox[1] > bbox[1]));
        bool yoverlap = !((labelbbox[2] < bbox[2] && labelbbox[3] < bbox[2]) ||
          (labelbbox[2] > bbox[3] && labelbbox[3] > bbox[3]));
        if (xoverlap && yoverlap)
        {
          labelActor->SetVisibility(0);
        }
      }
    }
  }

  // Loop range accounts for "fake" min max ticks
  for (size_t i = 1; ticks.size() > 0 && i < ticks.size() - 1; i++)
  {
    int labelIdx = tickToLabelId[i - 1];
    if (labelIdx == -1)
    {
      // No label
      continue;
    }
    vtkTextActor* textActor = this->P->TextActors[labelIdx];
    if (textActor->GetVisibility() == 0)
    {
      continue;
    }

    double val = ticks[i];

    double normVal;
    if (isLogTable)
    {
      normVal = ((log10(val) - log10(range[0])) / (log10(range[1]) - log10(range[0])));
    }
    else
    {
      normVal = (val - range[0]) / (range[1] - range[0]);
    }

    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      double x =
        precede ? this->P->TickBox.Posn[0] + this->P->TickBox.Size[0] : this->P->TickBox.Posn[0];
      double y = normVal * this->P->TickBox.Size[1] + this->P->TickBox.Posn[1];
      vtkIdType ids[2];
      ids[0] = tickPoints->InsertNextPoint(x - this->LabelSpace + 2, y, 0.0);
      ids[1] = tickPoints->InsertNextPoint(x + this->LabelSpace - 2, y, 0.0);
      tickCells->InsertNextCell(2, ids);
    }
    else // this->Orientation == VTK_ORIENT_HORIZONTAL
    {
      double x = this->P->TickBox.Posn[0] + normVal * this->P->TickBox.Size[1];
      double y =
        precede ? this->P->TickBox.Posn[1] + this->P->TickBox.Size[0] : this->P->TickBox.Posn[1];
      vtkIdType ids[2];
      ids[0] = tickPoints->InsertNextPoint(x, y - this->LabelSpace + 2, 0.0);
      ids[1] = tickPoints->InsertNextPoint(x, y + this->LabelSpace - 2, 0.0);
      tickCells->InsertNextCell(2, ids);
    }
  }
}

//----------------------------------------------------------------------------
namespace
{
static void AddLabelIfUnoccluded(double x, const vtkColor3ub& color, const std::string& label,
  vtkScalarBarActorInternal* scalarBar)
{
  // Don't place the label if the nearest existing labels are within 1% of
  // the screen-space size of the scalar bar.
  bool okLo = true, okHi = true;
  if (!scalarBar->Labels.empty())
  {
    double delta = scalarBar->ScalarBarBox.Size[1] / 100.;
    std::map<double, vtkStdString>::iterator clo = scalarBar->Labels.lower_bound(x);

    if (clo == scalarBar->Labels.end())
    { // nothing bounds above, just check below:
      okLo = (x - scalarBar->Labels.rbegin()->first) > delta;
    }
    else
    {
      okHi = (clo->first - x) > delta;
      if (clo != scalarBar->Labels.begin())
      { // something bounds below
        --clo;
        okLo = (x - clo->first) > delta;
      }
    }
  }
  if (okLo && okHi)
  {
    scalarBar->Labels[x] = label;
    scalarBar->LabelColors[x] = color;
  }
}
}

//-----------------------------------------------------------------------------
void vtkPVScalarBarActor::EditAnnotations()
{
  vtkScalarsToColors* lut = this->LookupTable;
  if (lut && !lut->GetIndexedLookup())
  {
    const double* range = lut->GetRange();
    double dr = range[1] - range[0];
    double minX = this->P->ScalarBarBox.Posn[this->P->TL[1]];
    double maxX = this->P->ScalarBarBox.Size[1] + minX;

    // Add annotations with min and max values
    if (this->AddRangeAnnotations)
    {
      this->AddValueLabelIfUnoccluded(range[0], minX, dr);
      this->AddValueLabelIfUnoccluded(range[1], maxX, dr);
    }

    // Add annotations with corresponding values between discretized colors
    vtkDiscretizableColorTransferFunction* transferFunc =
      vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
    vtkIdType nbValues = lut->GetNumberOfAvailableColors();
    if (transferFunc && transferFunc->GetDiscretize() && this->AutomaticAnnotations && nbValues)
    {
      double step = dr / nbValues;
      double stepPos = (maxX - minX) / nbValues;
      for (vtkIdType i = 0; i <= nbValues; i++)
      {
        double value = range[0] + step * i;
        double pos = minX + stepPos * i;
        this->AddValueLabelIfUnoccluded(value, pos, value - range[0]);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPVScalarBarActor::AddValueLabelIfUnoccluded(double value, double pos, double /*diff*/)
{
  double lVal = log10(value);
  // The least significant digit
  int least = (lVal > 0 ? +1 : -1) * static_cast<int>(floor(fabs(lVal)));
  // The most significant digit
  int most = static_cast<int>(ceil(log10(fabs(value))));
  // How many digits of precision are required
  int dig = (most == VTK_INT_MIN) ? 3 : (most - least);
  // Label
  char label[64], fmt[64];
  SNPRINTF(fmt, 63, "%%.%dg", dig < 3 ? 3 : dig);
  SNPRINTF(label, 63, fmt, value);
  vtkColor4d fltCol;
  vtkColor3ub col;
  this->LookupTable->GetColor(value, fltCol.GetData());
  for (int j = 0; j < 3; ++j)
  {
    col.GetData()[j] = static_cast<unsigned char>(fltCol.GetData()[j] * 255.);
  }
  AddLabelIfUnoccluded(pos, col, label, this->P);
}

//-----------------------------------------------------------------------------
void vtkPVScalarBarActor::BuildScalarBarTexture()
{
  vtkNew<vtkFloatArray> tmp;
  tmp->SetNumberOfTuples(COLOR_TEXTURE_MAP_SIZE);
  double* range = this->LookupTable->GetRange();
  int isLogTable = this->LookupTable->UsingLogScale();
  for (int i = 0; i < COLOR_TEXTURE_MAP_SIZE; i++)
  {
    double normVal = (double)i / (COLOR_TEXTURE_MAP_SIZE - 1);
    double val;
    if (isLogTable)
    {
      double lval = log10(range[0]) + normVal * (log10(range[1]) - log10(range[0]));
      val = pow(10.0, lval);
    }
    else
    {
      val = (range[1] - range[0]) * normVal + range[0];
    }
    tmp->SetValue(i, val);
  }
  vtkNew<vtkImageData> colorMapImage;
  colorMapImage->SetExtent(0, COLOR_TEXTURE_MAP_SIZE - 1, 0, 0, 0, 0);
  colorMapImage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  vtkDataArray* colors =
    this->LookupTable->MapScalars(tmp.GetPointer(), VTK_COLOR_MODE_MAP_SCALARS, 0);
  colorMapImage->GetPointData()->SetScalars(colors);
  colors->Delete();

  this->ScalarBarTexture->SetInputData(colorMapImage.GetPointer());
}
