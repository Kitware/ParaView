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
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDefaultPainter.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkInformationIntegerKey.h"
#include "vtkDataSet.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"

#include "vtkgl.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>
#include <vtkstd/string>
//#include <sstream>
#include <assert.h>

#define PI 3.141592653589793

vtkCxxRevisionMacro(vtkScatterPlotMapper, "1.6");
vtkStandardNewMacro(vtkScatterPlotMapper);

vtkInformationKeyMacro(vtkScatterPlotMapper, FIELD_ACTIVE_COMPONENT, Integer);

template <class T>
static T vtkClamp(T val, T min, T max)
{
  val = val < min? min : val;
  val = val > max? max : val;
  return val;
}

class vtkScatterPlotMapperArray
{
public:
  vtkstd::vector<vtkSmartPointer<vtkPainterPolyDataMapper > > Mappers;
};

int INPUTS_PORT=0;
int GLYPHS_PORT=1;

#define max(a,b)(((a)>(b))?(a):(b))
#define min(a,b)(((a)>(b))?(b):(a))

// ---------------------------------------------------------------------------
// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
vtkScatterPlotMapper::vtkScatterPlotMapper()
{
  this->SetNumberOfInputPorts(2);

  this->ThreeDMode = false;
  this->Colorize = false;
  this->GlyphMode = vtkScatterPlotMapper::NoGlyph;
  this->ScaleMode = vtkScatterPlotMapper::SCALE_BY_MAGNITUDE;
  this->ScaleFactor = 1.0;
//   this->Range[0] = 0.0;
//   this->Range[1] = 1.0;
  this->OrientationMode = vtkScatterPlotMapper::DIRECTION;

  this->SourceMappers = 0;

  this->DisplayListId = 0; // for the matrices and color per glyph
  this->LastWindow = 0;

  this->NestedDisplayLists = true;

  //this->Masking = false;
  this->SelectionColorId=1;

  this->ScalarsToColorsPainter = vtkScalarsToColorsPainter::New();
  //this->PainterInformation = vtkInformation::New();
}

// ---------------------------------------------------------------------------
vtkScatterPlotMapper::~vtkScatterPlotMapper()
{
  if(this->SourceMappers!=0)
    {
    delete this->SourceMappers;
    this->SourceMappers=0;
    }

  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
    }
  if (this->ScalarsToColorsPainter)
    {
    this->ScalarsToColorsPainter->Delete();
    this->ScalarsToColorsPainter = 0;
    }
  //this->PainterInformation->Delete();
  //this->PainterInformation = 0;
}

void vtkScatterPlotMapper::SetArrayByFieldIndex(ArrayIndex idx, int fieldIndex, 
                                                int component, int connection)
{
  vtkDataSet* input = vtkDataSet::SafeDownCast( 
    this->GetInputDataObject(INPUTS_PORT, connection));
  if(!input || !input->GetPointData())
    {
    vtkErrorMacro("No vtkPointdata for input at the connection " << connection << ".");
    }
  this->SetInputArrayToProcess(idx, INPUTS_PORT, connection,
                               vtkDataObject::FIELD_ASSOCIATION_POINTS, 
                               input->GetPointData()->GetArrayName(fieldIndex));
  vtkInformation* array = this->GetInputArrayInformation(idx);
  if(!array)
    {
    vtkErrorMacro("No vtkInformation for array" << idx << ".");
    return;
    }
  array->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(),component);
  this->Modified();
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::SetArrayByFieldName(ArrayIndex idx, 
                                         const char* arrayName, 
                                         int component, int connection)
{
  //this->SetInputArrayToProcess(vtkScatterPlotMapper::MASK, 0, 0,
  //  vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
  this->SetInputArrayToProcess(idx, INPUTS_PORT, connection,
                               vtkDataObject::FIELD_ASSOCIATION_POINTS, 
//                               vtkDataObject::FIELD_ASSOCIATION_NONE, 
                               arrayName);
  vtkInformation* array = this->GetInputArrayInformation(idx);
  if(!array)
    {
    vtkErrorMacro("No vtkInformation for array" << idx << ".");
    return;
    }
  array->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(),component);
  this->Modified();
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::SetArrayByFieldType(ArrayIndex idx, 
                                         int fieldAttributeType, 
                                         int component, int connection)
{
  this->SetInputArrayToProcess(idx, INPUTS_PORT, connection,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
  vtkInformation* array = this->GetInputArrayInformation(idx);
  if(!array)
    {
    vtkErrorMacro("No vtkInformation for array" << idx << ".");
    return;
    }
  array->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(),component);
  this->Modified();
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::SetArrayByPointCoord(ArrayIndex idx, 
                                                int component, int connection)
{
  vtkInformation *info = this->GetInputArrayInformation(idx);

  info->Set(INPUT_PORT(), INPUTS_PORT);
  info->Set(INPUT_CONNECTION(), connection);
  info->Set(vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT(),component);

  info->Remove(vtkDataObject::FIELD_ASSOCIATION());
  info->Remove(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
  info->Remove(vtkDataObject::FIELD_NAME());

  this->Modified();
}

void vtkScatterPlotMapper::SetArrayByName(ArrayIndex idx, const char* arrayName)
{
  vtkstd::string arrayString(arrayName);
  vtkstd::string array = "Points";
  if(arrayString.find(array) == 0)
    {
    int component = 0;
    if(arrayString.length() > array.length() && 
       arrayString[array.length()] == '(')
      {
      vtkstd::size_t endParenthesis = arrayString.find(')',array.length());
      if(endParenthesis != vtkstd::string::npos)
        {
        vtksys_ios::stringstream componentString;
        componentString << 
          arrayString.substr(array.length(),endParenthesis-array.length());
        char parenthesis;
        componentString >> parenthesis >> component >> parenthesis;
        }
      }
    this->SetArrayByPointCoord(idx, component);
    }
  else
    {
    int component = 0;
    vtkstd::size_t startParenthesis = arrayString.find('(');
    array = arrayString.substr(0, startParenthesis);
    if(startParenthesis != vtkstd::string::npos)
      {
      vtkstd::size_t endParenthesis = arrayString.find(')',array.length());
      if( endParenthesis != vtkstd::string::npos)
        {
        vtksys_ios::stringstream componentString;
        componentString << 
          arrayString.substr(array.length(),endParenthesis-array.length());
        char parenthesis;
        componentString >> parenthesis >> component >> parenthesis;
        }
      }
    this->SetArrayByFieldName(idx, array.c_str(), component);
    }
}


// ---------------------------------------------------------------------------
vtkDataArray* vtkScatterPlotMapper::GetArray(ArrayIndex idx)
{
  vtkInformation* array = this->GetInputArrayInformation(idx);
  vtkDataObject* object = this->GetInputDataObject(INPUTS_PORT, array->Get(INPUT_CONNECTION()));
  if(vtkCompositeDataSet::SafeDownCast(object))
    {
    vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(object);
    vtkSmartPointer<vtkCompositeDataIterator> it;
    it.TakeReference(composite->NewIterator());
    return this->GetArray(idx, vtkDataSet::SafeDownCast(
                            composite->GetDataSet(it)));
    }
  return this->GetArray(idx, vtkDataSet::SafeDownCast(object));
}

// ---------------------------------------------------------------------------
vtkDataArray* vtkScatterPlotMapper::GetArray(
  vtkScatterPlotMapper::ArrayIndex idx, vtkDataSet* input)
{
  //cout << "GetArray:" << idx << " " << input << endl;
  vtkDataArray* array = NULL;
  switch(idx)
    {
    case Z_COORDS:
      if(!this->ThreeDMode)
        {
        return array;
        }
      break;
    case COLOR:
      if(!this->Colorize)
        {
        return array;
        }
      break;
    case GLYPH_X_SCALE:
    case GLYPH_Y_SCALE:
    case GLYPH_Z_SCALE:
      if(!(this->GlyphMode & vtkScatterPlotMapper::ScaledGlyph))
        {
        return array;
        }
      break;    
    case GLYPH_SOURCE:
      if(!(this->GlyphMode & vtkScatterPlotMapper::UseMultiGlyph))
        {
        return array;
        }
      break;
    case GLYPH_X_ORIENTATION:
    case GLYPH_Y_ORIENTATION:
    case GLYPH_Z_ORIENTATION:
      if(!(this->GlyphMode & vtkScatterPlotMapper::OrientedGlyph))
        {
        return array;
        }
      break;
    default:
      break;
    }
  vtkInformation* info = this->GetInputArrayInformation(idx);
  if(info->Has(vtkDataObject::FIELD_NAME()) || 
     info->Has(vtkDataObject::FIELD_ATTRIBUTE_TYPE()))
    {
    array = this->GetInputArrayToProcess(idx, input);
    //cout<< "GetArray: " << array << endl;
    }
  else
    {
    if(vtkPointSet::SafeDownCast(input))
      {
      array = vtkPointSet::SafeDownCast(input)->GetPoints()->GetData();
      }
    }
  return array;
}
// ---------------------------------------------------------------------------
int vtkScatterPlotMapper::GetArrayComponent(
  vtkScatterPlotMapper::ArrayIndex idx)
{
  vtkInformation* array = this->GetInputArrayInformation(idx);
  return array->Get(FIELD_ACTIVE_COMPONENT());
}

// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkScatterPlotMapper::SetGlyphSourceConnection(int id,
  vtkAlgorithmOutput *algOutput)
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
      "source id, using " << numConnections << " instead.");
    this->AddInputConnection(GLYPHS_PORT, algOutput);
    }
}
// ---------------------------------------------------------------------------
// Specify a source object at a specified table location.
void vtkScatterPlotMapper::AddGlyphSourceConnection(
  vtkAlgorithmOutput *algOutput)
{
  this->AddInputConnection(GLYPHS_PORT, algOutput);
}

// ---------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
vtkPolyData *vtkScatterPlotMapper::GetGlyphSource(int id)
{
  if ( id < 0 || id >= this->GetNumberOfInputConnections(GLYPHS_PORT) )
    {
    return NULL;
    }

  return vtkPolyData::SafeDownCast(
    this->GetInputDataObject(GLYPHS_PORT, id));
}

// ---------------------------------------------------------------------------
vtkUnsignedCharArray* vtkScatterPlotMapper::GetColors()
{
  return vtkUnsignedCharArray::SafeDownCast(
    vtkDataSet::SafeDownCast(this->ScalarsToColorsPainter->GetOutput())
    ->GetPointData()->GetScalars());
}

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::UpdatePainterInformation()
{
  if (this->GetMTime() < this->PainterUpdateTime || 
      !this->ScalarsToColorsPainter)
    {
    return;
    }

  if(this->Colorize)
    {
    vtkInformation* info = this->ScalarsToColorsPainter->GetInformation();
//    vtkInformation* info = this->PainterInformation;
    vtkInformation* colorArrayInformation = 
      this->GetInputArrayInformation(vtkScatterPlotMapper::COLOR);
    vtkDataArray* array = this->GetArray(vtkScatterPlotMapper::COLOR);
    if(!array)
      {
      return;
      }
    info->Set(vtkPainter::STATIC_DATA(), this->Static);
    info->Set(vtkScalarsToColorsPainter::USE_LOOKUP_TABLE_SCALAR_RANGE(),
              //this->GetUseLookupTableScalarRange());
              false);
    info->Set(vtkScalarsToColorsPainter::SCALAR_RANGE(), 
              //          this->GetScalarRange(), 2);
              array->GetRange(),2);
    if(colorArrayInformation->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
       (colorArrayInformation->Get(vtkDataObject::FIELD_ASSOCIATION())
        == vtkDataObject::FIELD_ASSOCIATION_POINTS || 
        colorArrayInformation->Get(vtkDataObject::FIELD_ASSOCIATION())
        == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS))
      {
      info->Set(vtkScalarsToColorsPainter::SCALAR_MODE(), 
                VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      }
    else
      {
      info->Set(vtkScalarsToColorsPainter::SCALAR_MODE(), this->GetScalarMode());
      }
    info->Set(vtkScalarsToColorsPainter::COLOR_MODE(), this->GetColorMode());
    //if INTERPOLATE_SCALARS_BEFORE_MAPPING is set to 1, then
    // we would have to use texture coordinates: (vtkScalarsToColorsPainter
    // ->GetOutputData()->GetPointData()->GetScalars() will be NULL). 
    info->Set(vtkScalarsToColorsPainter::INTERPOLATE_SCALARS_BEFORE_MAPPING(),
//              this->GetInterpolateScalarsBeforeMapping());
              false);
    info->Set(vtkScalarsToColorsPainter::LOOKUP_TABLE(), this->LookupTable);
    info->Set(vtkScalarsToColorsPainter::SCALAR_VISIBILITY(), 
              this->GetScalarVisibility());
    if(colorArrayInformation->Has(vtkDataObject::FIELD_ATTRIBUTE_TYPE()))
      {
      info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(), 
                VTK_GET_ARRAY_BY_ID);
      info->Set(vtkScalarsToColorsPainter::ARRAY_ID(), 
                colorArrayInformation->Get(
                  vtkDataObject::FIELD_ATTRIBUTE_TYPE()));
      info->Remove(vtkScalarsToColorsPainter::ARRAY_NAME());
      }
    else if(colorArrayInformation->Has(vtkDataObject::FIELD_NAME()))
      {
      info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(), 
                VTK_GET_ARRAY_BY_NAME);
      info->Set(vtkScalarsToColorsPainter::ARRAY_NAME(), 
                colorArrayInformation->Get(vtkDataObject::FIELD_NAME()));
      info->Remove(vtkScalarsToColorsPainter::ARRAY_ID());
      }
    else
      {
      info->Remove(vtkScalarsToColorsPainter::ARRAY_ID());
      info->Remove(vtkScalarsToColorsPainter::ARRAY_NAME());
      info->Set(vtkScalarsToColorsPainter::ARRAY_ACCESS_MODE(), 
                this->ArrayAccessMode);
      }
    info->Set(vtkScalarsToColorsPainter::ARRAY_COMPONENT(), 
              colorArrayInformation->Get(
                vtkScatterPlotMapper::FIELD_ACTIVE_COMPONENT()));
    //info->Set(vtkScalarsToColorsPainter::ARRAY_ID(), this->ArrayId);
    //info->Set(vtkScalarsToColorsPainter::ARRAY_NAME(), this->ArrayName);
    //info->Set(vtkScalarsToColorsPainter::ARRAY_COMPONENT(), this->ArrayComponent);
    info->Set(vtkScalarsToColorsPainter::SCALAR_MATERIAL_MODE(), 
              this->GetScalarMaterialMode());
    }
  this->PainterUpdateTime.Modified();
}


// ---------------------------------------------------------------------------
// vtkPolyData *vtkScatterPlotMapper::GetGlyphSource(int idx,
//   vtkInformationVector *sourceInfo)
// {
//   vtkInformation *info = sourceInfo->GetInformationObject(idx);
//   if (!info)
//     {
//     return NULL;
//     }
//   return vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
// }

// ---------------------------------------------------------------------------
// const char* vtkScatterPlotMapper::GetOrientationModeAsString()
// {
//   switch (this->OrientationMode)
//     {
//   case vtkScatterPlotMapper::DIRECTION:
//     return "Direction";
//   case vtkScatterPlotMapper::ORIENTATION:
//     return "Orientation";
//     }
//   return "Invalid";
// }

// ---------------------------------------------------------------------------
void vtkScatterPlotMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->GetNumberOfInputConnections(1) < 2 )
    {
    if ( this->GetGlyphSource(0) != NULL )
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
  int vtkScatterPlotMapper::RequestUpdateExtent(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (sourceInfo)
    {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      1);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      0);
    }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

// ---------------------------------------------------------------------------
int vtkScatterPlotMapper::FillInputPortInformation(int port,
  vtkInformation *info)
{
  if (port == INPUTS_PORT)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
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

// ---------------------------------------------------------------------------
// Description:
// Return the method of scaling as a descriptive character string.
// const char *vtkScatterPlotMapper::GetScaleModeAsString(void)
// {
//   if ( this->ScaleMode == SCALE_BY_MAGNITUDE)
//     {
//     return "ScaleByMagnitude";
//     }
//   else if ( this->ScaleMode == SCALE_BY_COMPONENTS)
//     {
//     return "ScaleByVectorComponents";
//     }

//   return "NoDataScaling";
// }

// ---------------------------------------------------------------------------
// Description:
// Send mapper ivars to sub-mapper.
// \pre mapper_exists: mapper!=0
void vtkScatterPlotMapper::CopyInformationToSubMapper(
  vtkPainterPolyDataMapper *mapper)
{
  assert("pre: mapper_exists" && mapper!=0);
  if(!mapper)
    {
    vtkErrorMacro("Mapper can't be NULL. ");
    return;
    }

  // see void vtkPainterPolyDataMapper::UpdatePainterInformation()

  mapper->SetStatic(this->Static);
  mapper->ScalarVisibilityOff();
  // not used
  //mapper->SetClippingPlanes(this->ClippingPlanes);

  mapper->SetResolveCoincidentTopology(this->GetResolveCoincidentTopology());
  mapper->SetResolveCoincidentTopologyZShift(
    this->GetResolveCoincidentTopologyZShift());

  // ResolveCoincidentTopologyPolygonOffsetParameters is static
  mapper->SetResolveCoincidentTopologyPolygonOffsetFaces(
    this->GetResolveCoincidentTopologyPolygonOffsetFaces());
  mapper->SetImmediateModeRendering(this->ImmediateModeRendering ||
                                    !this->NestedDisplayLists);
}

// ---------------------------------------------------------------------------
// Description:
// Method initiates the mapping process. Generally sent by the actor 
// as each frame is rendered.
void vtkScatterPlotMapper::Render(vtkRenderer *ren, vtkActor *actor)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() == 
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  if (selector)
    {
    selector->BeginRenderProp();
    }

  if (selector && !selecting_points)
    {
    // Selecting some other attribute. Not supported.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }
  bool immediateMode = this->ImmediateModeRendering ||
    vtkMapper::GetGlobalImmediateModeRendering() ||
    (selecting_points);

  bool createDisplayList = false;
  if (immediateMode)
    {
    this->ReleaseDisplayList();
    }
  else
    {
    // if something has changed, regenerate display lists.
    createDisplayList = this->DisplayListId == 0 || 
      this->GetMTime() > this->BuildTime ||
      ren->GetRenderWindow() != this->LastWindow.GetPointer();
    }
  //cout << " ImmediateMode: " << immediateMode << endl;
  //cout << " this->ImmediateMode: " << this->ImmediateModeRendering << endl;
  //cout << " Create Display List: " << createDisplayList << endl;
  if(immediateMode || createDisplayList)
    {
    //cout << __FUNCTION__ << endl;
    this->PrepareForRendering(ren,actor);
    
    if(createDisplayList)
      {
      this->ReleaseDisplayList();
      this->DisplayListId = glGenLists(1);
      glNewList(this->DisplayListId,GL_COMPILE);
      }

    bool multiplyWithAlpha = 
      this->ScalarsToColorsPainter->GetPremultiplyColorsWithAlpha(actor)==1;
    if (multiplyWithAlpha)
      {
      // We colors were premultiplied by alpha then we change the blending
      // function to one that will compute correct blended destination alpha
      // value, otherwise we stick with the default.
      // save the blend function.
      glPushAttrib(GL_COLOR_BUFFER_BIT);
      // the following function is not correct with textures because there
      // are not premultiplied by alpha.
      glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
      }

    if(this->GlyphMode & UseGlyph)
      {
      this->RenderGlyphs(ren, actor);
      }
    else
      {
      this->RenderPoints(ren, actor);
      }

    // from vtkOpenGLScalarsToColorsPainter::RenderInternal
    if(multiplyWithAlpha)
      {
      // restore the blend function
      glPopAttrib();
      }

    if(createDisplayList)
      {
      glEndList();
      this->BuildTime.Modified();
      this->LastWindow = ren->GetRenderWindow();
      }

    int error = glGetError();
    if(error!=GL_NO_ERROR)
      {
      cout<< " ERRRRRROR3: "<< error << endl;
      }
    } // if(immediateMode||createDisplayList)
    

  if(!immediateMode)
    {
    this->TimeToDraw=0.0;
    this->Timer->StartTimer();
    glCallList(this->DisplayListId);
    this->Timer->StopTimer();
    this->TimeToDraw += this->Timer->GetElapsedTime();
    }

  if (selector && !selecting_points)
    {
    // Selecting some other attribute. Not supported.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
  if (selector)
    {
    selector->EndRenderProp();
    }
}

unsigned long vtkScatterPlotMapper::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();

  vtkDataArray* xCoordsArray = this->GetArray(vtkScatterPlotMapper::X_COORDS);
  vtkDataArray* yCoordsArray = this->GetArray(vtkScatterPlotMapper::Y_COORDS);
  vtkDataArray* zCoordsArray = this->GetArray(vtkScatterPlotMapper::Z_COORDS);
  vtkDataArray* colorArray   = this->GetArray(vtkScatterPlotMapper::COLOR);
  if(xCoordsArray)
    {
    mTime = max(mTime,xCoordsArray->GetMTime());
    }
  if(yCoordsArray)
    {
    mTime = max(mTime,yCoordsArray->GetMTime());
    }
  if(this->ThreeDMode && zCoordsArray)
    {
    mTime = max(mTime,zCoordsArray->GetMTime());
    }
  if(this->Colorize && colorArray)
    {
    mTime = max(mTime,colorArray->GetMTime());
    }
  if(this->GlyphMode & vtkScatterPlotMapper::UseGlyph)
    {
    vtkDataArray* glyphXScaleArray = 
      this->GetArray(vtkScatterPlotMapper::GLYPH_X_SCALE);
    vtkDataArray* glyphYScaleArray = 
      this->GetArray(vtkScatterPlotMapper::GLYPH_Y_SCALE);
    vtkDataArray* glyphZScaleArray = 
      this->GetArray(vtkScatterPlotMapper::GLYPH_Z_SCALE);
    vtkDataArray* glyphSourceArray = 
      this->GetArray(vtkScatterPlotMapper::GLYPH_SOURCE);
    vtkDataArray* glyphXOrientationArray = 
      this->GetArray(vtkScatterPlotMapper::GLYPH_X_ORIENTATION);
    vtkDataArray* glyphYOrientationArray = 
      this->GetArray(vtkScatterPlotMapper::GLYPH_Y_ORIENTATION);
    vtkDataArray* glyphZOrientationArray = 
      this->GetArray(vtkScatterPlotMapper::GLYPH_Z_ORIENTATION);
    
    if(this->GlyphMode & ScaledGlyph && glyphXScaleArray)
      {
      mTime = max(mTime,glyphXScaleArray->GetMTime());
      }
    if(this->GlyphMode & ScaledGlyph && this->ScalingArrayMode == Xc_Yc_Zc &&
       glyphYScaleArray)
      {
      mTime = max(mTime,glyphYScaleArray->GetMTime());
      }
    if(this->GlyphMode & ScaledGlyph && this->ScalingArrayMode == Xc_Yc_Zc &&
       glyphZScaleArray)
      {
      mTime = max(mTime,glyphZScaleArray->GetMTime());
      }
    if(this->GlyphMode & UseMultiGlyph && glyphSourceArray)
      {
      mTime = max(mTime,glyphSourceArray->GetMTime());
      }
    if(this->GlyphMode & OrientedGlyph && glyphXOrientationArray)
      {
      mTime = max(mTime,glyphXOrientationArray->GetMTime());
      }
    if(this->GlyphMode & OrientedGlyph && glyphYOrientationArray)
      {
      mTime = max(mTime,glyphYOrientationArray->GetMTime());
      }
    if(this->GlyphMode & OrientedGlyph && glyphZOrientationArray)
      {
      mTime = max(mTime,glyphZOrientationArray->GetMTime());
      }
    }
  return mTime;
}
//-------------------------------------------------------------------------
double *vtkScatterPlotMapper::GetBounds()
{
  //  static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};
  vtkMath::UninitializeBounds(this->Bounds);
  // do we have an input
  if(!this->GetNumberOfInputConnections(0))
    {
    return this->Bounds;
    }
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

  //vtkDataSet *input = this->GetInput();
  //input->GetBounds(this->Bounds);
  vtkDataArray *xArray = this->GetArray(vtkScatterPlotMapper::X_COORDS);
  vtkDataArray *yArray = this->GetArray(vtkScatterPlotMapper::Y_COORDS);
  vtkDataArray *zArray = this->GetArray(vtkScatterPlotMapper::Z_COORDS);
  
  if(xArray)
    {
    xArray->GetRange(this->Bounds,this->GetArrayComponent(X_COORDS));
    }
  if(yArray)
    {
    yArray->GetRange(this->Bounds+2,this->GetArrayComponent(Y_COORDS));
    }
  if(zArray && this->ThreeDMode)
    {
    zArray->GetRange(this->Bounds+4,this->GetArrayComponent(Z_COORDS));
    }
  else
    {
    this->Bounds[4] = -1.;
    this->Bounds[5] = 1.;
    }
  
  if(!this->GlyphMode)
    {
    return this->Bounds;
    }
//   cout << this->Bounds[0] << " " << this->Bounds[1] << " "
//        << this->Bounds[2] << " " << this->Bounds[3] << " "
//        << this->Bounds[4] << " " << this->Bounds[5] << endl;
  
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

  double maxScale[2] = {1.0, 1.0};

  vtkDataArray* glyphXScaleArray = this->GetArray(GLYPH_X_SCALE);
  vtkDataArray* glyphYScaleArray = this->GetArray(GLYPH_Y_SCALE);
  vtkDataArray* glyphZScaleArray = this->GetArray(GLYPH_Z_SCALE);

  if (this->GlyphMode && this->GlyphMode & ScaledGlyph &&
      glyphXScaleArray)
    {
    double xScaleRange[2] = {1.0, 1.0};
    double yScaleRange[2] = {1.0, 1.0};
    double zScaleRange[2] = {1.0, 1.0};
    double x0ScaleRange[2] = {1.0, 1.0};
    double x1ScaleRange[2] = {1.0, 1.0};
    double x2ScaleRange[2] = {1.0, 1.0};
    double range[3];
    switch(this->ScaleMode)
      {
      case SCALE_BY_MAGNITUDE:
        switch(this->ScalingArrayMode)
          {
          case Xc_Yc_Zc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(GLYPH_X_SCALE));
            glyphYScaleArray->GetRange(
              yScaleRange, this->GetArrayComponent(GLYPH_Y_SCALE));
            glyphZScaleArray->GetRange(
              zScaleRange, this->GetArrayComponent(GLYPH_Z_SCALE));
            range[0] = xScaleRange[1];
            range[1] = yScaleRange[1];
            range[2] = zScaleRange[1];
            maxScale[1] = vtkMath::Norm(range);
            range[0] = xScaleRange[0];
            range[1] = yScaleRange[0];
            range[2] = zScaleRange[0];
            maxScale[0] = vtkMath::Norm(range);
            break;
          case Xc0_Xc1_Xc2:
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
          case Xc_Xc_Xc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(GLYPH_X_SCALE));
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
      case SCALE_BY_COMPONENTS:
        switch(this->ScalingArrayMode)
          {
          case Xc_Yc_Zc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(GLYPH_X_SCALE));
            glyphYScaleArray->GetRange(
              yScaleRange, this->GetArrayComponent(GLYPH_Y_SCALE));
            glyphZScaleArray->GetRange(
              zScaleRange, this->GetArrayComponent(GLYPH_Z_SCALE));
            maxScale[0] = min(xScaleRange[0],yScaleRange[0]);
            maxScale[0] = min(maxScale[0],zScaleRange[0]);
            maxScale[1] = max(xScaleRange[1],yScaleRange[1]);
            maxScale[1] = max(maxScale[1],zScaleRange[1]);
            break;
          case Xc0_Xc1_Xc2:
            glyphXScaleArray->GetRange(x0ScaleRange, 0);
            glyphXScaleArray->GetRange(x1ScaleRange, 1);
            glyphXScaleArray->GetRange(x2ScaleRange, 2);
            maxScale[0] = min(x0ScaleRange[0], x1ScaleRange[0]);
            maxScale[0] = min(maxScale[0], x2ScaleRange[0]);
            maxScale[1] = max(x0ScaleRange[1], x1ScaleRange[1]);
            maxScale[1] = max(maxScale[1], x2ScaleRange[1]);
            break;
          case Xc_Xc_Xc:
            glyphXScaleArray->GetRange(
              xScaleRange, this->GetArrayComponent(GLYPH_X_SCALE));
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
   if(this->GetGlyphSource(0) == 0)
     {
     this->GenerateDefaultGlyphs();
     }
  int indexRange[2] = {0, 0};
  int numberOfGlyphSources = this->GetNumberOfInputConnections(GLYPHS_PORT);

  vtkDataArray* glyphSourceArray = this->GetArray(GLYPH_SOURCE);  
  // Compute indexRange.  
  double sourceRange[2] = {0.,1.};
  double sourceRangeDiff = 1.;
  if(glyphSourceArray)
    {
    glyphSourceArray->GetRange(sourceRange, this->GetArrayComponent(GLYPH_SOURCE));
    sourceRangeDiff = sourceRange[1] - sourceRange[0];
    if(sourceRangeDiff == 0.0)
      {
      sourceRangeDiff = 1.0;
      }
    double range[2];
    glyphSourceArray->GetRange(range, -1);
    for (int i=0; i < 2; ++i)
      {
      indexRange[i] = static_cast<int>(((range[i]-sourceRange[0])/sourceRangeDiff)*numberOfGlyphSources);
      indexRange[i] = ::vtkClamp(indexRange[i], 0, numberOfGlyphSources-1);
      }
    }

  vtkBoundingBox bbox; // empty

  for(int index = indexRange[0]; index <= indexRange[1]; ++index )
    {
    vtkPolyData *source = this->GetGlyphSource(index);
    // Make sure we're not indexing into empty glyph
    if(source!=0)
      {
      double bounds[6];
      source->GetBounds(bounds);// can be invalid/uninitialized
      if(vtkMath::AreBoundsInitialized(bounds))
        {
        bbox.AddBounds(bounds);
        }
      }
    }
   
  if (this->GlyphMode && this->GlyphMode & ScaledGlyph)
    {
    vtkBoundingBox bbox2(bbox);
    bbox.Scale(maxScale[0], maxScale[0], maxScale[0]);
    bbox2.Scale(maxScale[1], maxScale[1], maxScale[1]);
    bbox.AddBox(bbox2);
    bbox.Scale(this->ScaleFactor, this->ScaleFactor, this->ScaleFactor);
    }

  if(bbox.IsValid())
    {
    double bounds[6];
    if(this->GlyphMode &&
       this->GlyphMode & OrientedGlyph && 
       this->GetArray(GLYPH_X_ORIENTATION))
      {
      // bounding sphere.
      double c[3];
      bbox.GetCenter(c);
      double l = bbox.GetDiagonalLength()/2.0;
      bounds[0] = c[0] - l;
      bounds[1] = c[0] + l;
      bounds[2] = c[1] - l;
      bounds[3] = c[1] + l;
      bounds[4] = c[2] - l;
      bounds[5] = c[2] + l;
      }
    else
      {
      bbox.GetBounds(bounds);
      }

    for(int j = 0; j < 6; ++j)
      {
      this->Bounds[j] += bounds[j];
      }
    }
  else
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
    }
  return this->Bounds;
}

//-------------------------------------------------------------------------
void vtkScatterPlotMapper::GetBounds(double bounds[6])
{
  this->Superclass::GetBounds(bounds);
}

// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkScatterPlotMapper::ReleaseGraphicsResources(vtkWindow *window)
{
  if(this->SourceMappers!=0)
    {
    size_t c=this->SourceMappers->Mappers.size();
    for(size_t i=0; i<c; ++i)
      {
      this->SourceMappers->Mappers[i]->ReleaseGraphicsResources(window);
      }
    }
  this->ReleaseDisplayList();
}

// ---------------------------------------------------------------------------
// Description:
// Release display list used for matrices and color.
void vtkScatterPlotMapper::ReleaseDisplayList()
{
  if(this->DisplayListId>0)
    {
    glDeleteLists(this->DisplayListId,1);
    this->DisplayListId = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkScatterPlotMapper::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ScalarsToColorsPainter, 
                            "ScalarsToColorsPainter");
}


void vtkScatterPlotMapper::PrepareForRendering(vtkRenderer *ren, 
                                               vtkActor *actor)
{
  // Get the color ready
  this->UpdatePainterInformation();
  vtkInformation* array = this->GetInputArrayInformation(COLOR);
  vtkDataObject* object = this->GetInputDataObject(
    INPUTS_PORT,array->Get(INPUT_CONNECTION()));

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
    this->ScalarsToColorsPainter->SetInput(object);
    }
  this->ScalarsToColorsPainter->Render(ren, actor, 0xff, false);

  if(this->GlyphMode & vtkScatterPlotMapper::UseGlyph)
    {
    //Get the Glyphs ready
    this->InitGlyphMappers(ren, actor);
    }
}

void vtkScatterPlotMapper::RenderPoints(vtkRenderer *ren, vtkActor *actor)
{
  //cout << "render points" << endl;
  vtkDataArray* xCoordsArray = this->GetArray(X_COORDS);
  vtkDataArray* yCoordsArray = this->GetArray(Y_COORDS);
  vtkDataArray* zCoordsArray = this->GetArray(Z_COORDS);
  vtkDataArray* colorArray   = this->GetArray(COLOR);

  if(!xCoordsArray || !yCoordsArray || 
     (!zCoordsArray && this->ThreeDMode) ||
     (!colorArray && this->Colorize))
    {
    //vtkErrorMacro("One array is not set.");
    vtkWarningMacro("One array is not set.");
    //return;
    }
  
  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() == 
                                       vtkDataObject::FIELD_ASSOCIATION_POINTS);
  
  // COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR 
  //   COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR 
  //      COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR 
  vtkUnsignedCharArray* colors = this->Colorize? this->GetColors() : NULL;

  bool multiplyWithAlpha = 
    this->ScalarsToColorsPainter->GetPremultiplyColorsWithAlpha(actor)==1;
  if (multiplyWithAlpha)
    {
    // We colors were premultiplied by alpha then we change the blending
    // function to one that will compute correct blended destination alpha
    // value, otherwise we stick with the default.
    // save the blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    // the following function is not correct with textures because there
    // are not premultiplied by alpha.
    glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    }
  
  vtkIdType numPts = xCoordsArray->GetNumberOfTuples();
  if (numPts < 1)
    {
    vtkDebugMacro(<<"No points to glyph!");
    return;
    }  
  int Xc = this->GetArrayComponent(X_COORDS);
  int Yc = this->GetArrayComponent(Y_COORDS);
  int Zc = this->GetArrayComponent(Z_COORDS);
  glDisable(GL_LIGHTING);
  glBegin(GL_POINTS);
  // Traverse all Input points, transforming Source points
  for(vtkIdType inPtId=0; inPtId < numPts; inPtId++)
    {
    if(!(inPtId % 10000))
      {
      this->UpdateProgress (static_cast<double>(inPtId)/
                            static_cast<double>(numPts));
      if (this->GetAbortExecute())
        {
        cout << "abort" << endl;
        break;
        }
      }
     
    //COLORING
    // Copy scalar value
    if(selecting_points)
      {
      selector->RenderAttributeId(inPtId);
      }
    else if(colors)
      {
      unsigned char rgba[4];
      colors->GetTupleValue(inPtId, rgba);
      glColor4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
      }

    // translate Source to Input point
    double point[3] = {0.,0.,0.};
    //input->GetPoint(inPtId, point);
    point[0] = xCoordsArray->GetTuple(inPtId)[Xc];
    point[1] = yCoordsArray->GetTuple(inPtId)[Yc];
    if(this->ThreeDMode)
      {
      point[2] = zCoordsArray->GetTuple(inPtId)[Zc];
      }
    glVertex3f(point[0],point[1],point[2]);
    }
  glEnd();
 }


void vtkScatterPlotMapper::RenderGlyphs(vtkRenderer *ren, vtkActor *actor)
{
  vtkDataArray* xCoordsArray = this->GetArray(X_COORDS);
  vtkDataArray* yCoordsArray = this->GetArray(Y_COORDS);
  vtkDataArray* zCoordsArray = this->GetArray(Z_COORDS);
  vtkDataArray* colorArray = this->GetArray(COLOR);
  vtkDataArray* glyphXScaleArray = this->GetArray(GLYPH_X_SCALE);
  vtkDataArray* glyphYScaleArray = this->GetArray(GLYPH_Y_SCALE);
  vtkDataArray* glyphZScaleArray = this->GetArray(GLYPH_Z_SCALE);
  vtkDataArray* glyphSourceArray = this->GetArray(GLYPH_SOURCE);
  vtkDataArray* glyphXOrientationArray = this->GetArray(GLYPH_X_ORIENTATION);
  vtkDataArray* glyphYOrientationArray = this->GetArray(GLYPH_Y_ORIENTATION);
  vtkDataArray* glyphZOrientationArray = this->GetArray(GLYPH_Z_ORIENTATION);

  if(!xCoordsArray || !yCoordsArray || 
     (!zCoordsArray && this->ThreeDMode) ||
     (!colorArray && this->Colorize) || 
     (!glyphXScaleArray && this->GlyphMode & ScaledGlyph) ||
     (!glyphYScaleArray && this->GlyphMode & ScaledGlyph &&
      this->GetArrayComponent(GLYPH_X_SCALE)!=-1) ||
     (!glyphZScaleArray && this->GlyphMode & ScaledGlyph && 
      this->GetArrayComponent(GLYPH_X_SCALE)!=-1) ||
     (!glyphSourceArray && this->GlyphMode & UseMultiGlyph) ||
     (!glyphXOrientationArray && this->GlyphMode & OrientedGlyph) ||
     (!glyphYOrientationArray && this->GlyphMode & OrientedGlyph) ||
     (!glyphZOrientationArray && this->GlyphMode & OrientedGlyph))
    {
    //vtkErrorMacro("One array is not set.");
    vtkWarningMacro("One array is not set.");
    //return;
    }
  
 
  // vtkBitArray *maskArray = 0;
//     if (this->Masking)
//       {
//       maskArray = vtkBitArray::SafeDownCast(this->GetMaskArray(input));
//       if (maskArray==0)
//         {
//         vtkDebugMacro(<<"masking is enabled but there is no mask array. Ignore masking.");
//         }
//       else
//         {
//         if (maskArray->GetNumberOfComponents()!=1)
//           {
//           vtkErrorMacro(" expecting a mask array with one component, getting "
//             << maskArray->GetNumberOfComponents() << " components.");
//           return;
//           }
//         }
//       }
    
  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() == 
                                       vtkDataObject::FIELD_ASSOCIATION_POINTS);
  
  int Xc = this->GetArrayComponent(X_COORDS);
  int Yc = this->GetArrayComponent(Y_COORDS);
  int Zc = this->GetArrayComponent(Z_COORDS);
  int SXc = this->GetArrayComponent(GLYPH_X_SCALE);
  int SYc = this->ScalingArrayMode == Xc_Yc_Zc ?
    this->GetArrayComponent(GLYPH_Y_SCALE) : 0;
  int SZc = this->ScalingArrayMode == Xc_Yc_Zc ?
    this->GetArrayComponent(GLYPH_Z_SCALE) : 0;
  int SOc = this->GetArrayComponent(GLYPH_SOURCE);
  int OXc = this->GetArrayComponent(GLYPH_X_ORIENTATION);
  int OYc = this->GetArrayComponent(GLYPH_Y_ORIENTATION);
  int OZc = this->GetArrayComponent(GLYPH_Z_ORIENTATION);

  // Check input for consistency
  //
  double sourceRange[2] = {0.,1.};
  double sourceRangeDiff = 1.;
  if(glyphSourceArray)
    {
    glyphSourceArray->GetRange(sourceRange,SOc);
    sourceRangeDiff = sourceRange[1] - sourceRange[0];
    if(sourceRangeDiff==0.0)
      {
      sourceRangeDiff=1.0;
      }
    }

  int numberOfGlyphSources = this->GetNumberOfInputConnections(GLYPHS_PORT);
  
  // COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR 
  //   COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR 
  //      COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR COLOR 
  vtkUnsignedCharArray* colors = this->Colorize? this->GetColors() : NULL;

  vtkTransform *trans = vtkTransform::New();

  //vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numPts = xCoordsArray->GetNumberOfTuples();
  if (numPts < 1)
    {
    vtkDebugMacro(<<"No points to glyph!");
    return;
    }
  glMatrixMode(GL_MODELVIEW);
  // Traverse all Input points, transforming Source points
  for(vtkIdType inPtId=0; inPtId < numPts; inPtId++)
    {
    if(!(inPtId % 10000))
      {
      this->UpdateProgress (static_cast<double>(inPtId)/
                            static_cast<double>(numPts));
      if (this->GetAbortExecute())
        {
        break;
        }
      }
     
//      if (maskArray && maskArray->GetValue(inPtId)==0)
//        {
//        continue;
//        }

    // TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE
    //   TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE
    //      TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE  TRANSLATE
    // translate Source to Input point
    double point[3] = {0.,0.,0.};
    //input->GetPoint(inPtId, point);
    point[0] = xCoordsArray->GetTuple(inPtId)[Xc];
    point[1] = yCoordsArray->GetTuple(inPtId)[Yc];
    if(this->ThreeDMode)
      {
      point[2] = zCoordsArray->GetTuple(inPtId)[Zc];
      }

    // SCALING  SCALING  SCALING  SCALING  SCALING  SCALING  SCALING
    //   SCALING  SCALING  SCALING  SCALING  SCALING  SCALING  SCALING 
    //      SCALING  SCALING  SCALING  SCALING  SCALING  SCALING  SCALING 
    double scale[3] = {1.,1.,1.};
    if(this->GlyphMode & ScaledGlyph)
      {
      // Get the scalar and vector data
      double* xTuple = glyphXScaleArray->GetTuple(inPtId);
      double* yTuple = glyphYScaleArray?glyphYScaleArray->GetTuple(inPtId):NULL;
      double* zTuple = glyphZScaleArray?glyphZScaleArray->GetTuple(inPtId):NULL;
      switch (this->ScaleMode)
        {
        case SCALE_BY_MAGNITUDE:
          switch(this->ScalingArrayMode)
            {
            case Xc_Yc_Zc:
              scale[0] = xTuple[SXc];
              scale[1] = yTuple[SYc];
              scale[2] = zTuple[SZc];
              scale[0] = scale[1] = scale[2] =
                vtkMath::Norm(scale);
              break;
            case Xc0_Xc1_Xc2:
              scale[0] = scale[1] = scale[2] = 
                vtkMath::Norm( xTuple+SXc,
                               glyphXScaleArray->GetNumberOfComponents() );
              break;
            case Xc_Xc_Xc:
              scale[0] = scale[1] = scale[2] = xTuple[SXc];
              break;
            default:
              vtkErrorMacro("Wrong ScalingArray mode");
              break;
            }
          break;
        case SCALE_BY_COMPONENTS:
          switch(this->ScalingArrayMode)
            {
            case Xc_Yc_Zc:
              scale[0] = xTuple[SXc];
              scale[1] = yTuple[SYc];
              scale[2] = zTuple[SZc];
              break;
            case Xc0_Xc1_Xc2:
              if(glyphXScaleArray->GetNumberOfComponents() < 3)
                {
                vtkErrorMacro("Cannot scale by components since " << 
                              glyphXScaleArray->GetName() << 
                              " does not have at least 3 components.");
                }
              scale[0] = xTuple[SXc];
              scale[1] = xTuple[SXc+1];
              scale[2] = xTuple[SXc+2];
              break;
            case Xc_Xc_Xc:
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

    if(scale[0] == 0.0)
      {
      scale[0] = 1.0e-10;
      }
    if(scale[1] == 0.0)
      {
      scale[1] = 1.0e-10;
      }
    if(scale[2] == 0.0)
      {
      scale[2] = 1.0e-10;
      }

    // MULTI  MULTI  MULTI  MULTI  MULTI  MULTI  MULTI MULTI  MULTI
    //   MULTI  MULTI  MULTI  MULTI  MULTI  MULTI  MULTI MULTI  MULTI
    //      MULTI  MULTI  MULTI  MULTI  MULTI  MULTI  MULTI MULTI  MULTI
    int index = 0;
    // Compute index into table of glyphs
    if(this->GlyphMode & UseMultiGlyph && glyphSourceArray)
      {
      double value = 0.;
      if(SOc == -1)
        {
        value = vtkMath::Norm(glyphSourceArray->GetTuple(inPtId),
                              glyphSourceArray->GetNumberOfComponents());
        }
      else
        {
        value = glyphSourceArray->GetTuple(inPtId)[SOc];
        }
      index = static_cast<int>(((value - sourceRange[0])/sourceRangeDiff)*numberOfGlyphSources);
      index = ::vtkClamp(index, 0, numberOfGlyphSources-1);
      }

    // source can be null.
    vtkPolyData *source = this->GetGlyphSource(index);

    // Make sure we're not indexing into empty glyph
    if (!source)
      {
      vtkErrorMacro("Glyph " << index << " is not set");
      }

    // ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION
    //   ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION
    //      ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION  ORIENTATION
    double orientation[3] = {0.,0.,0.};
    if(this->GlyphMode & OrientedGlyph)
      {
      if(OXc == -1)
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

    // ORIENTATION
    if(this->GlyphMode & OrientedGlyph)
      {
      switch (this->OrientationMode)
        {
        case ROTATION:
          trans->RotateZ(orientation[2]);
          trans->RotateX(orientation[0]);
          trans->RotateY(orientation[1]);
          break;

        case DIRECTION:
          if (orientation[1] == 0.0 && orientation[2] == 0.0)
            {
            if (orientation[0] < 0) //just flip x if we need to
              {
              trans->RotateWXYZ(180.0,0,1,0);
              }
            }
          else
            {
            double vMag = vtkMath::Norm(orientation);
            double vNew[3];
            vNew[0] = (orientation[0]+vMag) / 2.0;
            vNew[1] = orientation[1] / 2.0;
            vNew[2] = orientation[2] / 2.0;
            trans->RotateWXYZ(180.0,vNew[0],vNew[1],vNew[2]);
            }
          break;
        }
      }
     
    //COLORING
    // Copy scalar value
    if(selecting_points)
      {
      selector->RenderAttributeId(inPtId);
      }
    else if(colors)
      {
      unsigned char rgba[4];
      colors->GetTupleValue(inPtId, rgba);
      glColor4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
      }

    //SCALING
    if(this->GlyphMode & ScaledGlyph)
      {
      trans->Scale(scale[0],scale[1],scale[2]);
      }
     
    // multiply points and normals by resulting matrix
    glPushMatrix();
    double *mat = trans->GetMatrix()->Element[0];
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
    this->SourceMappers->Mappers[static_cast<size_t>(
        index)]->Render(ren,actor);
    // Don't know why but can't assume glMatrix(GL_MODELVIEW);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }
  trans->Delete();
  
 }

void vtkScatterPlotMapper::InitGlyphMappers(vtkRenderer* ren, vtkActor* actor, 
                                            bool vtkNotUsed(createDisplayList))
{
  // Create a default source, if no source is specified.
  if (this->GetGlyphSource(0) == 0)
    {
    this->GenerateDefaultGlyphs();
    }
  
  int numberOfGlyphSources = this->GetNumberOfInputConnections(GLYPHS_PORT);
  if(this->SourceMappers==0)
    {
    this->SourceMappers = new vtkScatterPlotMapperArray;
    }
  vtkDataArray* glyphSourceArray = 
    this->GetArray(vtkScatterPlotMapper::GLYPH_SOURCE);
  if(glyphSourceArray)
    {
    this->SourceMappers->Mappers.resize(
      static_cast<size_t>(numberOfGlyphSources));
    }
  else
    {
    this->SourceMappers->Mappers.resize(1);
    }
  for (size_t cc=0; cc < this->SourceMappers->Mappers.size(); cc++)
    {
    vtkPolyData *s = this->GetGlyphSource(static_cast<int>(cc));
    // s can be null.
    if (this->SourceMappers->Mappers[cc]==0)
      {
      this->SourceMappers->Mappers[cc] = vtkPainterPolyDataMapper::New();
      this->SourceMappers->Mappers[cc]->Delete();
      vtkDefaultPainter *p=
        static_cast<vtkDefaultPainter *>(
          this->SourceMappers->Mappers[cc]->GetPainter());
      p->SetScalarsToColorsPainter(0); // bypass default mapping.
      p->SetClipPlanesPainter(0); // bypass default mapping.
      vtkHardwareSelectionPolyDataPainter::SafeDownCast(
        this->SourceMappers->Mappers[cc]->GetSelectionPainter())->EnableSelectionOff();
      // use the same painter for selection pass as well.
      }
    // Copy mapper ivar to sub-mapper
    this->CopyInformationToSubMapper(this->SourceMappers->Mappers[cc]);

    vtkPolyData *ss = this->SourceMappers->Mappers[cc]->GetInput();
    if (ss==0)
      {
      ss = vtkPolyData::New();
      this->SourceMappers->Mappers[cc]->SetInput(ss);
      ss->Delete();
      ss->ShallowCopy(s);
      }

    if (s->GetMTime()>ss->GetMTime())
      {
      ss->ShallowCopy(s);
      }

    if(!this->ImmediateModeRendering && 
       this->NestedDisplayLists)
      {
      this->SourceMappers->Mappers[cc]->SetForceCompileOnly(1);
      this->SourceMappers->Mappers[cc]->Render(ren,actor); // compile display list.
      this->SourceMappers->Mappers[cc]->SetForceCompileOnly(0);
      }
    }
}

void vtkScatterPlotMapper::GenerateDefaultGlyphs()
{
  // create a diamond shape
  vtkPolyData* defaultSource = vtkPolyData::New();
  vtkPoints* defaultPoints = vtkPoints::New();
  int points = 16;
  vtkIdType* defaultPointIds = new vtkIdType[points+1];
  for(int i = 0;i <= points; ++i)
    {
    defaultPointIds[i] = i;
    }
  // triangle
  defaultSource->Allocate();
  defaultPoints->Allocate(4);
  defaultPoints->InsertNextPoint(-0.2*tan(PI/6.), -0.1, 0);
  defaultPoints->InsertNextPoint(0, 0.1, 0);
  defaultPoints->InsertNextPoint(0.2*tan(PI/6.), -0.1, 0);
  defaultPoints->InsertNextPoint(-0.2*tan(PI/6.), -0.1, 0);
  defaultSource->SetPoints(defaultPoints);

  defaultSource->InsertNextCell(VTK_POLY_LINE, 4, defaultPointIds);
  defaultSource->SetUpdateExtent(0, 1, 0);
  this->AddGlyphSourceConnection(defaultSource->GetProducerPort());
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
  defaultSource->SetUpdateExtent(0, 1, 0);
  this->AddGlyphSourceConnection(defaultSource->GetProducerPort());
  defaultSource->Delete();
  defaultPoints->Delete();

  // pentagone
  defaultSource = vtkPolyData::New();
  defaultPoints = vtkPoints::New();
  defaultSource->Allocate();
  defaultPoints->Allocate(6);
  double angle5 = 2. * PI / 5.;
  defaultPoints->InsertNextPoint(0.0, 0.1, 0);
  defaultPoints->InsertNextPoint(0.1*cos(angle5 + PI/2.), 0.1*sin(angle5 + PI/2.), 0);
  defaultPoints->InsertNextPoint(0.1*cos(2.*angle5 + PI/2.), 0.1*sin(2.*angle5 + PI/2.), 0);
  defaultPoints->InsertNextPoint(0.1*cos(3.*angle5 + PI/2.), 0.1*sin(3.*angle5 + PI/2.), 0);
  defaultPoints->InsertNextPoint(0.1*cos(4.*angle5 + PI/2.), 0.1*sin(4.*angle5 + PI/2.), 0);
  defaultPoints->InsertNextPoint(0.0, 0.1, 0);
  defaultSource->SetPoints(defaultPoints);
  defaultSource->InsertNextCell(VTK_POLY_LINE, 6, defaultPointIds);
  defaultSource->SetUpdateExtent(0, 1, 0);
  this->AddGlyphSourceConnection(defaultSource->GetProducerPort());
  defaultSource->Delete();
  defaultPoints->Delete();

  // circle
  defaultSource= vtkPolyData::New();
  defaultPoints = vtkPoints::New();
  defaultSource->Allocate();
  defaultPoints->Allocate(points+1);
  double angle = 0.;
  double step = 2. * PI / points;
  for(int i = 0; i <= points; ++i)
    {
    angle = step*i;
    defaultPoints->InsertNextPoint(0.1*cos(angle), 0.1*sin(angle), 0);
    }
  defaultSource->SetPoints(defaultPoints);
  defaultSource->InsertNextCell(VTK_POLY_LINE, points + 1, defaultPointIds);
  defaultSource->SetUpdateExtent(0, 1, 0);
  this->AddGlyphSourceConnection(defaultSource->GetProducerPort());
  defaultSource->Delete();
  defaultPoints->Delete();

  delete [] defaultPointIds;
}
