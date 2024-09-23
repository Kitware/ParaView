// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkContext2DTexturedScalarBarActor.h"

#include "vtkAxis.h"
#include "vtkBoundingRectContextDevice2D.h"
#include "vtkBrush.h"
#include "vtkColorTransferFunction.h"
#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextDevice2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkTextProperty.h"
#include "vtkTransform2D.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"

#include "vtkNetworkImageSource.h"
#include "vtkPVLogoSource.h"

#include <limits>
#include <map>

#if defined(_WIN32) && !defined(__CYGWIN__)
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

/**
 * NOTE FOR DEVELOPERS
 * The color bar is defined so that the origin (0, 0) is the bottom left
 * corner of the region that contains the scalar bar.
 *
 * This class and vtkContext2DScalarBarActor could be refactored by creating
 * a superclass generifying the logic shared between scalar bar actors relying
 * on the vtkContext2D API.
 */

namespace
{
//----------------------------------------------------------------------------
vtkRectf GetColorBarRect(double size)
{
  return vtkRectf(0, 0, size, size);
}

//----------------------------------------------------------------------------
void CopyTextProperties(vtkTextProperty* fromProperty, vtkTextProperty* toProperty)
{
  toProperty->SetColor(fromProperty->GetColor());
  toProperty->SetOpacity(fromProperty->GetOpacity());
  toProperty->SetBackgroundColor(fromProperty->GetBackgroundColor());
  toProperty->SetBackgroundOpacity(fromProperty->GetBackgroundOpacity());
  toProperty->SetFontFamilyAsString(fromProperty->GetFontFamilyAsString());
  toProperty->SetFontFile(fromProperty->GetFontFile());
  toProperty->SetFontSize(fromProperty->GetFontSize());
}
}

// This class is a vtkContextItem that can be added to a vtkContextScene.
//----------------------------------------------------------------------------
class vtkContext2DTexturedScalarBarActor::vtkScalarBarItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkScalarBarItem, vtkContextItem);

  static vtkScalarBarItem* New() { VTK_OBJECT_FACTORY_NEW_BODY(vtkScalarBarItem); }

  // Forward calls to vtkContextItem::Paint to vtkContext2DTexturedScalarBarActor
  bool Paint(vtkContext2D* painter) override
  {
    bool somethingRendered = false;
    if (this->Actor)
    {
      somethingRendered = this->Actor->Paint(painter);
    }

    return somethingRendered && this->Superclass::Paint(painter);
  }

  // Reference to the Actor.
  vtkContext2DTexturedScalarBarActor* Actor = nullptr;

protected:
  vtkScalarBarItem()
    : Actor(nullptr)
  {
  }
  ~vtkScalarBarItem() override = default;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContext2DTexturedScalarBarActor);

//----------------------------------------------------------------------------
vtkContext2DTexturedScalarBarActor::vtkContext2DTexturedScalarBarActor()
{
  // Declared in superclass
  this->CustomLabels = vtkDoubleArray::New();

  // Setup internal ressources
  this->ScalarBarItem->Actor = this;

  vtkNew<vtkContextScene> localScene;
  this->ActorDelegate->SetScene(localScene);
  localScene->AddItem(this->ScalarBarItem);

  this->Axis->SetScene(localScene);
}

//----------------------------------------------------------------------------
vtkContext2DTexturedScalarBarActor::~vtkContext2DTexturedScalarBarActor() = default;

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::SetNumberOfCustomLabels(vtkIdType numLabels)
{
  this->CustomLabels->SetNumberOfTuples(numLabels);
}

//----------------------------------------------------------------------------
vtkIdType vtkContext2DTexturedScalarBarActor::GetNumberOfCustomLabels()
{
  return this->CustomLabels->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::SetCustomLabel(vtkIdType index, double value)
{
  if (index < 0 || index >= this->CustomLabels->GetNumberOfTuples())
  {
    vtkErrorMacro(<< index << " : index out of range");
    return;
  }

  this->CustomLabels->SetTypedTuple(index, &value);
}

//----------------------------------------------------------------------------
int vtkContext2DTexturedScalarBarActor::RenderOverlay(vtkViewport* viewport)
{
  this->CurrentViewport = viewport;
  this->CurrentBoundingRect = this->GetBoundingRect();
  return this->ActorDelegate->RenderOverlay(viewport);
}

//----------------------------------------------------------------------------
int vtkContext2DTexturedScalarBarActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->CurrentViewport = viewport;
  return 1;
}

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::ReleaseGraphicsResources(vtkWindow* window)
{
  if (!this->ActorDelegate->GetScene() || !this->ActorDelegate->GetScene()->GetLastPainter())
  {
    return;
  }

  vtkContextDevice2D* device = this->ActorDelegate->GetScene()->GetLastPainter()->GetDevice();

  // Downcast is needed because the context device superclass does
  // not define this method (but probably should).
  vtkOpenGLContextDevice2D* oglDevice = vtkOpenGLContextDevice2D::SafeDownCast(device);
  if (oglDevice)
  {
    oglDevice->ReleaseGraphicsResources(window);
  }
}

//----------------------------------------------------------------------------
double vtkContext2DTexturedScalarBarActor::GetSize()
{
  if (!this->CurrentViewport)
  {
    vtkDebugMacro(<< "No current viewport, cannot compute size.");
    return 0.;
  }

  // Convert scalar bar length from normalized viewport coordinates to pixels
  vtkNew<vtkCoordinate> lengthCoord;
  lengthCoord->SetCoordinateSystemToNormalizedViewport();
  lengthCoord->SetValue(0., this->ScalarBarLength);
  double* lengthOffset = lengthCoord->GetComputedDoubleDisplayValue(this->CurrentViewport);

  // Scalar bar size is relative to viewport height (2nd coordinate).
  // That way the scalar bar shrinks the same way the scene does when resizing
  // the viewport (only happens vertically).
  return lengthOffset[1];
}

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::UpdateTextProperties()
{
  // We can't just ShallowCopy the properties because it will clobber
  // the orientation and justification settings.
  vtkTextProperty* axisLabelProperty = this->Axis->GetLabelProperties();
  ::CopyTextProperties(this->LabelTextProperty, axisLabelProperty);

  vtkTextProperty* axisTitleProperty = this->Axis->GetTitleProperties();
  ::CopyTextProperties(this->TitleTextProperty, axisTitleProperty);
}

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::PaintColorBar(vtkContext2D* painter, double size)
{
  if (this->Texture)
  {
    vtkNew<vtkPVLogoSource> logoSource;
    logoSource->SetTexture(this->Texture);
    logoSource->Update();

    vtkImageData* image = vtkImageData::SafeDownCast(logoSource->GetOutput());
    vtkRectf barRect = ::GetColorBarRect(size);
    painter->DrawImage(barRect, image);
  }

  // Finally, draw a rect around the scalar bar and out-of-range
  // colors, if they are enabled.  We should probably draw four
  // lines instead.
  if (this->DrawScalarBarOutline)
  {
    vtkBrush* brush = painter->GetBrush();
    brush->SetOpacity(0);

    vtkPen* pen = painter->GetPen();
    pen->SetLineType(vtkPen::SOLID_LINE);
    pen->SetColorF(this->ScalarBarOutlineColor);
    pen->SetWidth(static_cast<float>(this->ScalarBarOutlineThickness));

    vtkRectf outlineRect = ::GetColorBarRect(size);
    painter->DrawRect(
      outlineRect.GetX(), outlineRect.GetY(), outlineRect.GetWidth(), outlineRect.GetHeight());
  }
}

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::PaintAxis(
  vtkContext2D* painter, double size, int orientation)
{
  vtkRectf rect = ::GetColorBarRect(size);

  // Use the length of the character "|" at the label font size for various
  // measurements.
  std::array<float, 4> bounds;
  painter->ApplyTextProp(this->LabelTextProperty);
  painter->ComputeStringBounds("|", bounds.data());
  float pipeHeight = bounds[3];

  // Note that at this point the font size is already scaled by the tile
  // scale factor. Later on, vtkAxis will scale the tick length and label offset
  // by the tile scale factor again, so we need to divide by the tile scale
  // factor here to take that into account.
  vtkWindow* renWin = this->CurrentViewport->GetVTKWindow();
  std::array<int, 2> tileScale;
  renWin->GetTileScale(tileScale.data());
  if (tileScale[1] == 0)
  {
    vtkWarningMacro(
      << "Retrieved zero tile scale from the render window. Scalar bar axis will not be drawn.");
    return;
  }
  else
  {
    pipeHeight /= tileScale[1];
  }

  // Compute a shift amount for tick marks.
  float axisShift = 0.25 * pipeHeight;

  // Compute tick lengths and label offsets based on the label font size
  float tickLength = 0.75 * pipeHeight;
  this->Axis->SetTickLength(tickLength);

  float offset = orientation == VTK_ORIENT_VERTICAL ? 0.5 : 0.3;
  this->Axis->SetLabelOffset(tickLength + offset * tickLength);

  // Position the axis
  if (this->TextPosition == PrecedeScalarBar)
  {
    // Left
    if (orientation == VTK_ORIENT_VERTICAL)
    {
      this->Axis->SetPoint1(rect.GetX() + axisShift, rect.GetY());
      this->Axis->SetPoint2(rect.GetX() + axisShift, rect.GetY() + rect.GetHeight());
      this->Axis->SetPosition(vtkAxis::LEFT);
    }
    else
    {
      // Bottom
      this->Axis->SetPoint1(rect.GetX(), rect.GetY() + axisShift);
      this->Axis->SetPoint2(rect.GetX() + rect.GetWidth(), rect.GetY() + axisShift);
      this->Axis->SetPosition(vtkAxis::BOTTOM);
    }
  }
  else
  {
    // Right
    if (orientation == VTK_ORIENT_VERTICAL)
    {
      this->Axis->SetPoint1(rect.GetX() + rect.GetWidth() - axisShift, rect.GetY());
      this->Axis->SetPoint2(
        rect.GetX() + rect.GetWidth() - axisShift, rect.GetY() + rect.GetHeight());
      this->Axis->SetPosition(vtkAxis::RIGHT);
    }
    else
    {
      // Top
      this->Axis->SetPoint1(rect.GetX(), rect.GetY() + rect.GetHeight() - axisShift);
      this->Axis->SetPoint2(
        rect.GetX() + rect.GetWidth(), rect.GetY() + rect.GetHeight() - axisShift);
      this->Axis->SetPosition(vtkAxis::TOP);
    }
  }

  vtkPen* axisPen = this->Axis->GetPen();
  axisPen->SetColorF(this->LabelTextProperty->GetColor());

  auto* range = orientation == VTK_ORIENT_VERTICAL ? this->FirstRange : this->SecondRange;
  auto notation =
    this->AutomaticLabelFormat ? vtkAxis::STANDARD_NOTATION : vtkAxis::PRINTF_NOTATION;
  auto* cutomLabels = this->UseCustomLabels ? this->CustomLabels : nullptr;

  // NOTE: the order of calls to this->Axis is important and should be
  // changed only with extreme care.
  this->Axis->SetTickLabelAlgorithm(vtkAxis::TICK_SIMPLE);
  this->Axis->SetUnscaledMinimumLimit(std::numeric_limits<double>::max() * -1.0);
  this->Axis->SetUnscaledMaximumLimit(std::numeric_limits<double>::max());
  this->Axis->SetUnscaledRange(range);
  this->Axis->SetAxisVisible(false);
  this->Axis->SetLabelsVisible(this->DrawTickLabels == 1);
  this->Axis->SetTicksVisible(this->DrawTickMarks);
  this->Axis->SetGridVisible(false);
  this->Axis->SetNotation(notation);
  this->Axis->SetLabelFormat(std::string(this->LabelFormat));
  this->Axis->AutoScale();
  this->Axis->SetRangeLabelsVisible(this->AddRangeLabels);
  this->Axis->SetRangeLabelFormat(std::string(this->RangeLabelFormat));
  this->Axis->SetCustomTickPositions(cutomLabels);
  this->Axis->RecalculateTickSpacing();

  this->Axis->Update();
  this->Axis->Paint(painter);
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DTexturedScalarBarActor::ComputeAxisBoundingRect(double size, int orientation)
{
  // vtkAxis::GetBoundingRect() is not accurate.  Compute it ourselves.
  // All the code in this section is needed to get the actual bounds of
  // the axis including any offsets applied in PaintAxis().
  vtkNew<vtkBoundingRectContextDevice2D> boundingDevice;
  vtkNew<vtkContextDevice2D> contextDevice;
  boundingDevice->SetDelegateDevice(contextDevice.Get());
  boundingDevice->Begin(this->CurrentViewport);
  vtkNew<vtkContext2D> context;
  context->Begin(boundingDevice);
  this->PaintAxis(context, size, orientation);
  context->End();
  boundingDevice->End();

  return boundingDevice->GetBoundingRect();
}

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::PaintTitle(
  vtkContext2D* painter, double size, const std::string& title, int orientation)
{
  std::string combinedTitle(title);
  if (this->ComponentTitle && strlen(this->ComponentTitle) > 0)
  {
    combinedTitle.append(" ");
    combinedTitle.append(this->ComponentTitle);
  }

  // Apply the text property so that title size is up to date.
  double titleOrientation = 0.0;
  if (orientation == VTK_ORIENT_VERTICAL)
  {
    titleOrientation = 90.0;
  }
  this->TitleTextProperty->SetOrientation(titleOrientation);
  this->TitleTextProperty->SetJustification(this->GetTitleJustification());
  painter->ApplyTextProp(this->TitleTextProperty);

  // Get title size
  std::array<float, 4> titleBounds;
  painter->ComputeStringBounds(combinedTitle, titleBounds.data());
  float titleWidth = titleBounds[2];
  float titleHeight = titleBounds[3];

  vtkRectf axisRect = this->ComputeAxisBoundingRect(size, orientation);
  vtkRectf barAndAxisRect = axisRect;
  vtkRectf colorBarRect = ::GetColorBarRect(size);
  barAndAxisRect.AddRect(colorBarRect);

  float titleX = barAndAxisRect.GetX() + 0.5 * barAndAxisRect.GetWidth();
  float titleY = colorBarRect.GetY() + 0.5 * colorBarRect.GetHeight();
  if (orientation == VTK_ORIENT_HORIZONTAL)
  {
    if (this->GetTitleJustification() == VTK_TEXT_LEFT)
    {
      titleX = barAndAxisRect.GetX();
    }
    else if (this->GetTitleJustification() == VTK_TEXT_RIGHT)
    {
      titleX = barAndAxisRect.GetX() + barAndAxisRect.GetWidth();
    }
    if (this->GetTextPosition() == vtkContext2DTexturedScalarBarActor::PrecedeScalarBar)
    {
      titleY = axisRect.GetY() - titleHeight - 0.25 * titleHeight;
    }
    else
    {
      // Handle zero-height axis.
      if (axisRect.GetHeight() < 1.0)
      {
        axisRect.SetHeight(colorBarRect.GetHeight());
      }
      titleY = axisRect.GetY() + axisRect.GetHeight() + 0.25 * titleHeight;
    }
  }
  else // Vertical orientation
  {
    // Handle zero-width axis.
    if (axisRect.GetWidth() < 1.0)
    {
      axisRect.SetWidth(0.25 * colorBarRect.GetWidth());
    }
    if (this->GetTitleJustification() == VTK_TEXT_LEFT)
    {
      titleY = barAndAxisRect.GetY();
    }
    else if (this->GetTitleJustification() == VTK_TEXT_RIGHT)
    {
      titleY = barAndAxisRect.GetY() + barAndAxisRect.GetHeight();
    }
    if (this->GetTextPosition() == vtkContext2DTexturedScalarBarActor::PrecedeScalarBar)
    {
      titleX = colorBarRect.GetX() - axisRect.GetWidth();
    }
    else
    {
      titleX = colorBarRect.GetX() + colorBarRect.GetWidth() + axisRect.GetWidth() + titleWidth;
    }
  }

  painter->ApplyTextProp(this->TitleTextProperty);
  painter->DrawString(titleX, titleY, combinedTitle);
}

//----------------------------------------------------------------------------
bool vtkContext2DTexturedScalarBarActor::Paint(vtkContext2D* painter)
{
  if (!this->Visibility)
  {
    return false;
  }

  double size = this->GetSize();

  // If scalar bar size is zero, no need to go further
  if (size == 0.)
  {
    return false;
  }

  this->UpdateTextProperties();

  vtkPen* pen = painter->GetPen();
  vtkBrush* brush = painter->GetBrush();

  // Save previous settings
  vtkNew<vtkPen> savePen;
  savePen->DeepCopy(pen);
  vtkNew<vtkBrush> saveBrush;
  saveBrush->DeepCopy(brush);

  pen->SetColorF(1, 1, 1);
  brush->SetColorF(1, 0, 0);

  vtkNew<vtkPoints2D> rect;
  rect->InsertNextPoint(0, 00);
  rect->InsertNextPoint(200, 40);

  int* displayPosition = this->PositionCoordinate->GetComputedDisplayValue(this->CurrentViewport);

  // Ensure that the scene held by the Axis is the current renderer
  // so that things like tile scale and DPI are correct.
  this->Axis->GetScene()->SetRenderer(vtkRenderer::SafeDownCast(this->CurrentViewport));

  // Paint the various components
  vtkNew<vtkTransform2D> tform;
  tform->Translate(displayPosition[0], displayPosition[1]);
  painter->PushMatrix();
  painter->AppendTransform(tform.GetPointer());

  // Draw background if enabled
  if (this->DrawBackground && !this->InGetBoundingRect)
  {
    pen->SetLineType(vtkPen::NO_PEN);
    brush->SetColorF(this->BackgroundColor[0], this->BackgroundColor[1], this->BackgroundColor[2],
      this->BackgroundColor[3]);
    painter->DrawRect(this->CurrentBoundingRect.GetX() - this->BackgroundPadding,
      this->CurrentBoundingRect.GetY() - this->BackgroundPadding,
      this->CurrentBoundingRect.GetWidth() + 2.0 * this->BackgroundPadding,
      this->CurrentBoundingRect.GetHeight() + 2.0 * this->BackgroundPadding);
  }

  // Draw texture
  this->PaintColorBar(painter, size);

  // Draw axis
  this->PaintAxis(painter, size, VTK_ORIENT_VERTICAL);
  this->PaintAxis(painter, size, VTK_ORIENT_HORIZONTAL);

  // Draw titles
  this->PaintTitle(painter, size, this->Title, VTK_ORIENT_VERTICAL);
  this->PaintTitle(painter, size, this->Title2, VTK_ORIENT_HORIZONTAL);

  // Restore settings
  pen->DeepCopy(savePen.GetPointer());
  brush->DeepCopy(saveBrush.GetPointer());

  painter->PopMatrix();

  return false;
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DTexturedScalarBarActor::GetBoundingRect()
{
  this->InGetBoundingRect = true;
  vtkRectf rect =
    vtkBoundingRectContextDevice2D::GetBoundingRect(this->ScalarBarItem, this->CurrentViewport);
  this->InGetBoundingRect = false;

  return rect;
}

//----------------------------------------------------------------------------
void vtkContext2DTexturedScalarBarActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
