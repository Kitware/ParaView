/*=========================================================================

 Program:   ParaView
 Module:    vtkPVProminentValuesInformation.cxx

 Copyright (c) Kitware, Inc.
 All rights reserved.
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 cxx     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPVProminentValuesInformation.h"

#include "vtkAbstractArray.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkClientServerStream.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExecutive.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKey.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVPostFilter.h"
#include "vtkPointData.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <map>
#include <set>
#include <sstream>
#include <vector>

#define VTK_MAX_CATEGORICAL_VALS (32)

namespace
{
typedef std::map<int, std::set<std::vector<vtkVariant> > > vtkInternalDistinctValuesBase;
}

class vtkPVProminentValuesInformation::vtkInternalDistinctValues
  : public vtkInternalDistinctValuesBase
{
};

vtkStandardNewMacro(vtkPVProminentValuesInformation);

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation::vtkPVProminentValuesInformation()
{
  this->PortNumber = 0;
  this->FieldName = 0;
  this->FieldAssociation = 0;
  this->DistinctValues = 0;
  this->InitializeParameters();
  this->Initialize();
  this->Force = false;
  this->Valid = true;
}

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation::~vtkPVProminentValuesInformation()
{
  this->Initialize();
  this->InitializeParameters();
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::InitializeParameters()
{
  this->PortNumber = 0;
  this->SetFieldName(0);
  this->SetFieldAssociation(0);
  this->NumberOfComponents = -2;
  this->Uncertainty = -1.;
  this->Fraction = -1.;
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::Initialize()
{
  if (this->DistinctValues)
  {
    delete this->DistinctValues;
    this->DistinctValues = 0;
  }
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "PortNumber: " << this->PortNumber << endl;
  if (this->FieldName)
  {
    os << indent << "FieldName: " << this->FieldName << endl;
  }
  if (this->FieldAssociation)
  {
    os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  }
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "DistinctValues :" << endl;
  if (this->DistinctValues)
  {
    for (vtkInternalDistinctValues::iterator cit = this->DistinctValues->begin();
         cit != this->DistinctValues->end(); ++cit)
    {
      os << i2 << "Component " << cit->first << " (" << cit->second.size() << " values )" << endl;
      for (vtkInternalDistinctValues::mapped_type::iterator eit = cit->second.begin();
           eit != cit->second.end(); ++eit)
      {
        os << i3;
        for (std::vector<vtkVariant>::const_iterator vit = eit->begin(); vit != eit->end(); ++vit)
        {
          os << " " << vit->ToString();
        }
        os << endl;
      }
    }
  }
  else
  {
    os << i2 << "None" << endl;
  }
  os << "Fraction: " << this->Fraction << endl;
  os << "Uncertainty: " << this->Uncertainty << endl;
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::SetNumberOfComponents(int numComps)
{
  if (this->NumberOfComponents == numComps)
  {
    return;
  }
  this->NumberOfComponents = numComps;
  if (this->DistinctValues)
  {
    this->DistinctValues->clear();
  }
  if (numComps <= 0)
  {
    this->NumberOfComponents = 0;
    return;
  }
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::DeepCopy(vtkPVProminentValuesInformation* info)
{
  this->DeepCopyParameters(info);

  // Copy the list of unique values for each component.
  if (this->DistinctValues && !info->DistinctValues)
  {
    delete this->DistinctValues;
    this->DistinctValues = 0;
  }
  else if (info->DistinctValues)
  {
    if (!this->DistinctValues)
    {
      this->DistinctValues = new vtkInternalDistinctValues;
    }
    *this->DistinctValues = *info->DistinctValues;
  }
}

//----------------------------------------------------------------------------
int vtkPVProminentValuesInformation::Compare(vtkPVProminentValuesInformation* info)
{
  if (info == NULL)
  {
    return 0;
  }
  if (strcmp(info->GetFieldName(), this->FieldName) == 0 &&
    strcmp(info->GetFieldAssociation(), this->FieldAssociation) == 0 &&
    info->GetNumberOfComponents() == this->NumberOfComponents)
  {
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyFromObject(vtkObject* obj)
{
  // vtkPVProminentValuesInformation may be collected info from a
  // `vtkPVDataRepresentation` subclass (in which case we're collecting
  // representation data information) or a `vtkAlgorithm`.
  // So we handle the two cases here.

  vtkDataObject* dobj = nullptr;
  if (vtkPVDataRepresentation* repr = vtkPVDataRepresentation::SafeDownCast(obj))
  {
    dobj = vtkDataObject::SafeDownCast(repr->GetRenderedDataObject(0));
  }
  else if (auto algo = vtkAlgorithm::SafeDownCast(obj))
  {
    // We don't use vtkAlgorithm::GetOutputDataObject() since that calls a
    // UpdateDataObject() pass, which may raise errors if the algo is not
    // fully setup yet.
    if (strcmp(algo->GetClassName(), "vtkPVNullSource") == 0)
    {
      // Don't gather any data information from the hypothetical null source.
      return;
    }
    auto info = algo->GetExecutive()->GetOutputInformation(this->PortNumber);
    if (!info || vtkDataObject::GetData(info) == NULL)
    {
      return;
    }
    dobj = algo->GetOutputDataObject(this->PortNumber);
  }

  if (vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    this->CopyFromCompositeDataSet(cds);
  }
  else if (dobj)
  {
    this->CopyFromLeafDataObject(dobj);
  }
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::DeepCopyParameters(vtkPVProminentValuesInformation* other)
{
  this->SetFieldName(other->GetFieldName());
  this->SetFieldAssociation(other->GetFieldAssociation());
  this->SetNumberOfComponents(other->GetNumberOfComponents());
  this->Fraction = other->Fraction;
  this->Uncertainty = other->Uncertainty;
  this->Force = other->Force;
  this->Valid = other->Valid;
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyFromCompositeDataSet(vtkCompositeDataSet* cds)
{
  if (!cds)
  {
    return;
  }

  vtkCompositeDataIterator* iter = cds->NewIterator();
  if (!iter)
  {
    vtkErrorMacro("Could not create iterator.");
    return;
  }
  vtkNew<vtkPVProminentValuesInformation> other;
  other->DeepCopyParameters(this);

  iter->SkipEmptyNodesOn();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataObject* node = iter->GetCurrentDataObject();
    if (vtkCompositeDataSet::SafeDownCast(node))
    { // only visit leaf nodes.
      continue;
    }
    other->Initialize();
    other->CopyFromLeafDataObject(node);
    this->AddInformation(other.GetPointer());
  }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyFromLeafDataObject(vtkDataObject* dobj)
{
  if (!dobj)
  {
    return;
  }

  vtkFieldData* fieldData = nullptr;
  const int fieldAssoc = vtkDataObject::GetAssociationTypeFromString(this->FieldAssociation);
  switch (fieldAssoc)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
      // try points first, then cells.
      fieldData = dobj->GetAttributesAsFieldData(vtkDataObject::FIELD_ASSOCIATION_POINTS);
      if (fieldData == nullptr || fieldData->GetAbstractArray(this->FieldName) == nullptr)
      {
        fieldData = dobj->GetAttributesAsFieldData(vtkDataObject::FIELD_ASSOCIATION_CELLS);
      }
      break;
    default:
      fieldData = dobj->GetAttributesAsFieldData(fieldAssoc);
      break;
  }
  if (auto array = fieldData ? fieldData->GetAbstractArray(this->FieldName) : nullptr)
  {
    this->CopyDistinctValuesFromObject(array);
  }
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyDistinctValuesFromObject(vtkAbstractArray* array)
{
  // Check whether each component of the array takes on a small number
  // of unique values (i.e, test whether the array represents samples
  // from a discrete set or a continuum).
  // When there is more than 1 component, we also test whether the
  // tuples themselves behave discretely.
  if (this->DistinctValues)
  {
    this->DistinctValues->clear();
  }
  else
  {
    this->DistinctValues = new vtkInternalDistinctValues;
  }
  int nc = this->GetNumberOfComponents();
  vtkNew<vtkVariantArray> cvalues;
  std::vector<vtkVariant> tuple;
  // bool tooManyValues;
  for (int c = (nc > 1 ? -1 : 0); c < nc; ++c)
  {
    int tupleSize = c < 0 ? nc : 1;
    tuple.resize(tupleSize);
    std::set<std::vector<vtkVariant> >& compDistincts((*this->DistinctValues)[c]);
    cvalues->Initialize();
    unsigned int maxDiscreteValues = array->GetMaxDiscreteValues();
    if (this->Force)
    {
      array->SetMaxDiscreteValues(VTK_UNSIGNED_INT_MAX);
    }
    array->GetProminentComponentValues(c, cvalues.GetPointer(), 0., 0.);
    array->SetMaxDiscreteValues(maxDiscreteValues);
    vtkIdType nt = cvalues->GetNumberOfTuples();
    if (nt > 0)
    {
      for (vtkIdType t = 0; t < nt; ++t)
      {
        for (int i = 0; i < tupleSize; ++i)
        {
          tuple[i] = cvalues->GetValue(i + t * tupleSize);
        }
        compDistincts.insert(tuple);
        /*
        if (compDistincts.insert(tuple).second)
          {
          if ((tooManyValues =
            compDistincts.size() > vtkAbstractArray::MAX_DISCRETE_VALUES))
            break;
          }
          */
      }
      this->Valid = true;
    }
    else
    {
      // if there is no tuples provided, it means we were unable
      // to determine the prominent values and this information
      // is invalid
      this->Valid = false;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::AddInformation(vtkPVInformation* info)
{
  if (!info)
  {
    return;
  }

  vtkPVProminentValuesInformation* aInfo = vtkPVProminentValuesInformation::SafeDownCast(info);
  if (!aInfo)
  {
    vtkErrorMacro("Could not downcast info to array info.");
    return;
  }
  if (!this->DistinctValues)
  {
    // If this object is uninitialized, copy.
    this->DeepCopy(aInfo);
  }
  else
  {
    // Add unique values to our own.
    this->AddDistinctValues(aInfo);
  }
  this->Valid = this->Valid && aInfo->GetValid();
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Copy parameter values to stream.
  *css << this->PortNumber << std::string(this->FieldAssociation) << std::string(this->FieldName)
       << this->NumberOfComponents << this->Fraction << this->Uncertainty << this->Force
       << this->Valid;

  // Now copy results to stream.
  int numberOfDistinctValueComponents =
    static_cast<int>(this->DistinctValues ? this->DistinctValues->size() : 0);
  *css << numberOfDistinctValueComponents;
  if (numberOfDistinctValueComponents)
  {
    // Iterate over component numbers:
    vtkInternalDistinctValues::iterator cit;
    for (cit = this->DistinctValues->begin(); cit != this->DistinctValues->end(); ++cit)
    {
      unsigned nuv = static_cast<unsigned>(cit->second.size());
      *css << cit->first << nuv;
      vtkInternalDistinctValues::mapped_type::iterator eit;
      for (eit = cit->second.begin(); eit != cit->second.end(); ++eit)
      {
        std::vector<vtkVariant>::const_iterator vit;
        for (vit = eit->begin(); vit != eit->end(); ++vit)
        {
          *css << *vit;
        }
      }
    }
  }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyFromStream(const vtkClientServerStream* css)
{
  int pos = 0;
  vtkStdString fieldAssoc;
  vtkStdString fieldName;
  if (!css->GetArgument(0, pos++, &this->PortNumber))
  {
    vtkErrorMacro("Error parsing dataset port number from message.");
    return;
  }

  if (!css->GetArgument(0, pos++, &fieldAssoc))
  {
    vtkErrorMacro("Error parsing field association from message.");
    return;
  }
  this->SetFieldAssociation(fieldAssoc.c_str());

  if (!css->GetArgument(0, pos++, &fieldName))
  {
    vtkErrorMacro("Error parsing field name from message.");
    return;
  }
  this->SetFieldName(fieldName.c_str());

  if (!css->GetArgument(0, pos++, &this->NumberOfComponents))
  {
    vtkErrorMacro("Error parsing number of components from message.");
    return;
  }

  if (!css->GetArgument(0, pos++, &this->Fraction))
  {
    vtkErrorMacro("Error parsing fraction from message.");
    return;
  }

  if (!css->GetArgument(0, pos++, &this->Uncertainty))
  {
    vtkErrorMacro("Error parsing uncertainty from message.");
    return;
  }

  if (!css->GetArgument(0, pos++, &this->Force))
  {
    vtkErrorMacro("Error parsing force flag from message.");
    return;
  }

  if (!css->GetArgument(0, pos++, &this->Valid))
  {
    vtkErrorMacro("Error parsing valid flag from message.");
    return;
  }

  int numberOfDistinctValueComponents;
  if (!css->GetArgument(0, pos++, &numberOfDistinctValueComponents))
  {
    vtkErrorMacro("Error parsing unique value existence from message.");
    return;
  }
  if (!numberOfDistinctValueComponents && this->DistinctValues)
  {
    this->DistinctValues->clear();
  }
  else if (numberOfDistinctValueComponents)
  {
    if (!this->DistinctValues)
    {
      this->DistinctValues = new vtkInternalDistinctValues;
    }
    else
    {
      this->DistinctValues->clear();
    }
    for (int i = 0; i < numberOfDistinctValueComponents; ++i)
    {
      int component;
      if (!css->GetArgument(0, pos++, &component))
      {
        vtkErrorMacro("Error decoding the " << i << "-th unique-value component ID.");
        return;
      }
      unsigned nuv;
      if (!css->GetArgument(0, pos++, &nuv))
      {
        vtkErrorMacro("Error decoding the number of unique values for component " << i);
        return;
      }
      int tupleSize = (component < 0 ? this->NumberOfComponents : 1);
      std::vector<vtkVariant> tuple;
      tuple.resize(tupleSize);
      for (unsigned j = 0; j < nuv; ++j)
      {
        for (int k = 0; k < tupleSize; ++k)
        {
          if (!css->GetArgument(0, pos, &tuple[k]))
          {
            vtkErrorMacro("Error decoding the " << k << "-th entry of the " << j
                                                << "-th unique tuple for component " << i);
            return;
          }
        }
        (*this->DistinctValues)[component].insert(tuple);
      }
    }
  }
}

#define VTK_PROMINENT_MAGIC_NUMBER 573167
//-----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyParametersToStream(vtkMultiProcessStream& mps)
{
  this->Superclass::CopyParametersToStream(mps);
  vtkTypeUInt32 magic_number = VTK_PROMINENT_MAGIC_NUMBER;
  mps << magic_number << this->PortNumber << std::string(this->FieldAssociation)
      << std::string(this->FieldName) << this->NumberOfComponents << this->Fraction
      << this->Uncertainty << this->Force << this->Valid;
}

//-----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::CopyParametersFromStream(vtkMultiProcessStream& mps)
{
  this->Superclass::CopyParametersFromStream(mps);
  vtkTypeUInt32 magic_number;
  std::string fieldAssoc;
  std::string fieldName;
  mps >> magic_number >> this->PortNumber >> fieldAssoc >> fieldName >> this->NumberOfComponents >>
    this->Fraction >> this->Uncertainty >> this->Force >> this->Valid;
  if (magic_number != VTK_PROMINENT_MAGIC_NUMBER)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
  this->SetFieldAssociation(fieldAssoc.c_str());
  this->SetFieldName(fieldName.c_str());
}

//-----------------------------------------------------------------------------
void vtkPVProminentValuesInformation::AddDistinctValues(vtkPVProminentValuesInformation* info)
{
  if (!info->DistinctValues || !this->DistinctValues)
  { // Some information is uninitialized; do nothing.
    return;
  }

  for (int i = 0; i <= this->NumberOfComponents; ++i)
  {
    vtkInternalDistinctValues::iterator bit = info->DistinctValues->find(i);
    vtkInternalDistinctValues::mapped_type::iterator
      eit; // iterator over entries of component [ab]it->second.
    bool tooManyValues = false;
    if (bit != info->DistinctValues->end())
    { // Add info's values to our list of unique keys
      for (eit = bit->second.begin(); eit != bit->second.end(); ++eit)
      {
        if ((*this->DistinctValues)[i].insert(*eit).second &&
          ((*this->DistinctValues)[i].size() > vtkAbstractArray::MAX_DISCRETE_VALUES &&
              !this->Force))
        {
          tooManyValues = true;
          break;
        }
      }
    }

    // If the union of values is too large, delete the list of values
    if (tooManyValues)
    {
      this->DistinctValues->erase(i);
      this->Valid = false;
    }
  }
}

//-----------------------------------------------------------------------------
vtkAbstractArray* vtkPVProminentValuesInformation::GetProminentComponentValues(int component)
{
  vtkVariantArray* va = 0;
  vtkInternalDistinctValues::iterator compEntry;
  vtkIdType nt = 0;
  if (component < 0 && this->NumberOfComponents == 1)
  {
    component = 0;
  }
  if (!this->DistinctValues ||
    (compEntry = this->DistinctValues->find(component)) == this->DistinctValues->end() ||
    (nt = static_cast<vtkIdType>(compEntry->second.size())) == 0)
  {
    return va;
  }

  va = vtkVariantArray::New();
  int nc = (component < 0 ? this->NumberOfComponents : 1);
  va->SetNumberOfComponents(nc);
  va->Allocate(nt * nc);
  vtkInternalDistinctValues::mapped_type::iterator eit;
  for (eit = compEntry->second.begin(); eit != compEntry->second.end(); ++eit)
  {
    for (int i = 0; i < nc; ++i)
    {
      va->InsertNextValue((*eit)[i]);
    }
  }
  return va;
}
