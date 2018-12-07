#include "vtkContext2DScalarBarActor.h"

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

#include <limits>
#include <map>

#if defined(_WIN32) && !defined(__CYGWIN__)
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

// NOTE FOR DEVELOPERS
// The color bar is defined so that the origin (0, 0) is the bottom left
// corner of the region that contains the scalar bar, the out-of-range
// color swatches, and the NaN color swatches. This is true for both
// horizontal and vertical orientations.
//
// The layout of the color bar for both orientations is as follows:
//
// VERTICAL           HORIZONTAL
//
//   +-+              Below Range    Above Range  NaN Color
//   | | Above Range   +-+-------------------+-+ +-+
//   +-+               | |                   | | | |
//   | |               +-+-------------------+-+-+-+
//   | |             (0, 0)    Scalar Bar
//   | |
//   | | Scalar Bar
//   | |
//   +-+
//   | | Below Range
//   +-+
//
//   +-+
//   | | Nan Color
//   + +
// (0, 0)

// This class is a vtkContextItem that can be added to a vtkContextScene.
//----------------------------------------------------------------------------
class vtkContext2DScalarBarActor::vtkScalarBarItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkScalarBarItem, vtkContextItem);

  static vtkScalarBarItem* New() { VTK_OBJECT_FACTORY_NEW_BODY(vtkScalarBarItem); }

  // Forward calls to vtkContextItem::Paint to vtkContext2DScalarBarActor
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
  vtkContext2DScalarBarActor* Actor;

protected:
  vtkScalarBarItem()
    : Actor(NULL)
  {
  }
  ~vtkScalarBarItem() override {}
};

//----------------------------------------------------------------------------
// Hide use of std::map from public interface
class vtkContext2DScalarBarActor::vtkAnnotationMap : public std::map<double, vtkStdString>
{
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContext2DScalarBarActor);

//----------------------------------------------------------------------------
vtkContext2DScalarBarActor::vtkContext2DScalarBarActor()
{
  this->ActorDelegate = vtkContextActor::New();

  this->TitleJustification = VTK_TEXT_LEFT;
  this->ForceHorizontalTitle = false;

  this->ScalarBarThickness = 16;
  this->ScalarBarLength = 0.33;

  this->AutomaticLabelFormat = 1;

  this->AddRangeLabels = 1;
  this->AutomaticAnnotations = 0;
  this->AddRangeAnnotations = 0;
  this->RangeLabelFormat = NULL;
  this->SetRangeLabelFormat("%g");

  this->OutlineScalarBar = 0;

  this->Spacer = 4.0;

  this->DrawTickMarks = true;

  this->UseCustomLabels = false;
  this->CustomLabels = vtkSmartPointer<vtkDoubleArray>::New();

  this->ReverseLegend = false;

  this->ScalarBarItem = vtkScalarBarItem::New();
  this->ScalarBarItem->Actor = this;

  vtkContextScene* localScene = vtkContextScene::New();
  this->ActorDelegate->SetScene(localScene);
  localScene->AddItem(this->ScalarBarItem);
  localScene->Delete();

  this->CurrentViewport = NULL;

  this->Axis = vtkAxis::New();
  this->Axis->SetScene(localScene);
}

//----------------------------------------------------------------------------
vtkContext2DScalarBarActor::~vtkContext2DScalarBarActor()
{
  this->SetLookupTable(NULL);
  this->ActorDelegate->Delete();
  this->SetTitle(NULL);
  this->SetComponentTitle(NULL);
  this->ScalarBarItem->Delete();
  this->SetTitleTextProperty(NULL);
  this->SetLabelTextProperty(NULL);
  this->Axis->Delete();
  this->SetRangeLabelFormat(nullptr);
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::SetUseCustomLabels(bool useLabels)
{
  if (useLabels != this->UseCustomLabels)
  {
    this->UseCustomLabels = useLabels;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::SetNumberOfCustomLabels(vtkIdType numLabels)
{
  this->CustomLabels->SetNumberOfTuples(numLabels);
}

//----------------------------------------------------------------------------
vtkIdType vtkContext2DScalarBarActor::GetNumberOfCustomLabels()
{
  return this->CustomLabels->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::SetCustomLabel(vtkIdType index, double value)
{
  if (index < 0 || index >= this->CustomLabels->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Index out of range");
    return;
  }

  this->CustomLabels->SetTypedTuple(index, &value);
}

//----------------------------------------------------------------------------
int vtkContext2DScalarBarActor::RenderOverlay(vtkViewport* viewport)
{
  this->CurrentViewport = viewport;

  int returnValue = 0;
  if (this->ActorDelegate)
  {
    returnValue = this->ActorDelegate->RenderOverlay(viewport);
  }

  return returnValue;
}

//----------------------------------------------------------------------------
int vtkContext2DScalarBarActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->CurrentViewport = viewport;

  return 1;
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::ReleaseGraphicsResources(vtkWindow* window)
{
  if (!this->ActorDelegate || !this->ActorDelegate->GetScene() ||
    !this->ActorDelegate->GetScene()->GetLastPainter())
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
void vtkContext2DScalarBarActor::UpdateScalarBarTexture(vtkImageData* image)
{
  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    image->SetDimensions(1, 256, 1);
  }
  else
  {
    image->SetDimensions(256, 1, 1);
  }
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  vtkUnsignedCharArray* colors =
    vtkUnsignedCharArray::SafeDownCast(image->GetPointData()->GetArray(0));

  vtkDiscretizableColorTransferFunction* ctf =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
  if (!ctf)
  {
    return;
  }

  double* lutRange = ctf->GetRange();
  const int numColors = 256;
  unsigned char color[4];
  for (int i = 0; i < numColors; ++i)
  {

    // Probably use MapScalarsThroughTable2 here instead.
    // Update only when LUT changes
    double originalValue = (((double)i / numColors) * (lutRange[1] - lutRange[0])) + lutRange[0];
    double value = originalValue;
    if (this->LookupTable->UsingLogScale())
    {
      value = log10(lutRange[0]) + i * (log10(lutRange[1]) - log10(lutRange[0])) / numColors;
      value = pow(10.0, value);
    }

    const unsigned char* colorTmp = ctf->MapValue(value);

    // The opacity function does not take into account the logarithmic
    // mapping, so we use the original value here.
    color[0] = colorTmp[0];
    color[1] = colorTmp[1];
    color[2] = colorTmp[2];
    color[3] = static_cast<unsigned char>(255.0 * ctf->GetOpacity(originalValue) + 0.5);
    if (this->ReverseLegend)
    {
      colors->SetTypedTuple(numColors - i - 1, color);
    }
    else
    {
      colors->SetTypedTuple(i, color);
    }
  }
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::GetSize(double size[2], vtkContext2D* painter)
{
  if (!this->CurrentViewport)
  {
    return;
  }

  // Convert scalar bar length from normalized viewport coordinates to pixels
  vtkNew<vtkCoordinate> lengthCoord;
  lengthCoord->SetCoordinateSystemToNormalizedViewport();
  lengthCoord->SetValue(this->Orientation == VTK_ORIENT_VERTICAL ? 0.0 : this->ScalarBarLength,
    this->Orientation == VTK_ORIENT_VERTICAL ? this->ScalarBarLength : 0.0);
  int* lengthOffset = lengthCoord->GetComputedDisplayValue(this->CurrentViewport);

  // The scalar bar thickness is defined in terms of points. That is,
  // if the thickness size is 12, that matches the height of a "|"
  // character in a 12 point font.
  vtkNew<vtkTextProperty> textProp;
  textProp->SetFontSize(this->ScalarBarThickness);
  painter->ApplyTextProp(textProp.Get());

  float bounds[4];
  painter->ComputeStringBounds("|", bounds);
  double thickness = bounds[3];

  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    size[0] = thickness;
    size[1] = lengthOffset[1];
  }
  else
  {
    size[0] = lengthOffset[0];
    size[1] = thickness;
  }
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DScalarBarActor::GetColorBarRect(double size[2])
{
  vtkRectf rect = vtkRectf(0, 0, size[0], size[1]);

  // Color swatches are squares with sides equal to the width of the
  // color bar when in vertical orientation and equal to the height
  // when in horizontal orientation.
  double swatchSize = this->Orientation == VTK_ORIENT_VERTICAL ? size[0] : size[1];

  // Count up the swatches
  double shift = 0;
  double sizeReduction = 0;
  if (this->DrawNanAnnotation)
  {
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      shift += swatchSize + this->Spacer;
    }
    sizeReduction += swatchSize + this->Spacer;
  }

  vtkDiscretizableColorTransferFunction* ctf =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
  if (ctf && ctf->GetUseAboveRangeColor())
  {
    sizeReduction += swatchSize;
  }

  if (ctf && ctf->GetUseBelowRangeColor())
  {
    shift += swatchSize;
    sizeReduction += swatchSize;
  }

  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    rect.SetY(rect.GetY() + shift);
    rect.SetHeight(rect.GetHeight() - sizeReduction);
  }
  else
  {
    rect.SetX(rect.GetX() + shift);
    rect.SetWidth(rect.GetWidth() - sizeReduction);
  }

  return rect;
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DScalarBarActor::GetFullColorBarRect(double size[2])
{
  // This will end up with the full color bar rect
  vtkRectf fullRect = this->GetColorBarRect(size);

  // Add these rects in if they have non-zero size
  vtkRectf aboveRect = this->GetAboveRangeColorRect(size);
  if (aboveRect.GetWidth() > 0 && aboveRect.GetHeight() > 0)
  {
    fullRect.AddRect(aboveRect);
  }

  vtkRectf belowRect = this->GetBelowRangeColorRect(size);
  if (belowRect.GetWidth() > 0 && belowRect.GetHeight() > 0)
  {
    fullRect.AddRect(belowRect);
  }

  return fullRect;
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DScalarBarActor::GetAboveRangeColorRect(double size[2])
{
  vtkRectf rect(0, 0, 0, 0);

  vtkDiscretizableColorTransferFunction* ctf =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
  if (!ctf)
  {
    if (this->LookupTable)
    {
      vtkErrorMacro(<< "Lookup table should be a vtkDiscretizableColorTransferFunction but was a "
                    << this->LookupTable->GetClassName());
    }
    else
    {
      vtkErrorMacro(<< "Lookup table was NULL");
    }
    return rect;
  }

  if (ctf->GetUseAboveRangeColor())
  {
    rect = this->GetOutOfRangeColorRectInternal(vtkContext2DScalarBarActor::ABOVE_RANGE, size);
  }
  return rect;
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DScalarBarActor::GetBelowRangeColorRect(double size[2])
{
  vtkRectf rect(0, 0, 0, 0);

  vtkDiscretizableColorTransferFunction* ctf =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
  if (!ctf)
  {
    if (this->LookupTable)
    {
      vtkErrorMacro(<< "Lookup table should be a vtkDiscretizableColorTransferFunction but was a "
                    << this->LookupTable->GetClassName());
    }
    else
    {
      vtkErrorMacro(<< "Lookup table was NULL");
    }
    return rect;
  }

  if (ctf->GetUseBelowRangeColor())
  {
    rect = this->GetOutOfRangeColorRectInternal(vtkContext2DScalarBarActor::BELOW_RANGE, size);
  }
  return rect;
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DScalarBarActor::GetOutOfRangeColorRectInternal(
  vtkContext2DScalarBarActor::OutOfRangeType type, double size[2])
{
  vtkRectf rect(0, 0, 0, 0);
  bool graphicallyAbove = type == vtkContext2DScalarBarActor::ABOVE_RANGE && !this->ReverseLegend;
  if (graphicallyAbove)
  {
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      double width = size[0];
      rect = vtkRectf(0, size[1] - width, width, width);
    }
    else
    {
      // Horizontal
      double nanSpace = this->GetNaNColorRect(size).GetWidth();
      if (nanSpace > 0)
      {
        nanSpace += this->Spacer;
      }
      double height = size[1];

      // Move it all the way to the right, minus the NaN swatch
      rect = vtkRectf(size[0] - nanSpace - height, 0, height, height);
    }
  }
  else
  {
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      double nanSpace = this->GetNaNColorRect(size).GetHeight();
      if (nanSpace > 0)
      {
        nanSpace += this->Spacer;
      }

      double height = size[0];
      rect = vtkRectf(0, nanSpace, height, height);
    }
    else
    {
      double width = size[1];
      rect = vtkRectf(0, 0, width, width);
    }
  }
  return rect;
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DScalarBarActor::GetNaNColorRect(double size[2])
{
  // Initialize to 0 width, 0 height
  vtkRectf rect(0, 0, 0, 0);

  if (this->DrawNanAnnotation)
  {
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      double width = size[0];
      rect = vtkRectf(0, 0, width, width);
    }
    else
    {
      // Horizontal
      double height = size[1];
      rect = vtkRectf(size[0] - height, 0, height, height);
    }
  }

  return rect;
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::UpdateTextProperties()
{
  // We can't just ShallowCopy the LabelTextProperty to axisTextProperty
  // because it will clobber the orientation and justification settings.
  vtkTextProperty* axisLabelProperty = this->Axis->GetLabelProperties();
  axisLabelProperty->SetColor(this->LabelTextProperty->GetColor());
  axisLabelProperty->SetOpacity(this->LabelTextProperty->GetOpacity());
  axisLabelProperty->SetBackgroundColor(this->LabelTextProperty->GetBackgroundColor());
  axisLabelProperty->SetBackgroundOpacity(this->LabelTextProperty->GetBackgroundOpacity());
  axisLabelProperty->SetFontFamilyAsString(this->LabelTextProperty->GetFontFamilyAsString());
  axisLabelProperty->SetFontFile(this->LabelTextProperty->GetFontFile());
  axisLabelProperty->SetFontSize(this->LabelTextProperty->GetFontSize());

  axisLabelProperty->SetBold(this->LabelTextProperty->GetBold());
  axisLabelProperty->SetItalic(this->LabelTextProperty->GetItalic());
  axisLabelProperty->SetShadow(this->LabelTextProperty->GetShadow());
  axisLabelProperty->SetShadowOffset(this->LabelTextProperty->GetShadowOffset());

  vtkTextProperty* axisTitleProperty = this->Axis->GetTitleProperties();
  axisTitleProperty->SetColor(this->TitleTextProperty->GetColor());
  axisTitleProperty->SetOpacity(this->TitleTextProperty->GetOpacity());
  axisTitleProperty->SetBackgroundColor(this->TitleTextProperty->GetBackgroundColor());
  axisTitleProperty->SetBackgroundOpacity(this->TitleTextProperty->GetBackgroundOpacity());
  axisTitleProperty->SetFontFamilyAsString(this->TitleTextProperty->GetFontFamilyAsString());
  axisTitleProperty->SetFontFile(this->TitleTextProperty->GetFontFile());
  axisTitleProperty->SetFontSize(this->TitleTextProperty->GetFontSize());

  axisTitleProperty->SetBold(this->TitleTextProperty->GetBold());
  axisTitleProperty->SetItalic(this->TitleTextProperty->GetItalic());
  axisTitleProperty->SetShadow(this->TitleTextProperty->GetShadow());
  axisTitleProperty->SetShadowOffset(this->TitleTextProperty->GetShadowOffset());
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::PaintColorBar(vtkContext2D* painter, double size[2])
{
  vtkRectf barRect = this->GetColorBarRect(size);

  vtkBrush* brush = painter->GetBrush();

  //-----------------------------
  // Draw scalar bar itself
  vtkDiscretizableColorTransferFunction* ctf =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
  if (!ctf)
  {
    return;
  }

  // Disable pen to prevent an outline around the color swatches
  vtkPen* pen = painter->GetPen();
  pen->SetLineType(vtkPen::NO_PEN);

  // Create a map from anchor values to annotations. Since maps sort by key,
  // when we iterate over the annotations later on, we will be doing it from
  // smallest to greatest annotation value.
  vtkAnnotationMap annotationAnchors;

  if (ctf->GetIndexedLookup())
  {
    // Divide up the color bar rect into the number of indexed colors
    int numIndexedColors = ctf->GetNumberOfAnnotatedValues();
    double indexedColorSwatchLength =
      this->Orientation == VTK_ORIENT_VERTICAL ? barRect.GetHeight() : barRect.GetWidth();

    // Subtract spaces between swatches
    if (numIndexedColors > 0)
    {
      indexedColorSwatchLength -= (numIndexedColors - 1) * this->Spacer;
      indexedColorSwatchLength /= numIndexedColors;
    }

    // Now loop over indexed colors and draw swatches
    double x, y;
    for (int i = 0; i < numIndexedColors; ++i)
    {
      double shift = i * (indexedColorSwatchLength + this->Spacer);
      double indexedColor[4];
      vtkVariant annotatedValue = ctf->GetAnnotatedValue(i);
      ctf->GetIndexedColor(i, indexedColor);
      vtkStdString annotation = ctf->GetAnnotation(i);
      brush->SetColorF(indexedColor);
      if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
        x = barRect.GetX();
        if (this->ReverseLegend)
        {
          y = barRect.GetY() + shift;
        }
        else
        {
          y = barRect.GetY() + barRect.GetHeight() - shift - indexedColorSwatchLength;
        }
        painter->DrawRect(x, y, barRect.GetWidth(), indexedColorSwatchLength);
        annotationAnchors[y + 0.5 * indexedColorSwatchLength] = annotation;
      }
      else
      {
        // Horizontal
        if (this->ReverseLegend)
        {
          x = barRect.GetX() + barRect.GetWidth() - shift - indexedColorSwatchLength;
        }
        else
        {
          x = barRect.GetX() + shift;
        }
        y = barRect.GetY();
        painter->DrawRect(x, y, indexedColorSwatchLength, barRect.GetHeight());
        annotationAnchors[x + 0.5 * indexedColorSwatchLength] = annotation;
      }
    }
  }
  else
  // Continuous color map
  {
    vtkNew<vtkImageData> image;
    this->UpdateScalarBarTexture(image.GetPointer());

    painter->DrawImage(barRect, image.GetPointer());

    // Draw the out-of-range colors if enabled
    // pen->SetLineType(vtkPen::NO_PEN);
    if (ctf->GetUseAboveRangeColor())
    {
      vtkRectf rect = this->GetAboveRangeColorRect(size);
      brush->SetColorF(ctf->GetAboveRangeColor());
      pen->SetLineType(vtkPen::NO_PEN);
      painter->DrawRect(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
    }

    if (ctf->GetUseBelowRangeColor())
    {
      vtkRectf rect = this->GetBelowRangeColorRect(size);
      brush->SetColorF(ctf->GetBelowRangeColor());
      pen->SetLineType(vtkPen::NO_PEN);
      painter->DrawRect(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
    }

    // Finally, draw a rect around the scalar bar and out-of-range
    // colors, if they are enabled.  We should probably draw four
    // lines instead.
    if (this->OutlineScalarBar)
    {
      vtkRectf outlineRect = this->GetFullColorBarRect(size);

      brush->SetOpacity(0);
      pen->SetLineType(vtkPen::SOLID_LINE);
      painter->DrawRect(
        outlineRect.GetX(), outlineRect.GetY(), outlineRect.GetWidth(), outlineRect.GetHeight());
    }

    // Now set up annotation anchor point map
    double lutRange[2];
    lutRange[0] = this->LookupTable->GetRange()[0];
    lutRange[1] = this->LookupTable->GetRange()[1];
    if (this->LookupTable->UsingLogScale())
    {
      lutRange[0] = log10(lutRange[0]);
      lutRange[1] = log10(lutRange[1]);
    }

    double low = barRect.GetX();
    double high = low + barRect.GetWidth();
    if (this->Orientation == VTK_ORIENT_VERTICAL)
    {
      low = barRect.GetY();
      high = low + barRect.GetHeight();
    }
    if (this->ReverseLegend)
    {
      std::swap(high, low);
    }

    if (this->GetAutomaticAnnotations())
    {
      // How many annotations should there be?
      vtkIdType numValues = ctf->GetNumberOfAvailableColors();
      if (ctf && ctf->GetDiscretize() && this->AutomaticAnnotations && numValues)
      {
        double step = (lutRange[1] - lutRange[0]) / numValues;
        for (vtkIdType i = 0; i <= numValues; i++)
        {
          double annotatedValue = lutRange[0] + step * i;

          double normalizedValue = (annotatedValue - lutRange[0]) / (lutRange[1] - lutRange[0]);

          double barPosition = normalizedValue * (high - low) + low;
          if (normalizedValue >= 0.0 - std::numeric_limits<double>::epsilon() &&
            normalizedValue <= 1.0 + std::numeric_limits<double>::epsilon() &&
            !vtkMath::IsNan(barPosition))
          {
            char annotation[1024];
            if (this->LookupTable->UsingLogScale())
            {
              annotatedValue = pow(10.0, annotatedValue);
            }
            SNPRINTF(annotation, 1023, this->LabelFormat, annotatedValue);
            annotationAnchors[barPosition] = annotation;
          }
        }
      }
    }
    else // Manual annotations
    {
      int numAnnotations = ctf->GetNumberOfAnnotatedValues();
      for (int i = 0; i < numAnnotations; ++i)
      {
        // Figure out placement of annotation value along color bar.
        double annotatedValue = ctf->GetAnnotatedValue(i).ToDouble();
        if (this->LookupTable->UsingLogScale())
        {
          // Scale in log space
          annotatedValue = log10(annotatedValue);
        }

        double normalizedValue = (annotatedValue - lutRange[0]) / (lutRange[1] - lutRange[0]);

        double barPosition = normalizedValue * (high - low) + low;
        if (normalizedValue >= 0.0 && normalizedValue <= 1.0 && !vtkMath::IsNan(barPosition))
        {
          vtkStdString annotation = ctf->GetAnnotation(i);
          annotationAnchors[barPosition] = annotation;
        }
      }
    }

    if (this->AddRangeAnnotations)
    {
      char annotation[1024];

      SNPRINTF(annotation, 1023, this->RangeLabelFormat, lutRange[0]);
      annotationAnchors[low] = annotation;

      SNPRINTF(annotation, 1023, this->RangeLabelFormat, lutRange[1]);
      annotationAnchors[high] = annotation;
    }

  } // Continuous color map

  // For all types of color maps, draw the NaN annotation.
  if (this->DrawNanAnnotation)
  {
    // Paint NaN color swatch
    vtkRectf rect = this->GetNaNColorRect(size);
    brush->SetOpacity(255);
    brush->SetColorF(ctf->GetNanColor());
    pen->SetLineType(vtkPen::NO_PEN);
    painter->DrawRect(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

    // Add NaN annotation
    double nanAnchor = rect.GetY() + 0.5 * rect.GetHeight();
    if (this->Orientation == VTK_ORIENT_HORIZONTAL)
    {
      nanAnchor = rect.GetX() + 0.5 * rect.GetWidth();
    }
    annotationAnchors[nanAnchor] = this->GetNanAnnotation();
  }

  // Draw the annotations
  if (this->GetDrawAnnotations())
  {
    this->PaintAnnotations(painter, size, annotationAnchors);
  }
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::PaintAxis(vtkContext2D* painter, double size[2])
{
  vtkRectf rect = this->GetColorBarRect(size);

  // Use the length of the character "|" at the label font size for various
  // measurements.
  float bounds[4];
  painter->ApplyTextProp(this->LabelTextProperty);
  painter->ComputeStringBounds("|", bounds);
  float pipeHeight = bounds[3];

  // Note that at this point the font size is already scaled by the tile
  // scale factor. Later on, vtkAxis will scale the tick length and label offset
  // by the tile scale factor again, so we need to divide by the tile scale
  // factor here to take that into account.
  vtkWindow* renWin = this->CurrentViewport->GetVTKWindow();
  int tileScale[2];
  renWin->GetTileScale(tileScale);
  pipeHeight /= tileScale[1];

  // Compute a shift amount for tick marks.
  float axisShift = 0.25 * pipeHeight;

  // Compute tick lengths and label offsets based on the label font size
  float tickLength = 0.75 * pipeHeight;
  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    this->Axis->SetTickLength(tickLength);

    // Offset the labels from the tick marks a bit
    float labelOffset = tickLength + (0.5 * tickLength);
    this->Axis->SetLabelOffset(labelOffset);
  }
  else
  {
    this->Axis->SetTickLength(tickLength);

    float labelOffset = tickLength + (0.3 * tickLength);
    this->Axis->SetLabelOffset(labelOffset);
  }

  // Position the axis
  if (this->TextPosition == PrecedeScalarBar)
  {
    // Left
    if (this->Orientation == VTK_ORIENT_VERTICAL)
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
    if (this->Orientation == VTK_ORIENT_VERTICAL)
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

  //-----------------------------
  // Get the range of the lut
  const double* lutRange = this->LookupTable->GetRange();
  double range[2];
  range[0] = lutRange[0];
  range[1] = lutRange[1];

  if (this->ReverseLegend)
  {
    std::swap(range[0], range[1]);
  }

  vtkPen* axisPen = this->Axis->GetPen();
  axisPen->SetColorF(this->LabelTextProperty->GetColor());

  bool indexedMode = this->LookupTable->GetIndexedLookup() == 1;

  // NOTE: the order of calls to this->Axis is important and should be
  // changed only with extreme care.
  this->Axis->SetTickLabelAlgorithm(vtkAxis::TICK_SIMPLE);
  this->Axis->SetUnscaledMinimumLimit(std::numeric_limits<double>::max() * -1.0);
  this->Axis->SetUnscaledMaximumLimit(std::numeric_limits<double>::max());
  this->Axis->SetUnscaledRange(range);
  this->Axis->SetAxisVisible(false);
  this->Axis->SetLabelsVisible(!indexedMode && this->DrawTickLabels == 1);
  this->Axis->SetTicksVisible(!indexedMode && this->DrawTickMarks);
  this->Axis->SetGridVisible(false);

  if (this->AutomaticLabelFormat)
  {
    this->Axis->SetNotation(vtkAxis::STANDARD_NOTATION);
  }
  else
  {
    this->Axis->SetNotation(vtkAxis::PRINTF_NOTATION);
  }
  this->Axis->SetLabelFormat(std::string(this->LabelFormat));
  this->Axis->SetLogScale(this->LookupTable->UsingLogScale() == 1);
  this->Axis->AutoScale();
  this->Axis->SetRangeLabelsVisible(!indexedMode && this->AddRangeLabels == 1);
  this->Axis->SetRangeLabelFormat(std::string(this->RangeLabelFormat));

  if (this->UseCustomLabels)
  {
    if (this->Axis->GetLogScale())
    {
      // Take log of label positions
      vtkNew<vtkDoubleArray> logCustomLabels;
      logCustomLabels->SetNumberOfTuples(this->CustomLabels->GetNumberOfTuples());
      for (vtkIdType id = 0; id < logCustomLabels->GetNumberOfTuples(); ++id)
      {
        double d = this->CustomLabels->GetValue(id);
        d = log10(d);
        logCustomLabels->SetValue(id, d);
      }
      this->Axis->SetCustomTickPositions(logCustomLabels.GetPointer());
    }
    else
    {
      this->Axis->SetCustomTickPositions(this->CustomLabels);
    }
  }
  else
  {
    this->Axis->SetCustomTickPositions(NULL);
  }

  this->Axis->SetUnscaledRange(range);
  this->Axis->RecalculateTickSpacing();

  this->Axis->Update();
  this->Axis->Paint(painter);
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::PaintTitle(vtkContext2D* painter, double size[2])
{
  std::string combinedTitle(this->Title);
  if (this->ComponentTitle && strlen(this->ComponentTitle) > 0)
  {
    combinedTitle.append(" ");
    combinedTitle.append(this->ComponentTitle);
  }

  // Apply the text property so that title size is up to date.
  double titleOrientation = 0.0;
  if (this->GetOrientation() == VTK_ORIENT_VERTICAL && !this->GetForceHorizontalTitle())
  {
    titleOrientation = 90.0;
  }
  this->TitleTextProperty->SetOrientation(titleOrientation);
  this->TitleTextProperty->SetJustification(this->GetTitleJustification());
  painter->ApplyTextProp(this->TitleTextProperty);

  // Get title size
  float titleBounds[4];
  painter->ComputeStringBounds(combinedTitle, titleBounds);
  float titleWidth = titleBounds[2];
  float titleHeight = titleBounds[3];

  // vtkAxis::GetBoundingRect() is not accurate.  Compute it ourselves.
  // All the code in this section is needed to get the actual bounds of
  // the axis including any offsets applied in PaintAxis().
  vtkNew<vtkBoundingRectContextDevice2D> boundingDevice;
  vtkNew<vtkContextDevice2D> contextDevice;
  boundingDevice->SetDelegateDevice(contextDevice.Get());
  boundingDevice->Begin(this->CurrentViewport);
  vtkNew<vtkContext2D> context;
  context->Begin(boundingDevice);
  this->PaintAxis(context, size);
  context->End();
  boundingDevice->End();

  vtkRectf axisRect = boundingDevice->GetBoundingRect();

  vtkRectf barAndAxisRect = axisRect;
  vtkRectf colorBarRect = this->GetColorBarRect(size);
  barAndAxisRect.AddRect(colorBarRect);

  float titleX = barAndAxisRect.GetX() + 0.5 * barAndAxisRect.GetWidth();
  float titleY = colorBarRect.GetY() + 0.5 * colorBarRect.GetHeight();
  if (this->GetOrientation() == VTK_ORIENT_HORIZONTAL || this->ForceHorizontalTitle)
  {
    if (this->GetTitleJustification() == VTK_TEXT_LEFT)
    {
      titleX = barAndAxisRect.GetX();
    }
    else if (this->GetTitleJustification() == VTK_TEXT_RIGHT)
    {
      titleX = barAndAxisRect.GetX() + barAndAxisRect.GetWidth();
    }
    if (this->GetTextPosition() == vtkContext2DScalarBarActor::PrecedeScalarBar)
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

    // Move title to the top if the title is forced horizontal
    if (this->ForceHorizontalTitle && this->GetOrientation() != VTK_ORIENT_HORIZONTAL)
    {
      titleY = barAndAxisRect.GetY() + barAndAxisRect.GetHeight() + 0.25 * titleHeight;
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
    if (this->GetTextPosition() == vtkContext2DScalarBarActor::PrecedeScalarBar)
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
bool vtkContext2DScalarBarActor::Paint(vtkContext2D* painter)
{
  if (!this->Visibility)
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

  double size[2];
  this->GetSize(size, painter);

  // Paint the various components
  vtkNew<vtkTransform2D> tform;
  tform->Translate(displayPosition[0], displayPosition[1]);
  painter->PushMatrix();
  painter->AppendTransform(tform.GetPointer());

  this->PaintColorBar(painter, size);
  this->PaintAxis(painter, size);
  // IMPORTANT: this needs to be done *after* this->Axis->Update() is called
  // in PaintAxis() so that we get an accurate axis bounding rectangle.
  this->PaintTitle(painter, size);

  // Restore settings
  pen->DeepCopy(savePen.GetPointer());
  brush->DeepCopy(saveBrush.GetPointer());

  painter->PopMatrix();

  return false;
}

//----------------------------------------------------------------------------
vtkRectf vtkContext2DScalarBarActor::GetBoundingRect()
{
  return vtkBoundingRectContextDevice2D::GetBoundingRect(
    this->ScalarBarItem, this->CurrentViewport);
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::PaintAnnotations(
  vtkContext2D* painter, double size[2], const vtkAnnotationMap& annotationAnchors)
{
  // Set the annotation text properties
  painter->ApplyTextProp(this->Axis->GetLabelProperties());

  if (this->Orientation == VTK_ORIENT_VERTICAL)
  {
    this->PaintAnnotationsVertically(painter, size, annotationAnchors);
  }
  else
  {
    this->PaintAnnotationsHorizontally(painter, size, annotationAnchors);
  }
}

namespace
{

//----------------------------------------------------------------------------
typedef struct AI
{
  double Anchor;
  double Position;
  vtkStdString Annotation;
  double Span;
} AnnotationInfo;

//----------------------------------------------------------------------------
void DistributeAnnotations(std::vector<AnnotationInfo>& annotations, float spacer)
{
  // Look for clusters of overlapping annotations. We'll space these
  // annotations about the center of mass of each cluster. This winds
  // up looking nice.

  // Process clusters.
  // 1. Find centroid of cluster.
  // 2. Compute lower and upper bounds of cluster.
  // 3. Layout the labels.
  bool overlapDetected = false;
  int tries = 0;
  int maxTries = 20;

  // Keep track of adjacent annotations that overlap at any point in
  // the placement process. This is to prevent annotations from
  // potentially jumping from cluster to cluster during the layout
  // process. overlaps[j] == true means that annotations j and j+1
  // overlapped at some point.
  std::vector<bool> overlaps(annotations.size(), false);

  // Iterate repeatedely in case repositioning of clustered
  // annotations causes new overlap.
  do
  {
    overlapDetected = false;
    double clusterWidth = 0.0;
    size_t clusterCount = 0;
    for (size_t j = 0; j < annotations.size(); ++j)
    {
      // Check for overlap with neighbors
      bool overlapsNext = false;
      double lowerMax = 0.0;
      double upperMin = 0.0;

      if (j < annotations.size() - 1)
      {
        lowerMax = annotations[j].Position + 0.5 * annotations[j].Span + 0.5 * spacer;
        upperMin = annotations[j + 1].Position - 0.5 * annotations[j + 1].Span - 0.5 * spacer;
        overlapsNext = lowerMax > upperMin;
        overlapDetected = overlapDetected || overlapsNext;
      }

      if (overlapDetected)
      {
        clusterWidth += annotations[j].Span;
        clusterCount++;
      }

      if (overlapsNext)
      {
        overlaps[j] = true;
      }

      if (!overlaps[j])
      {
        // Cluster ended. Go back and change the annotation positions
        // based on the cluster centroid.
        if (clusterCount > 0)
        {
          // Weight centers of each label by width
          double clusterCenter = 0.0;
          for (size_t k = j - clusterCount + 1; k <= j; ++k)
          {
            double weight = annotations[k].Span / clusterWidth;
            clusterCenter += annotations[k].Anchor * weight;
          }

          double accumWidth = 0.0;
          clusterWidth += spacer * (clusterCount - 1); // Add in spacer width
          for (size_t k = 0; k < clusterCount; ++k)
          {
            // Start from the right (bigger coordinate) side and work toward the left.
            annotations[j - k].Position =
              clusterCenter + 0.5 * clusterWidth - accumWidth - 0.5 * annotations[j - k].Span;
            accumWidth += annotations[j - k].Span + spacer;
          }
        }

        // Reset cluster stats
        clusterWidth = 0.0;
        clusterCount = 0;
      }
    }

    ++tries;
  } while (overlapDetected && tries < maxTries);
}

} // end anonymous namespace

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::PaintAnnotationsVertically(
  vtkContext2D* painter, double size[2], const vtkAnnotationMap& annotationAnchors)
{
  vtkRectf barRect = this->GetColorBarRect(size);

  // Copy annotations and position info into a vector.
  std::vector<AnnotationInfo> annotations;
  annotations.reserve(annotationAnchors.size());
  vtkAnnotationMap::const_iterator annotationMapIter;
  for (annotationMapIter = annotationAnchors.begin(); annotationMapIter != annotationAnchors.end();
       ++annotationMapIter)
  {
    float bounds[4]; // bounds contains x, y, width, height
    painter->ComputeStringBounds(annotationMapIter->second, bounds);

    AnnotationInfo p;
    p.Anchor = annotationMapIter->first;
    p.Position = annotationMapIter->first;
    p.Annotation = annotationMapIter->second;
    p.Span = bounds[3]; // height
    annotations.push_back(p);
  }

  vtkWindow* renWin = this->CurrentViewport->GetVTKWindow();
  int tileScale[2];
  renWin->GetTileScale(tileScale);

  // Calculate the annotation labels
  const float spacer = 1 * tileScale[0]; // vertical space between annotations
  DistributeAnnotations(annotations, spacer);

  // Iterate over anchors and draw annotations
  std::vector<AnnotationInfo>::iterator vectorIter;
  for (vectorIter = annotations.begin(); vectorIter != annotations.end(); ++vectorIter)
  {
    const int annotationLeader = 8 * tileScale[0];
    double anchorPt[2] = { barRect.GetX(), vectorIter->Anchor };
    double labelPt[2] = { anchorPt[0] - annotationLeader, vectorIter->Position };

    painter->GetTextProp()->SetJustification(VTK_TEXT_RIGHT);

    if (this->TextPosition == PrecedeScalarBar)
    {
      anchorPt[0] = barRect.GetX() + barRect.GetWidth();
      labelPt[0] = anchorPt[0] + annotationLeader;

      painter->GetTextProp()->SetJustification(VTK_TEXT_LEFT);
    }

    vtkPen* pen = painter->GetPen();
    pen->SetOpacity(255);
    pen->SetLineType(vtkPen::SOLID_LINE);
    pen->SetColorF(this->Axis->GetLabelProperties()->GetColor());

    painter->DrawLine(anchorPt[0], anchorPt[1], labelPt[0], labelPt[1]);
    painter->GetTextProp()->SetVerticalJustification(VTK_TEXT_CENTERED);
    painter->DrawString(labelPt[0], labelPt[1], vectorIter->Annotation);
  }
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::PaintAnnotationsHorizontally(
  vtkContext2D* painter, double size[2], const vtkAnnotationMap& annotationAnchors)
{
  vtkRectf barRect = this->GetColorBarRect(size);

  // Copy annotations and position info into a vector.
  std::vector<AnnotationInfo> annotations;
  annotations.reserve(annotationAnchors.size());
  vtkAnnotationMap::const_iterator annotationMapIter;
  for (annotationMapIter = annotationAnchors.begin(); annotationMapIter != annotationAnchors.end();
       ++annotationMapIter)
  {
    float bounds[4]; // bounds contains x, y, width, height
    painter->ComputeStringBounds(annotationMapIter->second, bounds);
    AnnotationInfo p;
    p.Anchor = annotationMapIter->first;
    p.Position = annotationMapIter->first;
    p.Annotation = annotationMapIter->second;
    p.Span = bounds[2]; // width
    annotations.push_back(p);
  }

  vtkWindow* renWin = this->CurrentViewport->GetVTKWindow();
  int tileScale[2];
  renWin->GetTileScale(tileScale);

  // Get horizontal spacing distance as a function of the font
  // properties. Use width of '-' as spacing between annotations.
  float bounds[4];
  painter->ComputeStringBounds("-", bounds);
  const float spacer = bounds[2] * tileScale[0];

  // Calculate the annotation labels
  DistributeAnnotations(annotations, spacer);

  // Iterate over anchors and draw annotations
  std::vector<AnnotationInfo>::iterator vectorIter;
  for (vectorIter = annotations.begin(); vectorIter != annotations.end(); ++vectorIter)
  {
    const int annotationLeader = 8 * tileScale[0];
    double anchorPt[2] = { vectorIter->Anchor, barRect.GetY() };
    double labelPt[2] = { vectorIter->Position, anchorPt[1] - annotationLeader };
    double labelOffset = 3;

    painter->GetTextProp()->SetJustification(VTK_TEXT_CENTERED);
    painter->GetTextProp()->SetVerticalJustification(VTK_TEXT_TOP);
    if (this->TextPosition == PrecedeScalarBar)
    {
      anchorPt[1] = barRect.GetY() + barRect.GetHeight();
      labelPt[1] = anchorPt[1] + annotationLeader;
      labelOffset *= -1.0;

      painter->GetTextProp()->SetVerticalJustification(VTK_TEXT_BOTTOM);
    }
    painter->DrawString(labelPt[0], labelPt[1] - labelOffset, vectorIter->Annotation);

    vtkPen* pen = painter->GetPen();
    pen->SetOpacity(255);
    pen->SetLineType(vtkPen::SOLID_LINE);
    pen->SetColorF(this->Axis->GetLabelProperties()->GetColor());

    painter->DrawLine(anchorPt[0], anchorPt[1], labelPt[0], labelPt[1]);
  }
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkContext2DScalarBarActor::GetEstimatedNumberOfAnnotations()
{
  vtkDiscretizableColorTransferFunction* ctf =
    vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
  if (!ctf)
  {
    return 0;
  }
  if (this->GetAutomaticAnnotations() && !ctf->GetIndexedLookup())
  {
    // How many annotations should there be?
    return ctf->GetNumberOfAvailableColors();
  }
  else // Manual annotations
  {
    return ctf->GetNumberOfAnnotatedValues();
  }
}
