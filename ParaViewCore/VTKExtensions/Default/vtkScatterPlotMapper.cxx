/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScatterPlotMapper.h"

#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
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
#include "vtkPainter.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkScatterPlotPainter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkTrivialProducer.h"
#include "vtkgl.h"

#include <assert.h>
#include <sstream>
#include <string>
#include <vector>

//#define PI 3.141592653589793

vtkStandardNewMacro(vtkScatterPlotMapper);

vtkInformationKeyMacro(vtkScatterPlotMapper, FIELD_ACTIVE_COMPONENT, Integer);

int INPUTS_PORT = 0;
int GLYPHS_PORT = 1;

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) > (b)) ? (b) : (a))

// ---------------------------------------------------------------------------
vtkScatterPlotMapper::vtkScatterPlotMapper()
{
  this->SetNumberOfInputPorts(2);
  vtkScatterPlotPainter* painter = vtkScatterPlotPainter::New();
  this->Painter->SetDelegatePainter(painter);
  painter->Delete();
  vtkDefaultPainter::SafeDownCast(this->Painter)->SetLightingPainter(0);
  vtkDefaultPainter::SafeDownCast(this->Painter)->SetRepresentationPainter(0);
  vtkDefaultPainter::SafeDownCast(this->Painter)->SetCoincidentTopologyResolutionPainter(0);

  this->ThreeDMode = false;
  this->Colorize = false;
  this->GlyphMode = vtkScatterPlotMapper::NoGlyph;
  this->ScaleMode = vtkScatterPlotMapper::SCALE_BY_MAGNITUDE;
  this->ScaleFactor = 1.0;
  this->OrientationMode = vtkScatterPlotMapper::DIRECTION;
  this->NestedDisplayLists = true;
  this->ParallelToCamera = false;
}

// ---------------------------------------------------------------------------
vtkScatterPlotMapper::~vtkScatterPlotMapper()
{
}

vtkScatterPlotPainter* vtkScatterPlotMapper::GetScatterPlotPainter()
{
  return vtkScatterPlotPainter::SafeDownCast(this->Painter->GetDelegatePainter());
}

//----------------------------------------------------------------------------
void vtkScatterPlotMapper::SetArrayByFieldIndex(
  ArrayIndex idx, int fieldIndex, int fieldAssociation, int component, int connection)
{
  vtkDataSet* input = vtkDataSet::SafeDownCast(this->GetInputDataObject(INPUTS_PORT, connection));
  if (!input || !input->GetPointData())
  {
    vtkErrorMacro("No vtkPointdata for input at the connection " << connection << ".");
  }

  vtkInformation* info = this->GetInputArrayInformation(idx);
  info->Set(INPUT_PORT(), INPUTS_PORT);
  info->Set(INPUT_CONNECTION(), connection);
  info->Set(vtkDataObject::FIELD_ASSOCIATION(), fieldAssociation);
  info->Set(vtkDataObject::FIELD_NAME(), input->GetPointData()->GetArrayName(fieldIndex));
  info->Remove(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
  /*
  this->SetInputArrayToProcess(idx, INPUTS_PORT, connection,
                               fieldAssociation, 
                               input->GetPointData()->GetArrayName(fieldIndex));
  vtkInformation* array = this->GetInputArrayInformation(idx);
  if(!array)
    {
    vtkErrorMacro("No vtkInformation for array" << idx << ".");
    return;
    }

    array*/ info->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(), component);
  this->Modified();
  if (this->GetScatterPlotPainter())
  {
    this->GetScatterPlotPainter()->GetInputArrayInformation(idx)->Copy(info, 1);
  }
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::SetArrayByFieldName(
  ArrayIndex idx, const char* arrayName, int fieldAssociation, int component, int connection)
{
  /*
  //this->SetInputArrayToProcess(vtkScatterPlotMapper::MASK, 0, 0,
  //  vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
  this->SetInputArrayToProcess(idx, INPUTS_PORT, connection,
                               fieldAssociation,
//                               vtkDataObject::FIELD_ASSOCIATION_POINTS,
//                               vtkDataObject::FIELD_ASSOCIATION_NONE,
                               arrayName);
  vtkInformation* array = this->GetInputArrayInformation(idx);
  if(!array)
    {
    vtkErrorMacro("No vtkInformation for array" << idx << ".");
    return;
    }
  array->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(),component);
*/
  // Similar at: this->SetInputArrayToProcess
  vtkInformation* info = this->GetInputArrayInformation(idx);
  info->Set(INPUT_PORT(), INPUTS_PORT);
  info->Set(INPUT_CONNECTION(), connection);
  info->Set(vtkDataObject::FIELD_ASSOCIATION(), fieldAssociation);
  info->Set(vtkDataObject::FIELD_NAME(), arrayName);
  info->Remove(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
  info->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(), component);
  this->Modified();
  if (this->GetScatterPlotPainter())
  {
    this->GetScatterPlotPainter()->GetInputArrayInformation(idx)->Copy(info, 1);
  }
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::SetArrayByFieldType(
  ArrayIndex idx, int fieldAttributeType, int fieldAssociation, int component, int connection)
{
  /*
  this->SetInputArrayToProcess(idx, INPUTS_PORT, connection,
    fieldAssociation, fieldAttributeType);
  vtkInformation* array = this->GetInputArrayInformation(idx);
  if(!array)
    {
    vtkErrorMacro("No vtkInformation for array" << idx << ".");
    return;
    }
  array->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(),component);
  */
  vtkInformation* info = this->GetInputArrayInformation(idx);
  info->Set(INPUT_PORT(), INPUTS_PORT);
  info->Set(INPUT_CONNECTION(), connection);
  info->Set(vtkDataObject::FIELD_ASSOCIATION(), fieldAssociation);
  info->Set(vtkDataObject::FIELD_ATTRIBUTE_TYPE(), fieldAttributeType);
  info->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(), component);
  // remove name if there is one
  info->Remove(vtkDataObject::FIELD_NAME());
  this->Modified();
  if (this->GetScatterPlotPainter())
  {
    this->GetScatterPlotPainter()->GetInputArrayInformation(idx)->Copy(info, 1);
  }
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::SetArrayByPointCoord(ArrayIndex idx, int component, int connection)
{
  vtkInformation* info = this->GetInputArrayInformation(idx);

  info->Set(INPUT_PORT(), INPUTS_PORT);
  info->Set(INPUT_CONNECTION(), connection);
  info->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(), component);

  info->Remove(vtkDataObject::FIELD_ASSOCIATION());
  info->Remove(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
  info->Remove(vtkDataObject::FIELD_NAME());

  this->Modified();
  if (this->GetScatterPlotPainter())
  {
    this->GetScatterPlotPainter()->GetInputArrayInformation(idx)->Copy(info, 1);
  }
}

void vtkScatterPlotMapper::SetArrayByName(ArrayIndex idx, const char* arrayName)
{
  std::string array(arrayName);

  // Skip delimiters at beginning.
  std::string::size_type lastPos = array.find_first_not_of(',', 0);
  // Find first "non-delimiter".
  std::string::size_type pos = array.find_first_of(',', lastPos);
  std::vector<std::string> tokens;

  while (std::string::npos != pos || std::string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(array.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = array.find_first_not_of(',', pos);
    // Find next "non-delimiter"
    pos = array.find_first_of(',', lastPos);
  }

  std::string arrayString = "";
  std::string arrayType = "";
  std::string arrayComponent = "";
  switch (tokens.size())
  {
    case 0:
      return;
      break;
    case 1:
      arrayType = "point";
      arrayString = tokens[0];
      break;
    default:
    case 3:
      arrayComponent = tokens[2];
    case 2:
      arrayType = tokens[0];
      arrayString = tokens[1];
      break;
  }

  int component = 0;
  if (arrayComponent.empty())
  {
    array = arrayString;
    std::size_t startParenthesis = array.find('(');
    arrayString = array.substr(0, startParenthesis);
    if (startParenthesis != std::string::npos)
    {
      std::size_t endParenthesis = array.find(')', arrayString.length());
      if (endParenthesis != std::string::npos)
      {
        std::stringstream componentString;
        componentString << array.substr(
          arrayString.length(), endParenthesis - arrayString.length());
        char parenthesis;
        componentString >> parenthesis >> component >> parenthesis;
      }
    }
  }
  else
  {
    std::stringstream componentString;
    componentString << arrayComponent;
    componentString >> component;
  }
  if (arrayType == "point")
  {
    this->SetArrayByFieldName(
      idx, arrayString.c_str(), vtkDataObject::FIELD_ASSOCIATION_POINTS, component);
  }
  else if (arrayType == "cell")
  {
    this->SetArrayByFieldName(
      idx, arrayString.c_str(), vtkDataObject::FIELD_ASSOCIATION_CELLS, component);
  }
  else if (arrayType == "coord")
  {
    this->SetArrayByPointCoord(idx, component);
  }
  else
  {
    vtkErrorMacro(<< "Wrong array type: " << arrayType);
  }
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkScatterPlotMapper::GetArray(vtkScatterPlotMapper::ArrayIndex idx)
{
  vtkInformation* array = this->GetInputArrayInformation(idx);
  vtkDataObject* object = this->GetInputDataObject(INPUTS_PORT, array->Get(INPUT_CONNECTION()));
  /*
  if(vtkCompositeDataSet::SafeDownCast(object))
    {
    vtkCompositeDataSet* composite =
      vtkCompositeDataSet::SafeDownCast(object);
    vtkSmartPointer<vtkCompositeDataIterator> it;
    it.TakeReference(composite->NewIterator());
    return this->GetArray(idx, vtkDataSet::SafeDownCast(
                            composite->GetDataSet(it)));
    }
  */
  return this->GetArray(idx, vtkDataSet::SafeDownCast(object));
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkScatterPlotMapper::GetArray(
  vtkScatterPlotMapper::ArrayIndex idx, vtkDataSet* input)
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
  vtkInformation* info = this->GetInputArrayInformation(idx);
  if (info->Has(vtkDataObject::FIELD_NAME()) || info->Has(vtkDataObject::FIELD_ATTRIBUTE_TYPE()))
  {
    array = this->GetInputArrayToProcess(idx, input);
  }
  else
  {
    if (vtkPointSet::SafeDownCast(input))
    {
      array = vtkPointSet::SafeDownCast(input)->GetPoints()->GetData();
    }
  }
  return array;
}

// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkScatterPlotMapper::SetGlyphSourceConnection(int id, vtkAlgorithmOutput* algOutput)
{
  if (id < 0)
  {
    vtkErrorMacro("Bad index " << id << " for source.");
    return;
  }

  int numConnections = this->GetNumberOfInputConnections(GLYPHS_PORT);
  if (id < numConnections)
  {
    this->SetNthInputConnection(GLYPHS_PORT, id, algOutput);
  }
  else if (id == numConnections && algOutput)
  {
    this->AddInputConnection(GLYPHS_PORT, algOutput);
  }
  else if (algOutput)
  {
    vtkWarningMacro("The source id provided is larger than the maximum "
                    "source id, using "
      << numConnections << " instead.");
    this->AddInputConnection(GLYPHS_PORT, algOutput);
  }
}
// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkScatterPlotMapper::AddGlyphSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->AddInputConnection(GLYPHS_PORT, algOutput);
}

// ---------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
vtkPolyData* vtkScatterPlotMapper::GetGlyphSource(int id)
{
  if (id < 0 || id >= this->GetNumberOfInputConnections(GLYPHS_PORT))
  {
    return NULL;
  }

  return vtkPolyData::SafeDownCast(this->GetInputDataObject(GLYPHS_PORT, id));
}

void vtkScatterPlotMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);
  vtkCompositeDataSet* input =
    vtkCompositeDataSet::SafeDownCast(this->GetInputDataObject(INPUTS_PORT, 0));

  if (this->GlyphMode & vtkScatterPlotMapper::UseGlyph)
  {
    if (this->GetGlyphSource(0) == 0)
    {
      this->GenerateDefaultGlyphs();
    }
    this->InitGlyphMappers(NULL, NULL);
  }

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if (!input)
  {
    this->GetScatterPlotPainter()->SetInput(this->GetInputDataObject(INPUTS_PORT, 0));
    this->Superclass::ComputeBounds();
    return;
  }

  this->Update();

  // We do have hierarchical data - so we need to loop over
  // it and get the total bounds.
  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();
  double bounds[6];
  int i;

  while (!iter->IsDoneWithTraversal())
  {
    this->GetScatterPlotPainter()->SetInput(iter->GetCurrentDataObject());

    // Update Painter information if obsolete.
    if (this->PainterUpdateTime < this->GetMTime())
    {
      this->UpdatePainterInformation();
      this->PainterUpdateTime.Modified();
    }

    if (vtkMath::AreBoundsInitialized(this->Bounds))
    {
      this->Painter->UpdateBounds(bounds);
      cout << "UpBounds: " << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3]
           << " " << bounds[4] << " " << bounds[5] << endl;
      for (i = 0; i < 3; i++)
      {
        this->Bounds[i * 2] =
          (bounds[i * 2] < this->Bounds[i * 2]) ? (bounds[i * 2]) : (this->Bounds[i * 2]);
        this->Bounds[i * 2 + 1] = (bounds[i * 2 + 1] > this->Bounds[i * 2 + 1])
          ? (bounds[i * 2 + 1])
          : (this->Bounds[i * 2 + 1]);
      }
    }
    else
    {
      this->Painter->UpdateBounds(this->Bounds);
      cout << "Bounds: " << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3]
           << " " << bounds[4] << " " << bounds[5] << endl;
    }

    iter->GoToNextItem();
  }
  iter->Delete();
  this->BoundsMTime.Modified();
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->GetNumberOfInputConnections(1) < 2)
  {
    if (this->GetGlyphSource(0) != NULL)
    {
      os << indent << "Source: (" << this->GetGlyphSource(0) << ")\n";
    }
    else
    {
      os << indent << "Source: (none)\n";
    }
  }
  else
  {
    os << indent << "A table of " << this->GetNumberOfInputConnections(1)
       << " glyphs has been defined\n";
  }

  //   os << indent << "Scaling: " << (this->Scaling ? "On\n" : "Off\n");

  //   os << indent << "Scale Mode: " << this->GetScaleModeAsString() << endl;
  //   os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  //   os << indent << "Clamping: " << (this->Clamping ? "On\n" : "Off\n");
  //   os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";
  //   os << indent << "Orient: " << (this->Orient ? "On\n" : "Off\n");
  //   os << indent << "OrientationMode: "
  //     << this->GetOrientationModeAsString() << "\n";
  //   os << indent << "SourceIndexing: "
  //     << (this->SourceIndexing? "On" : "Off") << endl;
  //   os << "Masking: " << (this->Masking? "On" : "Off") << endl;
}

// ---------------------------------------------------------------------------
int vtkScatterPlotMapper::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == INPUTS_PORT)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    // info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    // info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
    //             "vtkMultiBlockDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    return 1;
  }
  else if (port == GLYPHS_PORT)
  {
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }
  return 0;
}

/*
// ---------------------------------------------------------------------------
int vtkScatterPlotMapper::FillOutputPortInformation(int vtkNotUsed(port),
  vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 0;
}
*/
// ---------------------------------------------------------------------------
// Description:
// Send mapper ivars to sub-mapper.
// \pre mapper_exists: mapper!=0
void vtkScatterPlotMapper::CopyInformationToSubMapper(vtkPainterPolyDataMapper* mapper)
{
  assert("pre: mapper_exists" && mapper != 0);
  if (!mapper)
  {
    vtkErrorMacro("Mapper can't be NULL. ");
    return;
  }

  // see void vtkPainterPolyDataMapper::UpdatePainterInformation()

  mapper->SetStatic(this->Static);
  mapper->ScalarVisibilityOff();
  // not used
  // mapper->SetClippingPlanes(this->ClippingPlanes);

  mapper->SetResolveCoincidentTopology(this->GetResolveCoincidentTopology());
  mapper->SetResolveCoincidentTopologyZShift(this->GetResolveCoincidentTopologyZShift());

  // ResolveCoincidentTopologyPolygonOffsetParameters is static
  mapper->SetResolveCoincidentTopologyPolygonOffsetFaces(
    this->GetResolveCoincidentTopologyPolygonOffsetFaces());
  mapper->SetImmediateModeRendering(this->NestedDisplayLists); // this->ImmediateModeRendering
  // || vtkMapper::GetGlobalImmediateModeRendering());// ||
  //! this->NestedDisplayLists);
}

//-----------------------------------------------------------------------------
void vtkScatterPlotMapper::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // vtkGarbageCollectorReport(collector, this->Painter,
  //                          "Painter");
}

//-----------------------------------------------------------------------------
void vtkScatterPlotMapper::PrepareForRendering(vtkRenderer* ren, vtkActor* actor)
{
  if (this->GlyphMode & vtkScatterPlotMapper::UseGlyph)
  {
    // Get the Glyphs ready
    this->InitGlyphMappers(ren, actor);
  }
}

//-----------------------------------------------------------------------------
void vtkScatterPlotMapper::Render(vtkRenderer* ren, vtkActor* actor)
{
  this->PrepareForRendering(ren, actor);
  this->Superclass::Render(ren, actor);
}

//-----------------------------------------------------------------------------
void vtkScatterPlotMapper::UpdatePainterInformation()
{
  vtkInformation* info = this->PainterInformation;
  this->Superclass::UpdatePainterInformation();

  for (int idx = 0; idx < vtkScatterPlotMapper::NUMBER_OF_ARRAYS; ++idx)
  {
    vtkInformationVector* inArrayVec = info->Get(vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS());
    if (!inArrayVec)
    {
      inArrayVec = vtkInformationVector::New();
      info->Set(vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS(), inArrayVec);
      inArrayVec->Delete();
    }
    vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
    if (!inArrayInfo)
    {
      inArrayInfo = vtkInformation::New();
      inArrayVec->SetInformationObject(idx, inArrayInfo);
      inArrayInfo->Delete();
    }
    vtkInformation* arrayInfo = this->GetInputArrayInformation(idx);
    inArrayInfo->Copy(arrayInfo, 1);
  }

  info->Set(vtkScatterPlotPainter::THREED_MODE(), this->ThreeDMode);
  info->Set(vtkScatterPlotPainter::COLORIZE(), this->Colorize);
  info->Set(vtkScatterPlotPainter::GLYPH_MODE(), this->GlyphMode);
  info->Set(vtkScatterPlotPainter::SCALING_ARRAY_MODE(), this->ScalingArrayMode);
  info->Set(vtkScatterPlotPainter::SCALE_MODE(), this->ScaleMode);
  info->Set(vtkScatterPlotPainter::SCALE_FACTOR(), this->ScaleFactor);
  info->Set(vtkScatterPlotPainter::ORIENTATION_MODE(), this->OrientationMode);
  info->Set(vtkScatterPlotPainter::NESTED_DISPLAY_LISTS(), this->NestedDisplayLists);
  info->Set(vtkScatterPlotPainter::PARALLEL_TO_CAMERA(), this->ParallelToCamera);

  if (this->GlyphMode & vtkScatterPlotMapper::UseGlyph)
  {
    this->InitGlyphMappers(NULL, NULL);
  }
}

//-----------------------------------------------------------------------------
void vtkScatterPlotMapper::InitGlyphMappers(
  vtkRenderer* ren, vtkActor* actor, bool vtkNotUsed(createDisplayList))
{
  // Create a default source, if no source is specified.
  if (this->GetGlyphSource(0) == 0)
  {
    cout << __FUNCTION__ << ": default glyphs must have been initialized before" << endl;
  }

  // vtkScatterPlotMapperArray* glyphMappers =
  vtkCollection* glyphMappers = this->GetScatterPlotPainter()->GetSourceGlyphMappers();

  if (glyphMappers == 0)
  {
    // glyphMappers = new vtkScatterPlotMapperArray;
    glyphMappers = vtkCollection::New();
    this->GetScatterPlotPainter()->SetSourceGlyphMappers(glyphMappers);
    glyphMappers->Delete();
  }

  vtkDataArray* glyphSourceArray = this->GetArray(vtkScatterPlotMapper::GLYPH_SOURCE);

  size_t numberOfGlyphSources =
    glyphSourceArray ? this->GetNumberOfInputConnections(GLYPHS_PORT) : 1;

  for (size_t cc = 0; cc < numberOfGlyphSources; cc++)
  {
    // if (glyphMappers->Mappers[cc]==0)
    vtkPainterPolyDataMapper* polyDataMapper =
      vtkPainterPolyDataMapper::SafeDownCast(glyphMappers->GetItemAsObject(static_cast<int>(cc)));
    if (polyDataMapper == NULL)
    {
      // glyphMappers->Mappers[cc] = vtkPainterPolyDataMapper::New();
      // glyphMappers->Mappers[cc]->Delete();
      polyDataMapper = vtkPainterPolyDataMapper::New();
      glyphMappers->AddItem(polyDataMapper);
      polyDataMapper->Delete();

      vtkDefaultPainter* p = vtkDefaultPainter::SafeDownCast(polyDataMapper->GetPainter());
      p->SetScalarsToColorsPainter(0); // bypass default mapping.
      p->SetClipPlanesPainter(0);      // bypass default mapping.
      vtkHardwareSelectionPolyDataPainter::SafeDownCast(polyDataMapper->GetSelectionPainter())
        ->EnableSelectionOff();
      // use the same painter for selection pass as well.
    }
    // Copy mapper ivar to sub-mapper
    this->CopyInformationToSubMapper(polyDataMapper);

    // source can be null.
    vtkPolyData* source = this->GetGlyphSource(static_cast<int>(cc));
    vtkPolyData* ss = polyDataMapper->GetInput();
    if (ss == 0)
    {
      ss = vtkPolyData::New();
      polyDataMapper->SetInputData(ss);
      ss->Delete();
      ss->ShallowCopy(source);
    }
    else if (source && source->GetMTime() > ss->GetMTime())
    {
      ss->ShallowCopy(source);
    }

    if ( //! this->ImmediateModeRendering &&
      this->NestedDisplayLists && ren && actor)
    {
      polyDataMapper->SetForceCompileOnly(1);
      polyDataMapper->Render(ren, actor); // compile display list.
      polyDataMapper->SetForceCompileOnly(0);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkScatterPlotMapper::GenerateDefaultGlyphs()
{
  // create a diamond shape
  vtkPolyData* defaultSource = vtkPolyData::New();
  vtkPoints* defaultPoints = vtkPoints::New();
  int points = 16;
  vtkIdType* defaultPointIds = new vtkIdType[points + 1];
  for (int i = 0; i <= points; ++i)
  {
    defaultPointIds[i] = i;
  }
  // triangle
  defaultSource->Allocate();
  defaultPoints->Allocate(4);
  defaultPoints->InsertNextPoint(-0.2 * tan(vtkMath::Pi() / 6.), -0.1, 0);
  defaultPoints->InsertNextPoint(0, 0.1, 0);
  defaultPoints->InsertNextPoint(0.2 * tan(vtkMath::Pi() / 6.), -0.1, 0);
  defaultPoints->InsertNextPoint(-0.2 * tan(vtkMath::Pi() / 6.), -0.1, 0);
  defaultSource->SetPoints(defaultPoints);

  defaultSource->InsertNextCell(VTK_POLY_LINE, 4, defaultPointIds);
  vtkSmartPointer<vtkTrivialProducer> triangleProducer = vtkSmartPointer<vtkTrivialProducer>::New();
  triangleProducer->SetOutput(defaultSource);
  this->AddGlyphSourceConnection(triangleProducer->GetOutputPort());
  defaultSource->Delete();
  defaultPoints->Delete();

  // square
  defaultSource = vtkPolyData::New();
  defaultPoints = vtkPoints::New();
  defaultSource->Allocate();
  defaultPoints->Allocate(5);
  defaultPoints->InsertNextPoint(-0.1, -0.1, 0);
  defaultPoints->InsertNextPoint(-0.1, 0.1, 0);
  defaultPoints->InsertNextPoint(0.1, 0.1, 0);
  defaultPoints->InsertNextPoint(0.1, -0.1, 0);
  defaultPoints->InsertNextPoint(-0.1, -0.1, 0);
  defaultSource->SetPoints(defaultPoints);
  defaultSource->InsertNextCell(VTK_POLY_LINE, 5, defaultPointIds);
  vtkSmartPointer<vtkTrivialProducer> squareProducer = vtkSmartPointer<vtkTrivialProducer>::New();
  squareProducer->SetOutput(defaultSource);
  this->AddGlyphSourceConnection(squareProducer->GetOutputPort());
  defaultSource->Delete();
  defaultPoints->Delete();

  // pentagon
  defaultSource = vtkPolyData::New();
  defaultPoints = vtkPoints::New();
  defaultSource->Allocate();
  defaultPoints->Allocate(11);
  double angle5 = 2. * vtkMath::Pi() / 5.;
  defaultPoints->InsertNextPoint(0.0, 0.1, 0);
  defaultPoints->InsertNextPoint(0.05 * cos(0.5 * angle5 + vtkMath::Pi() / 2.),
    0.05 * sin(0.5 * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(
    0.1 * cos(angle5 + vtkMath::Pi() / 2.), 0.1 * sin(angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(0.05 * cos(1.5 * angle5 + vtkMath::Pi() / 2.),
    0.05 * sin(1.5 * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(
    0.1 * cos(2. * angle5 + vtkMath::Pi() / 2.), 0.1 * sin(2. * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(0.05 * cos(2.5 * angle5 + vtkMath::Pi() / 2.),
    0.05 * sin(2.5 * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(
    0.1 * cos(3. * angle5 + vtkMath::Pi() / 2.), 0.1 * sin(3. * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(0.05 * cos(3.5 * angle5 + vtkMath::Pi() / 2.),
    0.05 * sin(3.5 * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(
    0.1 * cos(4. * angle5 + vtkMath::Pi() / 2.), 0.1 * sin(4. * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(0.05 * cos(4.5 * angle5 + vtkMath::Pi() / 2.),
    0.05 * sin(4.5 * angle5 + vtkMath::Pi() / 2.), 0);
  defaultPoints->InsertNextPoint(0.0, 0.1, 0);
  defaultSource->SetPoints(defaultPoints);
  defaultSource->InsertNextCell(VTK_POLY_LINE, 11, defaultPointIds);
  vtkSmartPointer<vtkTrivialProducer> pentagonProducer = vtkSmartPointer<vtkTrivialProducer>::New();
  pentagonProducer->SetOutput(defaultSource);
  this->AddGlyphSourceConnection(pentagonProducer->GetOutputPort());
  defaultSource->Delete();
  defaultPoints->Delete();

  // circle
  defaultSource = vtkPolyData::New();
  defaultPoints = vtkPoints::New();
  defaultSource->Allocate();
  defaultPoints->Allocate(points + 1);
  double angle = 0.;
  double step = 2. * vtkMath::Pi() / points;
  for (int i = 0; i <= points; ++i)
  {
    angle = step * i;
    defaultPoints->InsertNextPoint(0.1 * cos(angle), 0.1 * sin(angle), 0);
  }
  defaultSource->SetPoints(defaultPoints);
  defaultSource->InsertNextCell(VTK_POLY_LINE, points + 1, defaultPointIds);
  vtkSmartPointer<vtkTrivialProducer> circleProducer = vtkSmartPointer<vtkTrivialProducer>::New();
  circleProducer->SetOutput(defaultSource);
  this->AddGlyphSourceConnection(circleProducer->GetOutputPort());
  defaultSource->Delete();
  defaultPoints->Delete();

  delete[] defaultPointIds;
}
