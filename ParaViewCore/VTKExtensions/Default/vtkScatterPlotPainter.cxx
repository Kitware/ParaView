/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScatterPlotPainter.h"

#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDefaultPainter.h"
#include "vtkDisplayListPainter.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkPainter.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkScatterPlotMapper.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkgl.h"

#include <assert.h>
#include <sstream>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkScatterPlotPainter);

vtkInformationKeyMacro(vtkScatterPlotPainter, THREED_MODE, Integer);
vtkInformationKeyMacro(vtkScatterPlotPainter, COLORIZE, Integer);
vtkInformationKeyMacro(vtkScatterPlotPainter, GLYPH_MODE, Integer);
vtkInformationKeyMacro(vtkScatterPlotPainter, SCALING_ARRAY_MODE, Integer);
vtkInformationKeyMacro(vtkScatterPlotPainter, SCALE_MODE, Integer);
vtkInformationKeyMacro(vtkScatterPlotPainter, SCALE_FACTOR, Double);
vtkInformationKeyMacro(vtkScatterPlotPainter, ORIENTATION_MODE, Integer);
vtkInformationKeyMacro(vtkScatterPlotPainter, NESTED_DISPLAY_LISTS, Integer);
vtkInformationKeyMacro(vtkScatterPlotPainter, PARALLEL_TO_CAMERA, Integer);

vtkCxxSetObjectMacro(vtkScatterPlotPainter, SourceGlyphMappers, vtkCollection);
vtkCxxSetObjectMacro(vtkScatterPlotPainter, LookupTable, vtkScalarsToColors);

template <class T>
static T vtkClamp(T val, T min, T max)
{
  val = val < min ? min : val;
  val = val > max ? max : val;
  return val;
}

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) > (b)) ? (b) : (a))

// ---------------------------------------------------------------------------
vtkScatterPlotPainter::vtkScatterPlotPainter()
{
  this->ThreeDMode = 0;
  this->Colorize = 0;
  this->GlyphMode = vtkScatterPlotMapper::NoGlyph;
  this->ScaleMode = vtkScatterPlotMapper::SCALE_BY_MAGNITUDE;
  this->ScaleFactor = 1.0;
  //   this->Range[0] = 0.0;
  //   this->Range[1] = 1.0;
  this->OrientationMode = vtkScatterPlotMapper::DIRECTION;
  this->NestedDisplayLists = 1;
  this->ParallelToCamera = 1;

  this->ScalarsToColorsPainter = vtkScalarsToColorsPainter::New();
  this->SourceGlyphMappers = vtkCollection::New();

  this->DisplayListId = 0; // for the matrices and color per glyph

  // this->Masking = false;
  this->SelectionColorId = 1;

  // this->PainterInformation = vtkInformation::New();
  this->LookupTable = NULL;
}

// ---------------------------------------------------------------------------
vtkScatterPlotPainter::~vtkScatterPlotPainter()
{
  if (this->SourceGlyphMappers != 0)
  {
    this->SourceGlyphMappers->Delete();
    this->SourceGlyphMappers = 0;
  }
  /* done in vtkPainter::~vtkPainter
    if (this->LastWindow)
      {
      this->ReleaseGraphicsResources(this->LastWindow);
      this->LastWindow = 0;
      }
  */
  if (this->ScalarsToColorsPainter)
  {
    this->ScalarsToColorsPainter->Delete();
    this->ScalarsToColorsPainter = 0;
  }

  if (this->LookupTable)
  {
    this->LookupTable->Delete();
    this->LookupTable = 0;
  }
  // this->PainterInformation->Delete();
  // this->PainterInformation = 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkScatterPlotPainter::GetInputArrayInformation(int idx)
{
  // add this info into the algorithms info object
  vtkInformationVector* inArrayVec =
    this->Information->Get(vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    inArrayVec = vtkInformationVector::New();
    this->Information->Set(vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS(), inArrayVec);
    inArrayVec->Delete();
  }
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    inArrayInfo = vtkInformation::New();
    inArrayVec->SetInformationObject(idx, inArrayInfo);
    inArrayInfo->Delete();
  }
  return inArrayInfo;
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkScatterPlotPainter::GetArray(int idx)
{
  vtkDataObject* object = this->GetInput();

  // vtkInformation* array = this->GetInputArrayInformation(idx);
  // vtkDataObject* object =
  //  this->GetInputDataObject(INPUTS_PORT, array->Get(INPUT_CONNECTION()));
  /*
  if(vtkCompositeDataSet::SafeDownCast(object))
    {
    vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(object);
    vtkSmartPointer<vtkCompositeDataIterator> it;
    it.TakeReference(composite->NewIterator());
    int piece = composite->GetUpdatePiece();
    while( piece-- )
      {
      it->GoToNextItem();
      }
    return this->GetArray(idx, vtkDataSet::SafeDownCast(
                                 composite->GetDataSet(it)));
    }
  */
  return this->GetArray(idx, vtkDataSet::SafeDownCast(object));
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkScatterPlotPainter::GetArray(int idx, vtkDataSet* input)
{
  // cout << "GetArray:" << idx << " " << input << " " << input->GetClassName() << endl;
  vtkDataArray* array = NULL;
  switch (idx)
  {
    case vtkScatterPlotMapper::Z_COORDS:
      if (!this->ThreeDMode)
      {
        return array;
      }
      break;
    case vtkScatterPlotMapper::COLOR:
      if (!this->Colorize)
      {
        return array;
      }
      break;
    case vtkScatterPlotMapper::GLYPH_X_SCALE:
    case vtkScatterPlotMapper::GLYPH_Y_SCALE:
    case vtkScatterPlotMapper::GLYPH_Z_SCALE:
      if (!(this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph))
      {
        return array;
      }
      break;
    case vtkScatterPlotMapper::GLYPH_SOURCE:
      if (!(this->GlyphMode & vtkScatterPlotMapper::UseMultiGlyph))
      {
        return array;
      }
      break;
    case vtkScatterPlotMapper::GLYPH_X_ORIENTATION:
    case vtkScatterPlotMapper::GLYPH_Y_ORIENTATION:
    case vtkScatterPlotMapper::GLYPH_Z_ORIENTATION:
      if (!(this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph))
      {
        return array;
      }
      break;
    default:
      break;
  }
  bool useCellData;
  vtkInformation* info = this->GetInputArrayInformation(idx);
  if (info->Has(vtkDataObject::FIELD_NAME()))
  {
    array = vtkDataArray::SafeDownCast(
      this->GetInputArrayToProcess(info->Get(vtkDataObject::FIELD_ASSOCIATION()),
        info->Get(vtkDataObject::FIELD_NAME()), input, &useCellData));
  }
  else if (info->Has(vtkDataObject::FIELD_ATTRIBUTE_TYPE()))
  {
    array = vtkDataArray::SafeDownCast(
      this->GetInputArrayToProcess(info->Get(vtkDataObject::FIELD_ASSOCIATION()),
        info->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE()), input, &useCellData));
  }
  else
  {
    vtkPointSet* pointSet = vtkPointSet::SafeDownCast(input);
    vtkPoints* points = pointSet ? pointSet->GetPoints() : NULL;
    array = points ? points->GetData() : NULL;
  }
  return array;
}

// ---------------------------------------------------------------------------
int vtkScatterPlotPainter::GetArrayComponent(int idx)
{
  vtkInformation* array = this->GetInputArrayInformation(idx);
  return array->Get(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT());
}

// ---------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
vtkPolyData* vtkScatterPlotPainter::GetGlyphSource(int id)
{
  if (!this->SourceGlyphMappers)
  {
    return NULL;
  }
  vtkPainterPolyDataMapper* mapper =
    vtkPainterPolyDataMapper::SafeDownCast(this->SourceGlyphMappers->GetItemAsObject(id));
  return mapper ? vtkPolyData::SafeDownCast(mapper->GetInput()) : NULL;
}

// ---------------------------------------------------------------------------
vtkUnsignedCharArray* vtkScatterPlotPainter::GetColors()
{
  return vtkUnsignedCharArray::SafeDownCast(
    vtkDataSet::SafeDownCast(this->ScalarsToColorsPainter->GetOutput())
      ->GetPointData()
      ->GetScalars());
}

//-----------------------------------------------------------------------------
void vtkScatterPlotPainter::ProcessInformation(vtkInformation* info)
{
  // the input arrays to process are handled automatically by the superclasses
  if (info->Has(THREED_MODE()))
  {
    this->SetThreeDMode(info->Get(THREED_MODE()));
  }

  if (info->Has(COLORIZE()))
  {
    this->SetColorize(info->Get(COLORIZE()));
  }

  if (info->Has(GLYPH_MODE()))
  {
    this->SetGlyphMode(info->Get(GLYPH_MODE()));
  }

  if (info->Has(SCALING_ARRAY_MODE()))
  {
    this->SetScalingArrayMode(info->Get(SCALING_ARRAY_MODE()));
  }

  if (info->Has(SCALE_MODE()))
  {
    this->SetScaleMode(info->Get(SCALE_MODE()));
  }

  if (info->Has(SCALE_FACTOR()))
  {
    this->SetScaleFactor(info->Get(SCALE_FACTOR()));
  }

  if (info->Has(ORIENTATION_MODE()))
  {
    this->SetOrientationMode(info->Get(ORIENTATION_MODE()));
  }

  if (info->Has(NESTED_DISPLAY_LISTS()))
  {
    this->SetNestedDisplayLists(info->Get(NESTED_DISPLAY_LISTS()));
  }

  if (info->Has(PARALLEL_TO_CAMERA()))
  {
    this->SetParallelToCamera(info->Get(PARALLEL_TO_CAMERA()));
  }

  if (info->Has(vtkScalarsToColorsPainter::LOOKUP_TABLE()))
  {
    vtkScalarsToColors* lut =
      vtkScalarsToColors::SafeDownCast(info->Get(vtkScalarsToColorsPainter::LOOKUP_TABLE()));
    if (lut)
    {
      this->SetLookupTable(lut);
    }
  }

  // when the iVars will be set, this->MTime will get updated.
  // This will eventually get caught by PrepareForRendering()
  // which will update the output. We need to discard old colors,
  // since some iVar that affects the color might have changed.
}

// ---------------------------------------------------------------------------
void vtkScatterPlotPainter::UpdatePainterInformation()
{
  if (this->GetMTime() < this->ColorPainterUpdateTime || !this->ScalarsToColorsPainter)
  {
    return;
  }

  if (this->Colorize)
  {
    vtkInformation* info = this->ScalarsToColorsPainter->GetInformation();
    //    vtkInformation* info = this->PainterInformation;
    vtkInformation* colorArrayInformation =
      this->GetInputArrayInformation(vtkScatterPlotMapper::COLOR);
    vtkDataArray* array = this->GetArray(vtkScatterPlotMapper::COLOR);
    if (!array)
    {
      return;
    }
    // info->Set(vtkPainter::STATIC_DATA(), this->Static);
    info->Set(vtkPainter::STATIC_DATA(), this->Information->Get(vtkPainter::STATIC_DATA()));
    info->Set(vtkScalarsToColorsPainter::USE_LOOKUP_TABLE_SCALAR_RANGE(),
      // this->GetUseLookupTableScalarRange());
      false);
    info->Set(vtkScalarsToColorsPainter::SCALAR_RANGE(),
      //          this->GetScalarRange(), 2);
      array->GetRange(), 2);
    if (colorArrayInformation->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
      (colorArrayInformation->Get(vtkDataObject::FIELD_ASSOCIATION()) ==
            vtkDataObject::FIELD_ASSOCIATION_POINTS ||
          colorArrayInformation->Get(vtkDataObject::FIELD_ASSOCIATION()) ==
            vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS))
    {
      info->Set(vtkScalarsToColorsPainter::SCALAR_MODE(), VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    }
    else
    {
      info->Set(vtkScalarsToColorsPainter::SCALAR_MODE(),
        // this->GetScalarMode());
        this->Information->Get(vtkScalarsToColorsPainter::SCALAR_MODE()));
    }
    // not sure yet
    info->Set(vtkScalarsToColorsPainter::COLOR_MODE(),
      // this->GetColorMode());
      this->Information->Get(vtkScalarsToColorsPainter::COLOR_MODE()));

    // if INTERPOLATE_SCALARS_BEFORE_MAPPING is set to 1, then
    // we would have to use texture coordinates: (vtkScalarsToColorsPainter
    // ->GetOutputData()->GetPointData()->GetScalars() will be NULL).
    info->Set(vtkScalarsToColorsPainter::INTERPOLATE_SCALARS_BEFORE_MAPPING(),
      //              this->GetInterpolateScalarsBeforeMapping());
      // this->Information->Get(vtkScalarsToColorsPainter::INTERPOLATE_SCALARS_BEFORE_MAPPING()));
      false);
    info->Set(vtkScalarsToColorsPainter::LOOKUP_TABLE(), this->GetLookupTable());
    info->Set(vtkScalarsToColorsPainter::SCALAR_VISIBILITY(),
      //          this->GetScalarVisibility());
      // 1);
      this->Information->Get(vtkScalarsToColorsPainter::SCALAR_VISIBILITY()));
    if (colorArrayInformation->Has(vtkDataObject::FIELD_ATTRIBUTE_TYPE()))
    {
      info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(), VTK_GET_ARRAY_BY_ID);
      info->Set(vtkScalarsToColorsPainter::ARRAY_ID(),
        colorArrayInformation->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE()));
      info->Remove(vtkScalarsToColorsPainter::ARRAY_NAME());
    }
    else if (colorArrayInformation->Has(vtkDataObject::FIELD_NAME()))
    {
      info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(), VTK_GET_ARRAY_BY_NAME);
      info->Set(vtkScalarsToColorsPainter::ARRAY_NAME(),
        colorArrayInformation->Get(vtkDataObject::FIELD_NAME()));
      info->Remove(vtkScalarsToColorsPainter::ARRAY_ID());
    }
    else
    {
      info->Remove(vtkScalarsToColorsPainter::ARRAY_ID());
      info->Remove(vtkScalarsToColorsPainter::ARRAY_NAME());
      info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(),
        // this->ArrayAccessMode);
        this->Information->Get(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE()));
    }
    info->Set(vtkScalarsToColorsPainter::ARRAY_COMPONENT(),
      colorArrayInformation->Get(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT()));
    // info->Set(vtkScalarsToColorsPainter::ARRAY_ID(), this->ArrayId);
    // info->Set(vtkScalarsToColorsPainter::ARRAY_NAME(), this->ArrayName);
    // info->Set(vtkScalarsToColorsPainter::ARRAY_COMPONENT(), this->ArrayComponent);
    info->Set(vtkScalarsToColorsPainter::SCALAR_MATERIAL_MODE(),
      // this->GetScalarMaterialMode());
      this->Information->Get(vtkScalarsToColorsPainter::SCALAR_MATERIAL_MODE()));
  }
  this->ColorPainterUpdateTime.Modified();
}

void vtkScatterPlotPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkMTimeType vtkScatterPlotPainter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  vtkDataArray* xCoordsArray = this->GetArray(vtkScatterPlotMapper::X_COORDS);
  vtkDataArray* yCoordsArray = this->GetArray(vtkScatterPlotMapper::Y_COORDS);
  vtkDataArray* zCoordsArray = this->GetArray(vtkScatterPlotMapper::Z_COORDS);
  vtkDataArray* colorArray = this->GetArray(vtkScatterPlotMapper::COLOR);
  if (xCoordsArray)
  {
    mTime = max(mTime, xCoordsArray->GetMTime());
  }
  if (yCoordsArray)
  {
    mTime = max(mTime, yCoordsArray->GetMTime());
  }
  if (this->ThreeDMode && zCoordsArray)
  {
    mTime = max(mTime, zCoordsArray->GetMTime());
  }
  if (this->Colorize && colorArray)
  {
    mTime = max(mTime, colorArray->GetMTime());
  }
  if (this->GlyphMode & vtkScatterPlotMapper::UseGlyph)
  {
    vtkDataArray* glyphXScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_X_SCALE);
    vtkDataArray* glyphYScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Y_SCALE);
    vtkDataArray* glyphZScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Z_SCALE);
    vtkDataArray* glyphSourceArray = this->GetArray(vtkScatterPlotMapper::GLYPH_SOURCE);
    vtkDataArray* glyphXOrientationArray =
      this->GetArray(vtkScatterPlotMapper::GLYPH_X_ORIENTATION);
    vtkDataArray* glyphYOrientationArray =
      this->GetArray(vtkScatterPlotMapper::GLYPH_Y_ORIENTATION);
    vtkDataArray* glyphZOrientationArray =
      this->GetArray(vtkScatterPlotMapper::GLYPH_Z_ORIENTATION);

    if (this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph && glyphXScaleArray)
    {
      mTime = max(mTime, glyphXScaleArray->GetMTime());
    }
    if (this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph &&
      this->ScalingArrayMode == vtkScatterPlotMapper::Xc_Yc_Zc && glyphYScaleArray)
    {
      mTime = max(mTime, glyphYScaleArray->GetMTime());
    }
    if (this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph &&
      this->ScalingArrayMode == vtkScatterPlotMapper::Xc_Yc_Zc && glyphZScaleArray)
    {
      mTime = max(mTime, glyphZScaleArray->GetMTime());
    }
    if (this->GlyphMode & vtkScatterPlotMapper::UseMultiGlyph && glyphSourceArray)
    {
      mTime = max(mTime, glyphSourceArray->GetMTime());
    }
    if (this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph && glyphXOrientationArray)
    {
      mTime = max(mTime, glyphXOrientationArray->GetMTime());
    }
    if (this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph && glyphYOrientationArray)
    {
      mTime = max(mTime, glyphYOrientationArray->GetMTime());
    }
    if (this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph && glyphZOrientationArray)
    {
      mTime = max(mTime, glyphZOrientationArray->GetMTime());
    }
  }
  return mTime;
}
//-------------------------------------------------------------------------
void vtkScatterPlotPainter::UpdateBounds(double* bounds)
{
  //  static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};
  // vtkMath::UninitializeBounds(bounds);
  // do we have an input
  if (!this->GetInput())
  {
    return;
  }

  if (this->InformationProcessTime < this->Information->GetMTime())
  {
    // If the information object was modified, some subclass may
    // want to get the modified information.
    // Using ProcessInformation avoids the need to access the Information
    // object during each UpdateBounds, thus reducing unnecessary
    // expensive information key accesses.
    this->ProcessInformation(this->Information);
    this->InformationProcessTime.Modified();
  }

  /*
    if (!this->Static)
      {
      // For proper clipping, this would be this->Piece,
      // this->NumberOfPieces.
      // But that removes all benefites of streaming.
      // Update everything as a hack for paraview streaming.
      // This should not affect anything else, because no one uses this.
      // It should also render just the same.
      // Just remove this lie if we no longer need streaming in paraview :)
      //Note: Not sure if it is still true
      //this->GetInput()->SetUpdateExtent(0, 1, 0);
      //this->GetInput()->Update();

      // first get the bounds from the input
      this->Update();
      }
  */
  // vtkDataSet *input = this->GetInput();
  // input->GetBounds(bounds);
  vtkDataArray* xArray = this->GetArray(vtkScatterPlotMapper::X_COORDS);
  vtkDataArray* yArray = this->GetArray(vtkScatterPlotMapper::Y_COORDS);
  vtkDataArray* zArray = this->GetArray(vtkScatterPlotMapper::Z_COORDS);

  if (xArray)
  {
    xArray->GetRange(bounds, this->GetArrayComponent(vtkScatterPlotMapper::X_COORDS));
  }
  if (yArray)
  {
    yArray->GetRange(bounds + 2, this->GetArrayComponent(vtkScatterPlotMapper::Y_COORDS));
  }
  if (zArray && this->ThreeDMode)
  {
    zArray->GetRange(bounds + 4, this->GetArrayComponent(vtkScatterPlotMapper::Z_COORDS));
  }
  else
  {
    bounds[4] = 0.;
    bounds[5] = 0.;
  }

  if (!(this->GlyphMode & vtkScatterPlotMapper::UseGlyph))
  {
    // cout << __FUNCTION__ << " bounds: "
    //        << bounds[0] << " " << bounds[1] << " "
    //        << bounds[2] << " " << bounds[3] << " "
    //        << bounds[4] << " " << bounds[5] << endl;
    return;
  }

  // if the input is not conform to what the mapper expects (use vector
  // but no vector data), nothing will be mapped.
  // It make sense to return uninitialized bounds.

  //   vtkDataArray *scaleArray = this->GetScaleArray(input);
  //   vtkDataArray *orientArray = this->GetOrientationArray(input);
  // TODO:
  // 1. cumulative bbox of all the glyph
  // 2. scale it by scale factor and maximum scalar value (or vector mag)
  // 3. enlarge the input bbox half-way in each direction with the
  // glyphs bbox.

  //   double den=this->Range[1]-this->Range[0];
  //   if(den==0.0)
  //     {
  //     den=1.0;
  //     }

  double maxScale[2] = { 1.0, 1.0 };

  vtkDataArray* glyphXScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_X_SCALE);
  vtkDataArray* glyphYScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Y_SCALE);
  vtkDataArray* glyphZScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Z_SCALE);

  if (this->GlyphMode && this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph && glyphXScaleArray)
  {
    double xScaleRange[2] = { 1.0, 1.0 };
    double yScaleRange[2] = { 1.0, 1.0 };
    double zScaleRange[2] = { 1.0, 1.0 };
    double x0ScaleRange[2] = { 1.0, 1.0 };
    double x1ScaleRange[2] = { 1.0, 1.0 };
    double x2ScaleRange[2] = { 1.0, 1.0 };
    double range[3];
    switch (this->ScaleMode)
    {
      case vtkScatterPlotMapper::SCALE_BY_MAGNITUDE:
        switch (this->ScalingArrayMode)
        {
          case vtkScatterPlotMapper::Xc_Yc_Zc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_SCALE));
            glyphYScaleArray->GetRange(
              yScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Y_SCALE));
            glyphZScaleArray->GetRange(
              zScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Z_SCALE));
            range[0] = xScaleRange[1];
            range[1] = yScaleRange[1];
            range[2] = zScaleRange[1];
            maxScale[1] = vtkMath::Norm(range);
            range[0] = xScaleRange[0];
            range[1] = yScaleRange[0];
            range[2] = zScaleRange[0];
            maxScale[0] = vtkMath::Norm(range);
            break;
          case vtkScatterPlotMapper::Xc0_Xc1_Xc2:
            glyphXScaleArray->GetRange(x0ScaleRange, 0);
            glyphXScaleArray->GetRange(x1ScaleRange, 1);
            glyphXScaleArray->GetRange(x2ScaleRange, 2);
            range[0] = x0ScaleRange[1];
            range[1] = x1ScaleRange[1];
            range[2] = x2ScaleRange[1];
            maxScale[1] = vtkMath::Norm(range, 3);
            range[0] = x0ScaleRange[0];
            range[1] = x1ScaleRange[0];
            range[2] = x2ScaleRange[0];
            maxScale[0] = vtkMath::Norm(range, 3);
            break;
          case vtkScatterPlotMapper::Xc_Xc_Xc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_SCALE));
            range[0] = xScaleRange[1];
            range[1] = xScaleRange[1];
            range[2] = xScaleRange[1];
            maxScale[1] = vtkMath::Norm(range, 3);
            range[0] = xScaleRange[0];
            range[1] = xScaleRange[0];
            range[2] = xScaleRange[0];
            maxScale[0] = vtkMath::Norm(range, 3);
            break;
          default:
            vtkErrorMacro("Wrong ScalingArray mode");
            break;
        }
        break;
      case vtkScatterPlotMapper::SCALE_BY_COMPONENTS:
        switch (this->ScalingArrayMode)
        {
          case vtkScatterPlotMapper::Xc_Yc_Zc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_SCALE));
            glyphYScaleArray->GetRange(
              yScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Y_SCALE));
            glyphZScaleArray->GetRange(
              zScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Z_SCALE));
            maxScale[0] = min(xScaleRange[0], yScaleRange[0]);
            maxScale[0] = min(maxScale[0], zScaleRange[0]);
            maxScale[1] = max(xScaleRange[1], yScaleRange[1]);
            maxScale[1] = max(maxScale[1], zScaleRange[1]);
            break;
          case vtkScatterPlotMapper::Xc0_Xc1_Xc2:
            glyphXScaleArray->GetRange(x0ScaleRange, 0);
            glyphXScaleArray->GetRange(x1ScaleRange, 1);
            glyphXScaleArray->GetRange(x2ScaleRange, 2);
            maxScale[0] = min(x0ScaleRange[0], x1ScaleRange[0]);
            maxScale[0] = min(maxScale[0], x2ScaleRange[0]);
            maxScale[1] = max(x0ScaleRange[1], x1ScaleRange[1]);
            maxScale[1] = max(maxScale[1], x2ScaleRange[1]);
            break;
          case vtkScatterPlotMapper::Xc_Xc_Xc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_SCALE));
            maxScale[0] = min(xScaleRange[0], xScaleRange[0]);
            maxScale[0] = min(maxScale[0], xScaleRange[0]);
            maxScale[1] = max(xScaleRange[1], xScaleRange[1]);
            maxScale[1] = max(maxScale[1], xScaleRange[1]);
            break;
          default:
            vtkErrorMacro("Wrong ScalingArray mode");
            break;
        }
        break;

      default:
        // NO_DATA_SCALING: do nothing, set variables to avoid warnings.
        break;
    }
  }
  //     if (this->Clamping && this->ScaleMode != NO_DATA_SCALING)
  //       {
  //       xScaleRange[0]=vtkMath::ClampAndNormalizeValue(xScaleRange[0],
  //                                                      this->Range);
  //       xScaleRange[1]=vtkMath::ClampAndNormalizeValue(xScaleRange[1],
  //                                                      this->Range);
  //       yScaleRange[0]=vtkMath::ClampAndNormalizeValue(yScaleRange[0],
  //                                                      this->Range);
  //       yScaleRange[1]=vtkMath::ClampAndNormalizeValue(yScaleRange[1],
  //                                                      this->Range);
  //       zScaleRange[0]=vtkMath::ClampAndNormalizeValue(zScaleRange[0],
  //                                                      this->Range);
  //       zScaleRange[1]=vtkMath::ClampAndNormalizeValue(zScaleRange[1],
  //                                                      this->Range);
  //       }
  //     }
  //   // FB
  /*
     if(this->GetGlyphSource(0) == 0)
       {
       this->GenerateDefaultGlyphs();
       }
  */
  int indexRange[2] = { 0, 0 };
  int numberOfGlyphSources = this->SourceGlyphMappers->GetNumberOfItems();
  vtkDataArray* glyphSourceArray = this->GetArray(vtkScatterPlotMapper::GLYPH_SOURCE);
  // Compute indexRange.
  double sourceRange[2] = { 0., 1. };
  double sourceRangeDiff = 1.;
  if (glyphSourceArray && this->GlyphMode & vtkScatterPlotMapper::UseMultiGlyph)
  {
    glyphSourceArray->GetRange(
      sourceRange, this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_SOURCE));
    sourceRangeDiff = sourceRange[1] - sourceRange[0];
    if (sourceRangeDiff == 0.0)
    {
      sourceRangeDiff = 1.0;
    }
    double range[2];
    glyphSourceArray->GetRange(range, -1);
    for (int i = 0; i < 2; ++i)
    {
      indexRange[i] =
        static_cast<int>(((range[i] - sourceRange[0]) / sourceRangeDiff) * numberOfGlyphSources);
      indexRange[i] = ::vtkClamp(indexRange[i], 0, numberOfGlyphSources - 1);
    }
  }

  vtkBoundingBox bbox; // empty

  for (int index = indexRange[0]; index <= indexRange[1]; ++index)
  {
    vtkPolyData* source = this->GetGlyphSource(index);
    // Make sure we're not indexing into empty glyph
    if (source == 0)
    {
      continue;
    }
    double sourcebounds[6];
    source->GetBounds(sourcebounds); // can be invalid/uninitialized
    if (vtkMath::AreBoundsInitialized(sourcebounds))
    {
      bbox.AddBounds(sourcebounds);
    }
  }

  if (this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph)
  {
    vtkBoundingBox bbox2(bbox);
    bbox.Scale(maxScale[0], maxScale[0], maxScale[0]);
    bbox2.Scale(maxScale[1], maxScale[1], maxScale[1]);
    bbox.AddBox(bbox2);
  }
  bbox.Scale(this->ScaleFactor, this->ScaleFactor, this->ScaleFactor);

  if (bbox.IsValid())
  {
    double tmpBounds[6];
    if (this->GlyphMode && this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph &&
      this->GetArray(vtkScatterPlotMapper::GLYPH_X_ORIENTATION))
    {
      // bounding sphere.
      double c[3];
      bbox.GetCenter(c);
      double l = bbox.GetDiagonalLength() / 2.0;
      tmpBounds[0] = c[0] - l;
      tmpBounds[1] = c[0] + l;
      tmpBounds[2] = c[1] - l;
      tmpBounds[3] = c[1] + l;
      tmpBounds[4] = c[2] - l;
      tmpBounds[5] = c[2] + l;
    }
    else
    {
      bbox.GetBounds(tmpBounds);
    }

    for (int j = 0; j < 6; ++j)
    {
      bounds[j] += tmpBounds[j];
    }
  }
  else
  {
    double tmpBounds[6];
    bbox.GetBounds(tmpBounds);
    vtkMath::UninitializeBounds(bounds);
  }
}
// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkScatterPlotPainter::ReleaseGraphicsResources(vtkWindow* window)
{
  // this->ReleaseDisplayList();
  size_t count = this->SourceGlyphMappers ? this->SourceGlyphMappers->GetNumberOfItems() : 0;
  for (size_t i = 0; i < count; ++i)
  {
    vtkPainterPolyDataMapper* mapper = vtkPainterPolyDataMapper::SafeDownCast(
      this->SourceGlyphMappers->GetItemAsObject(static_cast<int>(i)));
    if (mapper)
    {
      mapper->ReleaseGraphicsResources(window);
    }
  }
  this->Superclass::ReleaseGraphicsResources(window);
}

// ---------------------------------------------------------------------------
// Description:
// Release display list used for matrices and color.
void vtkScatterPlotPainter::ReleaseDisplayList()
{
  if (this->DisplayListId > 0)
  {
    vtkOpenGLClearErrorMacro();
    glDeleteLists(this->DisplayListId, 1);
    this->DisplayListId = 0;
    vtkOpenGLCheckErrorMacro("failed after ReleaseDisplayList");
  }
}

//-----------------------------------------------------------------------------
void vtkScatterPlotPainter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ScalarsToColorsPainter, "ScalarsToColorsPainter");
}

//-----------------------------------------------------------------------------
void vtkScatterPlotPainter::PrepareForRendering(vtkRenderer* ren, vtkActor* actor)
{
  // Get the color ready
  this->UpdatePainterInformation();
  // vtkInformation* array = this->GetInputArrayInformation(vtkScatterPlotMapper::COLOR);
  // vtkDataObject* object = this->GetInputDataObject(
  //  INPUTS_PORT,array->Get(INPUT_CONNECTION()));

  vtkDataObject* object = this->GetInput();
  /*
    if(vtkCompositeDataSet::SafeDownCast(object))
      {
      vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(object);
      vtkCompositeDataIterator* iterator = composite->NewIterator();
      this->ScalarsToColorsPainter->SetInput(
      composite->GetDataSet(iterator));
      iterator->Delete();
      }
      else
      {
  */
  if (this->Colorize)
  {
    this->ScalarsToColorsPainter->SetInput(object);
    /* } */

    this->ScalarsToColorsPainter->Render(ren, actor, 0xff, true);
  }

  /* done in vtkScatterPlotMapper because it's a pb when using displaylists
  size_t count = this->SourceGlyphMappers ?
    this->SourceGlyphMappers->GetNumberOfItems() : 0;
  for(size_t i = 0; i < count; ++i)
    {
    vtkPainterPolyDataMapper* mapper =
      vtkPainterPolyDataMapper::SafeDownCast(
        this->SourceGlyphMappers->GetItemAsObject(i));
    if(mapper)
      {
      mapper->SetForceCompileOnly(1);
      mapper->Render(ren, actor); // compile display list.
      mapper->SetForceCompileOnly(0);
      }
    }
  */
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor
// as each frame is rendered.
void vtkScatterPlotPainter::RenderInternal(
  vtkRenderer* ren, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly)
{
  this->Timer->StartTimer();
  if (this->GlyphMode & vtkScatterPlotMapper::UseGlyph)
  {
    this->RenderGlyphs(ren, actor, typeflags, forceCompileOnly);
  }
  else
  {
    this->RenderPoints(ren, actor, typeflags, forceCompileOnly);
  }
  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
}

//-----------------------------------------------------------------------------
void vtkScatterPlotPainter::RenderPoints(vtkRenderer* vtkNotUsed(ren), vtkActor* vtkNotUsed(actor),
  unsigned long vtkNotUsed(typeflags), bool vtkNotUsed(forceCompileOnly))
{
  vtkOpenGLClearErrorMacro();

  // cout << "render points" << endl;
  vtkDataArray* xCoordsArray = this->GetArray(vtkScatterPlotMapper::X_COORDS);
  vtkDataArray* yCoordsArray = this->GetArray(vtkScatterPlotMapper::Y_COORDS);
  vtkDataArray* zCoordsArray =
    this->ThreeDMode ? this->GetArray(vtkScatterPlotMapper::Z_COORDS) : NULL;
  vtkDataArray* colorArray = this->Colorize ? this->GetArray(vtkScatterPlotMapper::COLOR) : NULL;

  if (!xCoordsArray)
  {
    vtkErrorMacro("The X coord array is not set.");
    return;
  }

  if (!yCoordsArray)
  {
    vtkErrorMacro("The Y coord array is not set.");
    return;
  }

  if (!zCoordsArray && this->ThreeDMode)
  {
    vtkWarningMacro("The Z coord array is not set.");
  }

  if (!colorArray && this->Colorize)
  {
    vtkWarningMacro("The color array is not set.");
  }
  /*
  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
                                       vtkDataObject::FIELD_ASSOCIATION_POINTS);
  */
  // COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR
  //   COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR
  //      COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR
  vtkUnsignedCharArray* colors = colorArray ? this->GetColors() : NULL;

  vtkIdType numPts = xCoordsArray->GetNumberOfTuples();
  if (numPts < 1)
  {
    vtkDebugMacro(<< "No points to glyph!");
    return;
  }
  int Xc = this->GetArrayComponent(vtkScatterPlotMapper::X_COORDS);
  int Yc = this->GetArrayComponent(vtkScatterPlotMapper::Y_COORDS);
  int Zc = this->GetArrayComponent(vtkScatterPlotMapper::Z_COORDS);

  double point[3] = { 0., 0., 0. };
  glDisable(GL_LIGHTING);
  glBegin(GL_POINTS);
  // Traverse all Input points, transforming Source points
  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
  {
    if (!(inPtId % 10000))
    {
      this->UpdateProgress(static_cast<double>(inPtId) / static_cast<double>(numPts));
      /*if (this->GetAbortExecute())
        {
        cout << "abort" << endl;
        break;
        }
      */
    }

    // COLORING
    // Copy scalar value
    /*    if(selecting_points)
      {
      selector->RenderAttributeId(inPtId);
      }
      else*/ if (colors)
    {
      unsigned char rgba[4];
      colors->GetTypedTuple(inPtId, rgba);
      glColor4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    // translate Source to Input point
    // input->GetPoint(inPtId, point);
    point[0] = xCoordsArray->GetTuple(inPtId)[Xc];
    point[1] = yCoordsArray->GetTuple(inPtId)[Yc];
    if (zCoordsArray)
    {
      point[2] = zCoordsArray->GetTuple(inPtId)[Zc];
    }
    glVertex3f(point[0], point[1], point[2]);
  }
  // glVertex3f(0, 0, 0);
  glEnd();

  vtkOpenGLCheckErrorMacro("failed after RenderPoints");
}

//-----------------------------------------------------------------------------
void vtkScatterPlotPainter::RenderGlyphs(vtkRenderer* ren, vtkActor* actor,
  unsigned long vtkNotUsed(typeflags), bool vtkNotUsed(forceCompileOnly))
{
  vtkOpenGLClearErrorMacro();

  vtkDataArray* xCoordsArray = this->GetArray(vtkScatterPlotMapper::X_COORDS);
  vtkDataArray* yCoordsArray = this->GetArray(vtkScatterPlotMapper::Y_COORDS);
  vtkDataArray* zCoordsArray =
    this->ThreeDMode ? this->GetArray(vtkScatterPlotMapper::Z_COORDS) : NULL;
  vtkDataArray* colorArray = this->Colorize ? this->GetArray(vtkScatterPlotMapper::COLOR) : NULL;
  vtkDataArray* glyphXScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_X_SCALE);
  vtkDataArray* glyphYScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Y_SCALE);
  vtkDataArray* glyphZScaleArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Z_SCALE);
  vtkDataArray* glyphSourceArray = this->GetArray(vtkScatterPlotMapper::GLYPH_SOURCE);
  vtkDataArray* glyphXOrientationArray = this->GetArray(vtkScatterPlotMapper::GLYPH_X_ORIENTATION);
  vtkDataArray* glyphYOrientationArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Y_ORIENTATION);
  vtkDataArray* glyphZOrientationArray = this->GetArray(vtkScatterPlotMapper::GLYPH_Z_ORIENTATION);

  if (!xCoordsArray)
  {
    vtkErrorMacro("The X coord array is not set.");
    return;
  }

  if (!yCoordsArray)
  {
    vtkErrorMacro("The Y coord array is not set.");
    return;
  }

  if (!zCoordsArray && this->ThreeDMode)
  {
    vtkWarningMacro("The Z coord array is not set.");
  }

  if (!colorArray && this->Colorize)
  {
    vtkWarningMacro("The color array is not set.");
  }

  if ((!glyphXScaleArray && this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph) ||
    (!glyphYScaleArray && this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph &&
        this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_SCALE) != -1) ||
    (!glyphZScaleArray && this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph &&
        this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_SCALE) != -1))
  {
    vtkWarningMacro("The glyph scale array is not set.");
  }

  if (!glyphSourceArray && this->GlyphMode & vtkScatterPlotMapper::UseMultiGlyph)
  {
    vtkWarningMacro("The glyph source array is not set.");
  }

  if ((!glyphXOrientationArray && this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph) ||
    (!glyphYOrientationArray && this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph) ||
    (!glyphZOrientationArray && this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph))
  {
    vtkWarningMacro("The glyph orientation array is not set.");
  }
  /*
  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
                                       vtkDataObject::FIELD_ASSOCIATION_POINTS);
  */
  int Xc = this->GetArrayComponent(vtkScatterPlotMapper::X_COORDS);
  int Yc = this->GetArrayComponent(vtkScatterPlotMapper::Y_COORDS);
  int Zc = this->GetArrayComponent(vtkScatterPlotMapper::Z_COORDS);
  int SXc = this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_SCALE);
  int SYc = this->ScalingArrayMode == vtkScatterPlotMapper::Xc_Yc_Zc
    ? this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Y_SCALE)
    : 0;
  int SZc = this->ScalingArrayMode == vtkScatterPlotMapper::Xc_Yc_Zc
    ? this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Z_SCALE)
    : 0;
  int SOc = this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_SOURCE);
  int OXc = this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_X_ORIENTATION);
  int OYc = this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Y_ORIENTATION);
  int OZc = this->GetArrayComponent(vtkScatterPlotMapper::GLYPH_Z_ORIENTATION);

  // Check input for consistency
  //
  double sourceRange[2] = { 0., 1. };
  double sourceRangeDiff = 1.;
  if (glyphSourceArray)
  {
    glyphSourceArray->GetRange(sourceRange, SOc);
    sourceRangeDiff = sourceRange[1] - sourceRange[0];
    if (sourceRangeDiff == 0.0)
    {
      sourceRangeDiff = 1.0;
    }
  }

  // int numberOfGlyphSources = this->GetNumberOfInputConnections(GLYPHS_PORT);
  int numberOfGlyphSources =
    this->SourceGlyphMappers ? this->SourceGlyphMappers->GetNumberOfItems() : 0;

  // COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR
  //   COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR
  //      COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR
  vtkUnsignedCharArray* colors = this->Colorize ? this->GetColors() : NULL;

  vtkCamera* cam = ren->GetActiveCamera();
  double camRot[4];
  cam->GetViewTransformObject()->GetOrientationWXYZ(camRot);
  vtkTransform* camTrans = vtkTransform::New();
  camTrans->RotateWXYZ(camRot[0], camRot[1], camRot[2], camRot[3]);
  camTrans->Inverse();

  vtkTransform* trans = vtkTransform::New();

  // vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numPts = xCoordsArray->GetNumberOfTuples();
  if (numPts < 1)
  {
    vtkDebugMacro(<< "No points to glyph!");
    return;
  }
  glMatrixMode(GL_MODELVIEW);
  // Traverse all Input points, transforming Source points
  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
  {
    if (!(inPtId % 10000))
    {
      this->UpdateProgress(static_cast<double>(inPtId) / static_cast<double>(numPts));
      /*
      if (this->GetAbortExecute())
        {
        break;
        }
      */
    }

    //      if (maskArray && maskArray->GetValue(inPtId)==0)
    //        {
    //        continue;
    //        }

    // TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE
    //   TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE
    //      TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE
    // translate Source to Input point
    double point[3] = { 0., 0., 0. };
    // input->GetPoint(inPtId, point);
    point[0] = xCoordsArray->GetTuple(inPtId)[Xc];
    point[1] = yCoordsArray->GetTuple(inPtId)[Yc];
    if (this->ThreeDMode)
    {
      point[2] = zCoordsArray->GetTuple(inPtId)[Zc];
    }

    // SCALING  SCALING  SCALING  SCALING  SCALING  SCALING  SCALING
    //   SCALING  SCALING  SCALING  SCALING  SCALING  SCALING  SCALING
    //      SCALING  SCALING  SCALING  SCALING  SCALING  SCALING  SCALING
    double scale[3] = { 1., 1., 1. };
    if (this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph)
    {
      // Get the scalar and vector data
      double* xTuple = glyphXScaleArray->GetTuple(inPtId);
      double* yTuple = glyphYScaleArray ? glyphYScaleArray->GetTuple(inPtId) : NULL;
      double* zTuple = glyphZScaleArray ? glyphZScaleArray->GetTuple(inPtId) : NULL;
      switch (this->ScaleMode)
      {
        case vtkScatterPlotMapper::SCALE_BY_MAGNITUDE:
          switch (this->ScalingArrayMode)
          {
            case vtkScatterPlotMapper::Xc_Yc_Zc:
              scale[0] = xTuple[SXc];
              scale[1] = yTuple[SYc];
              scale[2] = zTuple[SZc];
              scale[0] = scale[1] = scale[2] = vtkMath::Norm(scale);
              break;
            case vtkScatterPlotMapper::Xc0_Xc1_Xc2:
              scale[0] = scale[1] = scale[2] =
                vtkMath::Norm(xTuple + SXc, glyphXScaleArray->GetNumberOfComponents());
              break;
            case vtkScatterPlotMapper::Xc_Xc_Xc:
              scale[0] = scale[1] = scale[2] = xTuple[SXc];
              break;
            default:
              vtkErrorMacro("Wrong ScalingArray mode");
              break;
          }
          break;
        case vtkScatterPlotMapper::SCALE_BY_COMPONENTS:
          switch (this->ScalingArrayMode)
          {
            case vtkScatterPlotMapper::Xc_Yc_Zc:
              scale[0] = xTuple[SXc];
              scale[1] = yTuple[SYc];
              scale[2] = zTuple[SZc];
              break;
            case vtkScatterPlotMapper::Xc0_Xc1_Xc2:
              if (glyphXScaleArray->GetNumberOfComponents() < 3)
              {
                vtkErrorMacro("Cannot scale by components since "
                  << glyphXScaleArray->GetName() << " does not have at least 3 components.");
              }
              scale[0] = xTuple[SXc];
              scale[1] = xTuple[SXc + 1];
              scale[2] = xTuple[SXc + 2];
              break;
            case vtkScatterPlotMapper::Xc_Xc_Xc:
              scale[0] = scale[1] = scale[2] = xTuple[SXc];
              break;
            default:
              vtkErrorMacro("Wrong ScalingArray mode");
              break;
          }
          break;
        default:
          vtkErrorMacro("Wrong Scale mode");
          break;
      }

      // Clamp data scale if enabled
      //        if (this->Clamping && this->ScaleMode != NO_DATA_SCALING)
      //          {
      //          scalex = (scalex < this->Range[0] ? this->Range[0] :
      //                    (scalex > this->Range[1] ? this->Range[1] : scalex));
      //          scalex = (scalex - this->Range[0]) / den;
      //          scaley = (scaley < this->Range[0] ? this->Range[0] :
      //                    (scaley > this->Range[1] ? this->Range[1] : scaley));
      //          scaley = (scaley - this->Range[0]) / den;
      //          scalez = (scalez < this->Range[0] ? this->Range[0] :
      //                    (scalez > this->Range[1] ? this->Range[1] : scalez));
      //          scalez = (scalez - this->Range[0]) / den;
      //          }
    }

    scale[0] *= this->ScaleFactor;
    scale[1] *= this->ScaleFactor;
    scale[2] *= this->ScaleFactor;

    if (scale[0] == 0.0)
    {
      scale[0] = 1.0e-10;
    }
    if (scale[1] == 0.0)
    {
      scale[1] = 1.0e-10;
    }
    if (scale[2] == 0.0)
    {
      scale[2] = 1.0e-10;
    }

    // MULTI  MULTI  MULTI  MULTI  MULTI  MULTI  MULTI MULTI  MULTI
    //   MULTI  MULTI  MULTI  MULTI  MULTI  MULTI  MULTI MULTI  MULTI
    //      MULTI  MULTI  MULTI  MULTI  MULTI  MULTI  MULTI MULTI  MULTI
    int index = 0;
    // Compute index into table of glyphs
    if (this->GlyphMode & vtkScatterPlotMapper::UseMultiGlyph && glyphSourceArray)
    {
      double value = 0.;
      if (SOc == -1)
      {
        value = vtkMath::Norm(
          glyphSourceArray->GetTuple(inPtId), glyphSourceArray->GetNumberOfComponents());
      }
      else
      {
        value = glyphSourceArray->GetTuple(inPtId)[SOc];
      }
      index = static_cast<int>(((value - sourceRange[0]) / sourceRangeDiff) * numberOfGlyphSources);
      index = ::vtkClamp(index, 0, numberOfGlyphSources - 1);
    }

    // source can be null.
    vtkPolyData* source = this->GetGlyphSource(index);

    // Make sure we're not indexing into empty glyph
    if (!source)
    {
      vtkErrorMacro("Glyph " << index << " is not set");
    }

    // ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION
    //   ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION
    //      ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION
    double orientation[3] = { 0., 0., 0. };
    if (this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph)
    {
      if (OXc == -1)
      {
        glyphXOrientationArray->GetTuple(inPtId, orientation);
      }
      else
      {
        orientation[0] = glyphXOrientationArray->GetTuple(inPtId)[OXc];
        orientation[1] = glyphYOrientationArray->GetTuple(inPtId)[OYc];
        orientation[2] = glyphZOrientationArray->GetTuple(inPtId)[OZc];
      }
    }

    // Now begin copying/transforming glyph
    trans->Identity();

    // TRANSLATION
    trans->Translate(point);

    // Get the 2D glyphs parallel to the camera
    if (this->ThreeDMode && this->GlyphMode & vtkScatterPlotMapper::UseGlyph &&
      this->ParallelToCamera)
    {
      trans->Concatenate(camTrans);
    }

    // ORIENTATION
    if (this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph)
    {
      switch (this->OrientationMode)
      {
        case vtkScatterPlotMapper::ROTATION:
          trans->RotateZ(orientation[2]);
          trans->RotateX(orientation[0]);
          trans->RotateY(orientation[1]);
          break;

        case vtkScatterPlotMapper::DIRECTION:
          if (orientation[1] == 0.0 && orientation[2] == 0.0)
          {
            if (orientation[0] < 0) // just flip x if we need to
            {
              trans->RotateWXYZ(180.0, 0, 1, 0);
            }
          }
          else
          {
            double vMag = vtkMath::Norm(orientation);
            double vNew[3];
            vNew[0] = (orientation[0] + vMag) / 2.0;
            vNew[1] = orientation[1] / 2.0;
            vNew[2] = orientation[2] / 2.0;
            trans->RotateWXYZ(180.0, vNew[0], vNew[1], vNew[2]);
          }
          break;
      }
    }

    // multiply points and normals by resulting matrix
    glPushMatrix();

    // COLORING
    // Copy scalar value
    /*if(selecting_points)
      {
      selector->RenderAttributeId(inPtId);
      }
      else*/ if (colors)
    {
      unsigned char rgba[4];
      colors->GetTypedTuple(inPtId, rgba);
      glColor4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    // SCALING
    if (this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph || this->ScaleFactor != 1.)
    {
      trans->Scale(scale[0], scale[1], scale[2]);
    }

    // Get the 2D glyphs parallel to the camera (billboarding)
    /*
    if(this->ThreeDMode)
      {
      float modelview[16];
      glTranslatef(point);
      glGetFloatv(GL_MODELVIEW_MATRIX , modelview);
      // undo all rotations
      // beware all scaling is lost as well
      for (int i = 0; i < 3; ++i)
        {
        for (int j=0; j < 3; ++j)
          {
          if ( i==j )
            {
            modelview[i*4+j] = 1.0;
            }
          else
            {
            modelview[i*4+j] = 0.0;
            }
          }
        }
      // set the modelview with no rotations and scaling
      glLoadMatrixf(modelview);
      }
    */
    double* mat = trans->GetMatrix()->Element[0];
    float mat2[16]; // transpose for OpenGL, float is native OpenGL
    // format.
    mat2[0] = static_cast<float>(mat[0]);
    mat2[1] = static_cast<float>(mat[4]);
    mat2[2] = static_cast<float>(mat[8]);
    mat2[3] = static_cast<float>(mat[12]);
    mat2[4] = static_cast<float>(mat[1]);
    mat2[5] = static_cast<float>(mat[5]);
    mat2[6] = static_cast<float>(mat[9]);
    mat2[7] = static_cast<float>(mat[13]);
    mat2[8] = static_cast<float>(mat[2]);
    mat2[9] = static_cast<float>(mat[6]);
    mat2[10] = static_cast<float>(mat[10]);
    mat2[11] = static_cast<float>(mat[14]);
    mat2[12] = static_cast<float>(mat[3]);
    mat2[13] = static_cast<float>(mat[7]);
    mat2[14] = static_cast<float>(mat[11]);
    mat2[15] = static_cast<float>(mat[15]);
    glMultMatrixf(mat2);

    vtkPainterPolyDataMapper* mapper = vtkPainterPolyDataMapper::SafeDownCast(
      this->SourceGlyphMappers->GetItemAsObject(static_cast<size_t>(index)));
    if (mapper)
    {
      mapper->Render(ren, actor);
    }
    // Don't know why but can't assume glMatrix(GL_MODELVIEW);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }

  trans->Delete();
  camTrans->Delete();

  vtkOpenGLCheckErrorMacro("failed after RenderGlyphs");
}
