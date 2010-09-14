/*=========================================================================

  Program:   ParaView
  Module:    vtkQuerySelectionSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuerySelectionSource.h"

#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

class vtkQuerySelectionSource::vtkInternals
{
  template <class T>
  void PrintValues(ostream& stream, T& vec, int num_comps)
    {
    for (size_t cc=0; cc < vec.size(); cc++)
      {
      bool add_paren = (num_comps > 1 && 
        (static_cast<int>(cc) % num_comps == 0));
      if (add_paren)
        {
        stream << (cc > 0? "), (" : "(");
        }
      else if (cc  > 0)
        {
        stream << ", ";
        }
      stream << vec[cc];
      }
    if (num_comps > 1)
      {
      stream << ")";
      }
    stream << " ";
    }
public:
  typedef vtkstd::vector<vtkIdType> IdTypeVector;
  IdTypeVector IdTypeValues;

  typedef vtkstd::vector<double> DoubleVector;
  DoubleVector DoubleValues;
  void PrintValues(ostream& stream, int num_comps)
    {
    if (this->IdTypeValues.size() > 0)
      {
      this->PrintValues<IdTypeVector>(stream, this->IdTypeValues, num_comps);
      }
    else if (this->DoubleValues.size() > 0)
      {
      this->PrintValues<DoubleVector>(stream, this->DoubleValues, num_comps);
      }
    }
};

vtkStandardNewMacro(vtkQuerySelectionSource);
//----------------------------------------------------------------------------
vtkQuerySelectionSource::vtkQuerySelectionSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internals = new vtkInternals();

  this->TermMode = NONE;
  this->Operator = NONE;
  this->FieldType = vtkSelectionNode::CELL;
  this->Inverse = false;

  this->ArrayName = 0;
  this->ArrayComponent = 0;

  this->CompositeIndex = -1;
  this->HierarchicalIndex = -1;
  this->HierarchicalLevel = -1;
  this->ProcessID = -1;

  this->ContainingCells = 0;
  this->UserFriendlyText = 0;
}

//----------------------------------------------------------------------------
vtkQuerySelectionSource::~vtkQuerySelectionSource()
{
  this->SetArrayName(0);

  delete this->Internals;
  this->Internals = 0;

  delete [] this->UserFriendlyText;
  this->UserFriendlyText = 0;
}

//----------------------------------------------------------------------------
int vtkQuerySelectionSource::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // We can handle multiple piece request.
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkQuerySelectionSource::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  vtkSelection* output = vtkSelection::GetData(outputVector);
  vtkSelectionNode* selNode = vtkSelectionNode::New();
  output->AddNode(selNode);
  selNode->Delete();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = 0;
  if (outInfo->Has(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  if (this->ProcessID >= 0 && piece != this->ProcessID)
    {
    return 1;
    }

  vtkInformation* props = selNode->GetProperties();

  // Add qualifiers.
  if (this->CompositeIndex >= 0)
    {
    props->Set(vtkSelectionNode::COMPOSITE_INDEX(), this->CompositeIndex);
    }

  if (this->HierarchicalLevel >= 0)
    {
    props->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), this->HierarchicalLevel);
    }

  if (this->HierarchicalIndex >= 0)
    {
    props->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), this->HierarchicalIndex);
    }

  // Set field type.
  props->Set(vtkSelectionNode::FIELD_TYPE(), this->FieldType);

  // Determine content type based on the TermMode and Operator.
  int content_type = -1;
  if (this->TermMode == LOCATION && this->Operator == IS_ONE_OF)
    {
    content_type = vtkSelectionNode::LOCATIONS;
    }
  else if (this->TermMode == BLOCK && this->Operator == IS_ONE_OF)
    {
    content_type = vtkSelectionNode::BLOCKS;
    }
  else if (this->TermMode == ID || this->TermMode == GLOBALID ||
      this->TermMode == ARRAY)
    {
    if (this->Operator == IS_ONE_OF)
      {
      switch (this->TermMode)
        {
      case ARRAY:
        content_type = vtkSelectionNode::VALUES;
        break;

      case ID:
        content_type = vtkSelectionNode::INDICES;
        break;

      case GLOBALID:
        content_type = vtkSelectionNode::GLOBALIDS;
        break;
        }
      }
    else
      {
      content_type = vtkSelectionNode::THRESHOLDS;
      // if Operator == IS_GE or IS_LE then we will create the threshold pairs.
      }
    }
  else
    {
    // TODO: Handle block-based selection or process based selection etc.
    //vtkErrorMacro("Unsupported Operator/Term combination.");
    return 1;
    }

  props->Set(vtkSelectionNode::CONTENT_TYPE(), content_type);
  props->Set(vtkSelectionNode::CONTAINING_CELLS(), this->ContainingCells);
  props->Set(vtkSelectionNode::COMPONENT_NUMBER(), this->ArrayComponent);
  vtkAbstractArray* selectionList = this->BuildSelectionList();
  if (selectionList)
    {
    // HACK: Look at vtkExtractSelectedThresholds for details on this hack.
    if (this->TermMode == ID)
      {
      selectionList->SetName("vtkIndices");
      }
    else if (this->TermMode == GLOBALID)
      {
      selectionList->SetName("vtkGlobalIds");
      }
    else if (this->ArrayName)
      {
      selectionList->SetName(this->ArrayName);
      }
    selNode->SetSelectionList(selectionList);
    selectionList->Delete();
    }

  return 1;
}

namespace
{
  // array: the output vtkArray 
  // term_mode, op : identify the filter's term mode and operator type
  // min, max: the min and max values to use for LE/GE expressions.
  // values: input values.
  template <class vtkArray, class stlVector, class base_type>
  void vtkQuerySelectionSourceBuildSelectionList(
    vtkArray* array,
    int term_mode, int op,
    base_type min, base_type max,
    stlVector& values)
    {
    if (term_mode == vtkQuerySelectionSource::LOCATION &&
      op == vtkQuerySelectionSource::IS_ONE_OF)
      {
      array->SetNumberOfComponents(3);
      array->SetNumberOfTuples(static_cast<vtkIdType>(values.size()/3));
      }
    else if (op == vtkQuerySelectionSource::IS_BETWEEN)
      {
      array->SetNumberOfComponents(2);
      array->SetNumberOfTuples(static_cast<vtkIdType>(values.size()/2));
      }
    else if(op == vtkQuerySelectionSource::IS_GE ||
      op == vtkQuerySelectionSource::IS_LE)
      {
      array->SetNumberOfComponents(2);
      array->SetNumberOfTuples(static_cast<vtkIdType>(values.size()));
      }
    else
      {
      array->SetNumberOfComponents(1);
      array->SetNumberOfTuples(static_cast<vtkIdType>(values.size()));
      }

    vtkIdType numValues = array->GetNumberOfTuples() *
      array->GetNumberOfComponents();
    vtkIdType cc=0;
    typename stlVector::iterator iter;
    for (iter = values.begin(); iter != values.end() && cc < numValues ; ++iter)
      {
      if (op == vtkQuerySelectionSource::IS_LE)
        {
        array->SetValue(cc++, min);
        }
      array->SetValue(cc++, *iter);
      if (op == vtkQuerySelectionSource::IS_GE)
        {
        array->SetValue(cc++, max);
        }
      }
    }
};

//----------------------------------------------------------------------------
vtkAbstractArray* vtkQuerySelectionSource::BuildSelectionList()
{
  if (this->Internals->IdTypeValues.size() > 0)
    {
    vtkIdTypeArray* array = vtkIdTypeArray::New();
    vtkQuerySelectionSourceBuildSelectionList<
      vtkIdTypeArray, vtkInternals::IdTypeVector, vtkIdType>(array,
      this->TermMode, this->Operator,
      VTK_INT_MIN, VTK_INT_MAX, //FIXME for 64 bit idtype.
      this->Internals->IdTypeValues);
    return array;
    }
  else if (this->Internals->DoubleValues.size() > 0)
    {
    vtkDoubleArray* array = vtkDoubleArray::New();
    vtkQuerySelectionSourceBuildSelectionList<vtkDoubleArray,
      vtkInternals::DoubleVector, double >(array,
      this->TermMode, this->Operator,
      VTK_DOUBLE_MIN, VTK_DOUBLE_MAX,
      this->Internals->DoubleValues);
    return array;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::SetNumberOfDoubleValues(unsigned int cc)
{
  this->Internals->DoubleValues.resize(cc);
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::SetNumberOfIdTypeValues(unsigned int cc)
{
  this->Internals->IdTypeValues.resize(cc);
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::SetDoubleValues(double* values)
{
  for (vtkInternals::DoubleVector::iterator iter =
    this->Internals->DoubleValues.begin();
    iter != this->Internals->DoubleValues.end(); ++iter)
      {
      *iter = *values;
      values++;
      }

}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::SetIdTypeValues(vtkIdType* values)
{
  for (vtkInternals::IdTypeVector::iterator iter =
    this->Internals->IdTypeValues.begin();
    iter != this->Internals->IdTypeValues.end(); ++iter)
      {
      *iter = *values;
      values++;
      }

}

//----------------------------------------------------------------------------
const char* vtkQuerySelectionSource::GetUserFriendlyText()
{
  delete [] this->UserFriendlyText;
  this->UserFriendlyText =0;

  vtksys_ios::ostringstream stream;

  stream << (this->Inverse? "Inverse Select " : "Select ");
  switch (this->FieldType)
    {
  case vtkSelectionNode::CELL:
    stream << "Cells ";
    break;

  case vtkSelectionNode::POINT:
    if (this->ContainingCells)
      {
      stream << "Cells containing Points ";
      }
    else
      {
      stream << "Points ";
      }
    break;

  case vtkSelectionNode::VERTEX:
    stream << "Vertices ";
    break;

  case vtkSelectionNode::EDGE:
    stream << "Edges ";
    break;

  case vtkSelectionNode::ROW:
    stream << "Rows " ;
    break;
    }
  stream << "where ";

  bool add_and = true;
  switch (this->TermMode)
    {
  case ID:
    stream << "ID ";
    break;

  case GLOBALID:
    stream << "GLOBALID ";
    break;

  case ARRAY:
    stream << (this->ArrayName? this->ArrayName : "(invalid-array)");
    stream << " ";
    if (this->ArrayComponent >= 0)
      {
      stream << "(" << this->ArrayComponent << ") ";
      }
    else
      {
      stream << "(Mag) ";
      }
    break;

  case LOCATION:
    stream << "Location ";
    break;

  case BLOCK:
    stream << "Block ID ";
    break;

  case NONE:
    add_and = false;
    break;
    }

  if (this->TermMode != NONE)
    {
    switch (this->Operator)
      {
    case IS_ONE_OF:
      stream << "is one of ";
      if (this->TermMode == LOCATION)
        {
        this->Internals->PrintValues(stream, 3);
        }
      else
        {
        this->Internals->PrintValues(stream, 1);
        }
      break;

    case IS_BETWEEN:
      stream << "is between ";
      this->Internals->PrintValues(stream, 2);
      break;

    case IS_LE:
      stream << "is less equal than ";
      this->Internals->PrintValues(stream, 1);
      break;

    case IS_GE:
      stream << "is greater equal than ";
      this->Internals->PrintValues(stream, 1);
      break;
      }
    }

  if (this->CompositeIndex >= 0)
    {
    stream << "\n " << (add_and? "AND " : "")
           << "BlockID is " << this->CompositeIndex << " ";
    add_and = true;
    }
  if (this->HierarchicalLevel >= 0)
    {
    stream << "\n " << (add_and? "AND " : "");
    stream << "AMR Level is " << this->HierarchicalLevel << " ";
    add_and = true;
    }
  if (this->HierarchicalIndex >= 0)
    {
    stream << "\n " << (add_and? "AND " : "");
    stream << "AMR Block is " << this->HierarchicalIndex << " ";
    add_and = true;
    }
  if (this->ProcessID >= 0)
    {
    stream << "\n " << (add_and? "AND " : "");
    stream << "Process is " << this->ProcessID << " ";
    add_and = true;
    }

  this->UserFriendlyText = vtksys::SystemTools::DuplicateString(
    stream.str().c_str());
  return this->UserFriendlyText;
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


