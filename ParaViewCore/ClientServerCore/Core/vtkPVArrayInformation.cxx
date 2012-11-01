/*=========================================================================

 Program:   ParaView
 Module:    vtkPVArrayInformation.cxx

 Copyright (c) Kitware, Inc.
 All rights reserved.
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 cxx     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPVArrayInformation.h"

#include "vtkAbstractArray.h"
#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationKey.h"
#include "vtkInformationIterator.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkPVPostFilter.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <map>
#include <set>
#include <vector>
#include <vtksys/ios/sstream>

#define VTK_MAX_CATEGORICAL_VALS (32)

namespace
{
  typedef std::vector<vtkStdString*> vtkInternalComponentNameBase;

  struct vtkPVArrayInformationInformationKey
  {
    vtkStdString Location;
    vtkStdString Name;
  };

  typedef std::vector<vtkPVArrayInformationInformationKey> vtkInternalInformationKeysBase;

  typedef std::map<int,std::set<std::vector<vtkVariant> > > vtkInternalUniqueValuesBase;
}

class vtkPVArrayInformation::vtkInternalComponentNames:
    public vtkInternalComponentNameBase
{
};

class vtkPVArrayInformation::vtkInternalInformationKeys:
    public vtkInternalInformationKeysBase
{
};

class vtkPVArrayInformation::vtkInternalUniqueValues:
    public vtkInternalUniqueValuesBase
{
};

vtkStandardNewMacro(vtkPVArrayInformation);

//----------------------------------------------------------------------------
vtkPVArrayInformation::vtkPVArrayInformation()
{
  this->Name = 0;
  this->Ranges = 0;
  this->ComponentNames = 0;
  this->DefaultComponentName = 0;
  this->InformationKeys = 0;
  this->UniqueValues = 0;
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkPVArrayInformation::~vtkPVArrayInformation()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::Initialize()
{
  this->SetName(0);
  this->DataType = VTK_VOID;
  this->NumberOfComponents = 0;
  this->NumberOfTuples = 0;

  if (this->ComponentNames)
    {
    for ( unsigned int i=0; i < this->ComponentNames->size(); ++i)
      {
      if ( this->ComponentNames->at(i) )
        {
        delete this->ComponentNames->at(i);
        }
      }
    this->ComponentNames->clear();
    delete this->ComponentNames;
    this->ComponentNames = 0;
    }

  if (this->DefaultComponentName)
    {
    delete this->DefaultComponentName;
    this->DefaultComponentName = 0;
    }

  if (this->Ranges)
    {
    delete[] this->Ranges;
    this->Ranges = 0;
    }
  this->IsPartial = 0;

  if(this->InformationKeys)
    {
    this->InformationKeys->clear();
    delete this->InformationKeys;
    this->InformationKeys = 0;
    }

  if ( this->UniqueValues )
    {
    delete this->UniqueValues;
    this->UniqueValues = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  int num, idx;
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();

  this->Superclass::PrintSelf(os, indent);
  if (this->Name)
    {
    os << indent << "Name: " << this->Name << endl;
    }
  os << indent << "DataType: " << this->DataType << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  if (this->ComponentNames)
    {
    os << indent << "ComponentNames:" << endl;
    for (unsigned int i = 0; i < this->ComponentNames->size(); ++i)
      {
      os << i2 << this->ComponentNames->at(i) << endl;
      }
    }
  os << indent << "NumberOfTuples: " << this->NumberOfTuples << endl;
  os << indent << "IsPartial: " << this->IsPartial << endl;

  os << indent << "Ranges :" << endl;
  num = this->NumberOfComponents;
  if (num > 1)
    {
    ++num;
    }
  for (idx = 0; idx < num; ++idx)
    {
    os << i2 << this->Ranges[2 * idx] << ", " << this->Ranges[2 * idx + 1]
        << endl;
    }

  os << indent << "InformationKeys :" << endl;
  if(this->InformationKeys)
    {
    num = this->GetNumberOfInformationKeys();
    for (idx = 0; idx < num; ++idx)
      {
      os << i2 << this->GetInformationKeyLocation(idx) << "::"
          << this->GetInformationKeyName(idx) << endl;
      }
    }
  else
    {
    os << i2 << "None" << endl;
    }

  os << indent << "UniqueValues :" << endl;
  if(this->UniqueValues)
    {
    for ( vtkInternalUniqueValues::iterator cit = this->UniqueValues->begin(); cit != this->UniqueValues->end(); ++ cit )
      {
      os << i2 << "Component " << cit->first << " (" << cit->second.size() << " values )" << endl;
      for ( vtkInternalUniqueValues::mapped_type::iterator eit = cit->second.begin(); eit != cit->second.end(); ++ eit )
        {
        os << i3;
        for (std::vector<vtkVariant>::const_iterator vit = eit->begin(); vit != eit->end(); ++ vit)
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
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetNumberOfComponents(int numComps)
{
  if (this->NumberOfComponents == numComps)
    {
    return;
    }
  if (this->Ranges)
    {
    delete[] this->Ranges;
    this->Ranges = NULL;
    }
  this->NumberOfComponents = numComps;
  if (numComps <= 0)
    {
    this->NumberOfComponents = 0;
    return;
    }
  if (numComps > 1)
    { // Extra range for vector magnitude (first in array).
    numComps = numComps + 1;
    }

  int idx;
  this->Ranges = new double[numComps * 2];
  for (idx = 0; idx < numComps; ++idx)
    {
    this->Ranges[2 * idx] = VTK_DOUBLE_MAX;
    this->Ranges[2 * idx + 1] = -VTK_DOUBLE_MAX;
    }

  if ( this->UniqueValues )
    this->UniqueValues->clear();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetComponentName(vtkIdType component,
    const char *name)
{
  if (component < 0 || name == NULL)
    {
    return;
    }

  unsigned int index = static_cast<unsigned int> (component);
  if (this->ComponentNames == NULL)
    {
    //delayed allocate
    this->ComponentNames
        = new vtkPVArrayInformation::vtkInternalComponentNames();
    }

  if (index == this->ComponentNames->size())
    {
    //the array isn't large enough, so we will resize
    this->ComponentNames->push_back(new vtkStdString(name));
    return;
    }
  else if (index > this->ComponentNames->size())
    {
    this->ComponentNames->resize(index + 1, NULL);
    }

  //replace an exisiting element
  vtkStdString *compName = this->ComponentNames->at(index);
  if (!compName)
    {
    compName = new vtkStdString(name);
    this->ComponentNames->at(index) = compName;
    }
  else
    {
    compName->assign(name);
    }
}

//----------------------------------------------------------------------------
const char* vtkPVArrayInformation::GetComponentName(vtkIdType component)
{
  unsigned int index = static_cast<unsigned int> (component);
  //check signed component for less than zero
  if (this->ComponentNames && component >= 0 && index
      < this->ComponentNames->size())
    {
    vtkStdString *compName = this->ComponentNames->at(index);
    if (compName)
      {
      return compName->c_str();
      }
    }
  else if (this->ComponentNames && component == -1
      && this->ComponentNames->size() >= 1)
    {
    //we have a scalar array, and we need the component name
    vtkStdString *compName = this->ComponentNames->at(0);
    if (compName)
      {
      return compName->c_str();
      }
    }
  //we have failed to find a user set component name, use the default component name
  this->DetermineDefaultComponentName(component, this->GetNumberOfComponents());
  return this->DefaultComponentName->c_str();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetComponentRange(int comp, double min, double max)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
    {
    vtkErrorMacro("Bad component");
    }
  if (this->NumberOfComponents > 1)
    { // Shift over vector mag range.
    ++comp;
    }
  if (comp < 0)
    { // anything less than 0 just defaults to the vector mag.
    comp = 0;
    }
  this->Ranges[comp * 2] = min;
  this->Ranges[comp * 2 + 1] = max;
}

//----------------------------------------------------------------------------
double* vtkPVArrayInformation::GetComponentRange(int comp)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
    {
    vtkErrorMacro("Bad component");
    return NULL;
    }
  if (this->NumberOfComponents > 1)
    { // Shift over vector mag range.
    ++comp;
    }
  if (comp < 0)
    { // anything less than 0 just defaults to the vector mag.
    comp = 0;
    }
  return this->Ranges + comp * 2;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentRange(int comp, double *range)
{
  double *ptr;

  ptr = this->GetComponentRange(comp);

  if (ptr == NULL)
    {
    range[0] = VTK_DOUBLE_MAX;
    range[1] = -VTK_DOUBLE_MAX;
    return;
    }

  range[0] = ptr[0];
  range[1] = ptr[1];
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetDataTypeRange(double range[2])
{
  int dataType = this->GetDataType();
  switch (dataType)
    {
    case VTK_BIT:
      range[0] = VTK_BIT_MAX;
      range[1] = VTK_BIT_MAX;
      break;
    case VTK_UNSIGNED_CHAR:
      range[0] = VTK_UNSIGNED_CHAR_MIN;
      range[1] = VTK_UNSIGNED_CHAR_MAX;
      break;
    case VTK_CHAR:
      range[0] = VTK_CHAR_MIN;
      range[1] = VTK_CHAR_MAX;
      break;
    case VTK_UNSIGNED_SHORT:
      range[0] = VTK_UNSIGNED_SHORT_MIN;
      range[1] = VTK_UNSIGNED_SHORT_MAX;
      break;
    case VTK_SHORT:
      range[0] = VTK_SHORT_MIN;
      range[1] = VTK_SHORT_MAX;
      break;
    case VTK_UNSIGNED_INT:
      range[0] = VTK_UNSIGNED_INT_MIN;
      range[1] = VTK_UNSIGNED_INT_MAX;
      break;
    case VTK_INT:
      range[0] = VTK_INT_MIN;
      range[1] = VTK_INT_MAX;
      break;
    case VTK_UNSIGNED_LONG:
      range[0] = VTK_UNSIGNED_LONG_MIN;
      range[1] = VTK_UNSIGNED_LONG_MAX;
      break;
    case VTK_LONG:
      range[0] = VTK_LONG_MIN;
      range[1] = VTK_LONG_MAX;
      break;
    case VTK_FLOAT:
      range[0] = VTK_FLOAT_MIN;
      range[1] = VTK_FLOAT_MAX;
      break;
    case VTK_DOUBLE:
      range[0] = VTK_DOUBLE_MIN;
      range[1] = VTK_DOUBLE_MAX;
      break;
    default:
      // Default value:
      range[0] = 0;
      range[1] = 1;
      break;
    }
}
//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddRanges(vtkPVArrayInformation *info)
{
  double *range;
  double *ptr = this->Ranges;
  int idx;

  if (this->NumberOfComponents != info->GetNumberOfComponents())
    {
    vtkErrorMacro("Component mismatch.");
    }

  if (this->NumberOfComponents > 1)
    {
    range = info->GetComponentRange(-1);
    if (range[0] < ptr[0])
      {
      ptr[0] = range[0];
      }
    if (range[1] > ptr[1])
      {
      ptr[1] = range[1];
      }
    ptr += 2;
    }

  for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
    range = info->GetComponentRange(idx);
    if (range[0] < ptr[0])
      {
      ptr[0] = range[0];
      }
    if (range[1] > ptr[1])
      {
      ptr[1] = range[1];
      }
    ptr += 2;
    }

  this->NumberOfTuples += info->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::DeepCopy( vtkPVArrayInformation* info )
{
  int num, idx;

  this->SetName(info->GetName());
  this->DataType = info->GetDataType();
  this->SetNumberOfComponents(info->GetNumberOfComponents());
  this->SetNumberOfTuples(info->GetNumberOfTuples());

  num = 2 * this->NumberOfComponents;
  if (this->NumberOfComponents > 1)
    {
    num += 2;
    }
  for (idx = 0; idx < num; ++idx)
    {
    this->Ranges[idx] = info->Ranges[idx];
    }

  //clear the vector of old data
  if (this->ComponentNames)
    {
    for ( unsigned int i=0; i < this->ComponentNames->size(); ++i)
      {
      if ( this->ComponentNames->at(i) )
        {
        delete this->ComponentNames->at(i);
        }
      }
    this->ComponentNames->clear();
    delete this->ComponentNames;
    this->ComponentNames = 0;
    }

  if (info->ComponentNames)
    {
    this->ComponentNames
        = new vtkPVArrayInformation::vtkInternalComponentNames();
    //copy the passed in components if they exist
    this->ComponentNames->reserve(info->ComponentNames->size());
    const char *name;
    for (unsigned i = 0; i < info->ComponentNames->size(); ++i)
      {
      name = info->GetComponentName(i);
      if (name)
        {
        this->SetComponentName(i, name);
        }
      }
    }

  if (!this->InformationKeys)
    {
    this->InformationKeys
        = new vtkPVArrayInformation::vtkInternalInformationKeys();
    }

  //clear the vector of old data
  this->InformationKeys->clear();

  if (info->InformationKeys)
    {
    //copy the passed in components if they exist
    for (unsigned i = 0; i < info->InformationKeys->size(); ++i)
      {
      this->InformationKeys->push_back(info->InformationKeys->at(i));
      }
    }

  // Copy the list of unique values for each component.
  if ( this->UniqueValues && ! info->UniqueValues )
    {
    delete this->UniqueValues;
    this->UniqueValues = 0;
    }
  else if ( info->UniqueValues )
    {
    if ( ! this->UniqueValues )
      {
      this->UniqueValues = new vtkInternalUniqueValues;
      }
    *this->UniqueValues = *info->UniqueValues;
    }
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::Compare(vtkPVArrayInformation *info)
{
  if (info == NULL)
    {
    return 0;
    }
  if (strcmp(info->GetName(), this->Name) == 0 && info->GetNumberOfComponents()
      == this->NumberOfComponents && this->GetNumberOfInformationKeys()
      == info->GetNumberOfInformationKeys())
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromObject(vtkObject* obj)
{
  if (!obj)
    {
    this->Initialize();
    }

  vtkAbstractArray* const array = vtkAbstractArray::SafeDownCast(obj);
  if (!array)
    {
    vtkErrorMacro("Cannot downcast to abstract array.");
    this->Initialize();
    return;
    }

  this->SetName(array->GetName());
  this->DataType = array->GetDataType();
  this->SetNumberOfComponents(array->GetNumberOfComponents());
  this->SetNumberOfTuples(array->GetNumberOfTuples());

  if (array->HasAComponentName())
    {
    const char *name;
    //copy the component names over
    for (int i = 0; i < this->GetNumberOfComponents(); ++i)
      {
      name = array->GetComponentName(i);
      if (name)
        {
        //each component doesn't have to be named
        this->SetComponentName(i, name);
        }
      }
    }

  if (vtkDataArray* const data_array = vtkDataArray::SafeDownCast(obj))
    {
    double range[2];
    double *ptr;
    int idx;

    ptr = this->Ranges;
    if (this->NumberOfComponents > 1)
      {
      // First store range of vector magnitude.
      data_array->GetRange(range, -1);
      *ptr++ = range[0];
      *ptr++ = range[1];
      }
    for (idx = 0; idx < this->NumberOfComponents; ++idx)
      {
      data_array->GetRange(range, idx);
      *ptr++ = range[0];
      *ptr++ = range[1];
      }
    }

  if(this->InformationKeys)
    {
    this->InformationKeys->clear();
    delete this->InformationKeys;
    this->InformationKeys = 0;
    }
  if (array->HasInformation())
    {
    vtkInformation* info = array->GetInformation();
    vtkInformationIterator* it = vtkInformationIterator::New();
    it->SetInformationWeak(info);
    it->GoToFirstItem();
    while (!it->IsDoneWithTraversal())
      {
      vtkInformationKey* key = it->GetCurrentKey();
      this->AddInformationKey(key->GetLocation(), key->GetName());
      it->GoToNextItem();
      }
    it->Delete();
    }

  // Check whether each component of the array takes on a small number
  // of unique values (i.e, test whether the array represents samples
  // from a discrete set or a continuum).
  // When there is more than 1 component, we also test whether the
  // tuples themselves behave discretely.
  if ( this->UniqueValues )
    {
    this->UniqueValues->clear();
    }
  else
    {
    this->UniqueValues = new vtkInternalUniqueValues;
    }
  int nc = this->GetNumberOfComponents();
  vtkNew<vtkVariantArray> cvalues;
  std::vector<vtkVariant> tuple;
  //bool tooManyValues;
  for (int c = (nc > 1 ? -1 : 0); c < nc; ++c)
    {
    int tupleSize = c < 0 ? nc : 1;
    tuple.resize(tupleSize);
    std::set<std::vector<vtkVariant> >& compUniques(
      (*this->UniqueValues)[c]);
    cvalues->Initialize();
    array->GetUniqueComponentValues(c, cvalues.GetPointer());
    vtkIdType nt = cvalues->GetNumberOfTuples();
    if (nt > 0)
      {
      for (vtkIdType t = 0; t < nt; ++t)
        {
        for (int i = 0; i < tupleSize; ++i)
          {
          tuple[i] = cvalues->GetValue(i + t * tupleSize);
          }
        compUniques.insert(tuple);
        /*
        if (compUniques.insert(tuple).second)
          {
          if ((tooManyValues =
            compUniques.size() > vtkAbstractArray::MAX_DISCRETE_VALUES))
            break;
          }
          */
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddInformation(vtkPVInformation* info)
{
  if (!info)
    {
    return;
    }

  vtkPVArrayInformation* aInfo = vtkPVArrayInformation::SafeDownCast(info);
  if (!aInfo)
    {
    vtkErrorMacro("Could not downcast info to array info.");
    return;
    }
  if (aInfo->GetNumberOfComponents() > 0)
    {
    if (this->NumberOfComponents == 0)
      {
      // If this object is uninitialized, copy.
      this->DeepCopy(aInfo);
      }
    else
      {
      // Leave everything but ranges and unique values as original, add ranges and unique values.
      this->AddRanges(aInfo);
      this->AddInformationKeys(aInfo);
      this->AddUniqueValues(aInfo);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Array name, data type, and number of components.
  *css << this->Name;
  *css << this->DataType;
  *css << this->NumberOfTuples;
  *css << this->NumberOfComponents;

  // Range of each component.
  int num = this->NumberOfComponents;
  if (this->NumberOfComponents > 1)
    {
    // First range is range of vector magnitude.
    ++num;
    }
  for (int i = 0; i < num; ++i)
    {
    *css << vtkClientServerStream::InsertArray(this->Ranges + 2 * i, 2);
    }

  //add in the component names
  num = static_cast<int> (this->ComponentNames ? this->ComponentNames->size()
      : 0);
  *css << num;
  vtkStdString *compName;
  for (int i = 0; i < num; ++i)
    {
    compName = this->ComponentNames->at(i);
    *css << (compName? compName->c_str() : static_cast<const char*>(NULL));
    }

  int nkeys = this->GetNumberOfInformationKeys();
  *css << nkeys;
  for (int key = 0; key < nkeys; key++)
    {
    const char* location = this->GetInformationKeyLocation(key);
    const char* name = this->GetInformationKeyName(key);
    *css << location << name;
    }

  int numberOfUniqueValueComponents = static_cast<int>(
    this->UniqueValues ? this->UniqueValues->size() : 0);
  *css << numberOfUniqueValueComponents;
  if (numberOfUniqueValueComponents)
    {
    // Iterate over component numbers:
    vtkInternalUniqueValues::iterator cit;
    for (cit = this->UniqueValues->begin(); cit != this->UniqueValues->end(); ++cit)
      {
      unsigned nuv = static_cast<unsigned>(cit->second.size());
      *css << cit->first << nuv;
      vtkInternalUniqueValues::mapped_type::iterator eit;
      for (eit = cit->second.begin(); eit != cit->second.end(); ++ eit)
        {
        std::vector<vtkVariant>::const_iterator vit;
        for (vit = eit->begin(); vit != eit->end(); ++ vit)
          {
          *css << *vit;
          }
        }
      }
    }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromStream(const vtkClientServerStream* css)
{
  // Array name.
  const char* name = 0;
  if (!css->GetArgument(0, 0, &name))
    {
    vtkErrorMacro("Error parsing array name from message.");
    return;
    }
  this->SetName(name);

  // Data type.
  if (!css->GetArgument(0, 1, &this->DataType))
    {
    vtkErrorMacro("Error parsing array data type from message.");
    return;
    }

  // Number of tuples.
  int num;
  if (!css->GetArgument(0, 2, &num))
    {
    vtkErrorMacro("Error parsing number of tuples from message.");
    return;
    }
  this->SetNumberOfTuples(num);

  // Number of components.
  if (!css->GetArgument(0, 3, &num))
    {
    vtkErrorMacro("Error parsing number of components from message.");
    return;
    }
  this->SetNumberOfComponents(num);

  if (num > 1)
    {
    num++;
    }

  // Range of each component.
  for (int i = 0; i < num; ++i)
    {
    if (!css->GetArgument(0, 4 + i, this->Ranges + 2 * i, 2))
      {
      vtkErrorMacro("Error parsing range of component.");
      return;
      }
    }
  int pos = 4 + num;
  int numOfComponentNames;
  if (!css->GetArgument(0, pos++, &numOfComponentNames))
    {
    vtkErrorMacro("Error parsing number of component names.");
    return;
    }

  if (numOfComponentNames > 0)
    {
      //clear the vector of old data
    if (this->ComponentNames)
      {
      for ( unsigned int i=0; i < this->ComponentNames->size(); ++i)
        {
        if ( this->ComponentNames->at(i) )
          {
          delete this->ComponentNames->at(i);
          }
        }
      this->ComponentNames->clear();
      delete this->ComponentNames;
      this->ComponentNames = 0;
      }
    this->ComponentNames
          = new vtkPVArrayInformation::vtkInternalComponentNames();

    this->ComponentNames->reserve(numOfComponentNames);

    for (int cc=0; cc < numOfComponentNames; cc++)
      {
      const char* comp_name;
      if (!css->GetArgument(0, pos++, &comp_name))
        {
        vtkErrorMacro("Error parsing component name from message.");
        return;
        }
      // note comp_name may be NULL, but that's okay.
      this->SetComponentName(cc, comp_name);
      }
    }

  // information keys
  int nkeys;
  if (!css->GetArgument(0, pos++, &nkeys))
    {
    return;
    }

  // Location and name for each key.
  if (this->InformationKeys)
    {
    this->InformationKeys->clear();
    delete this->InformationKeys;
    this->InformationKeys = 0;
    }
  for (int i = 0; i < nkeys; ++i)
    {
    if (!css->GetArgument(0, pos++, &name))
      {
      vtkErrorMacro("Error parsing information key location from message.");
      return;
      }
    vtkStdString key_location = name;

    if (!css->GetArgument(0, pos++, &name))
      {
      vtkErrorMacro("Error parsing information key name from message.");
      return;
      }
    vtkStdString key_name = name;
    this->AddInformationKey(key_location, key_name);
    }

  int numberOfUniqueValueComponents;
  if ( ! css->GetArgument( 0, pos++, &numberOfUniqueValueComponents ) )
    {
    vtkErrorMacro("Error parsing unique value existence from message.");
    return;
    }
  if ( ! numberOfUniqueValueComponents && this->UniqueValues )
    {
    this->UniqueValues->clear();
    }
  else if ( numberOfUniqueValueComponents )
    {
    if ( ! this->UniqueValues )
      {
      this->UniqueValues = new vtkInternalUniqueValues;
      }
    else
      {
      this->UniqueValues->clear();
      }
    for ( int i = 0; i < numberOfUniqueValueComponents; ++ i )
      {
      int component;
      if (!css->GetArgument(0, pos++, &component))
        {
        vtkErrorMacro( "Error decoding the " << i << "-th unique-value component ID." );
        return;
        }
      unsigned nuv;
      if ( ! css->GetArgument( 0, pos++, &nuv ) )
        {
        vtkErrorMacro( "Error decoding the number of unique values for component " << i );
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
            vtkErrorMacro("Error decoding the " << k << "-th entry of the " << j << "-th unique tuple for component " << i);
            return;
            }
          }
        (*this->UniqueValues)[component].insert(tuple);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVArrayInformation::DetermineDefaultComponentName(
    const int &component_no, const int &num_components)
{
  if (!this->DefaultComponentName)
    {
    this->DefaultComponentName = new vtkStdString();
    }

  this->DefaultComponentName->assign(vtkPVPostFilter::DefaultComponentName(component_no, num_components));
}

void vtkPVArrayInformation::AddInformationKeys(vtkPVArrayInformation *info)
{
  for (int k = 0; k < info->GetNumberOfInformationKeys(); k++)
    {
    this->AddUniqueInformationKey(info->GetInformationKeyLocation(k),
        info->GetInformationKeyName(k));
    }
}

void vtkPVArrayInformation::AddInformationKey(const char* location,
    const char* name)
{
  if(this->InformationKeys == NULL)
    {
    this->InformationKeys = new vtkInternalInformationKeys();
    }
  vtkPVArrayInformationInformationKey info = vtkPVArrayInformationInformationKey();
  info.Location = location;
  info.Name = name;
  this->InformationKeys->push_back(info);
}

void vtkPVArrayInformation::AddUniqueInformationKey(const char* location,
    const char* name)
{
  if (!this->HasInformationKey(location, name))
    {
    this->AddInformationKey(location, name);
    }
}

int vtkPVArrayInformation::GetNumberOfInformationKeys()
{
  return static_cast<int>(this->InformationKeys ? this->InformationKeys->size() : 0);
}

const char* vtkPVArrayInformation::GetInformationKeyLocation(int index)
{
  if (index < 0 || index >= this->GetNumberOfInformationKeys())
    return NULL;

  return this->InformationKeys->at(index).Location;
}

const char* vtkPVArrayInformation::GetInformationKeyName(int index)
{
  if (index < 0 || index >= this->GetNumberOfInformationKeys())
    return NULL;

  return this->InformationKeys->at(index).Name;
}

int vtkPVArrayInformation::HasInformationKey(const char* location,
    const char* name)
{
  for (int k = 0; k < this->GetNumberOfInformationKeys(); k++)
    {
    const char* key_location = this->GetInformationKeyLocation(k);
    const char* key_name = this->GetInformationKeyName(k);
    if (strcmp(location, key_location) == 0 && strcmp(name, key_name) == 0)
      {
      return 1;
      }
    }
  return 0;
}

void vtkPVArrayInformation::AddUniqueValues( vtkPVArrayInformation* info )
{
  if ( ! info->UniqueValues )
    { // Other info is uninitialized; keep the values we have.
    return;
    }

  if ( ! this->UniqueValues )
    {
    vtkDebugMacro("Merge array \"" << this->Name << "\" has no locals");
    this->UniqueValues = new vtkInternalUniqueValues;
    }

  for ( int i = 0; i <= this->NumberOfComponents; ++ i )
    {
    vtkInternalUniqueValues::iterator ait = this->UniqueValues->find(i);
    vtkInternalUniqueValues::iterator bit = info->UniqueValues->find(i);
    vtkInternalUniqueValues::mapped_type::iterator eit; // iterator over entries of component [ab]it->second.
    bool tooManyValues = false;
    if (
      ait == this->UniqueValues->end() ||
      bit == info->UniqueValues->end() )
      { // someone had too many values to be rendered as categorical data.
      tooManyValues = true;
      }
    else
      { // Add info's values to our list of unique keys
      for (eit = bit->second.begin(); eit != bit->second.end(); ++eit)
        {
        if (
          (*this->UniqueValues)[i].insert( *eit ).second &&
          (*this->UniqueValues)[i].size() > VTK_MAX_CATEGORICAL_VALS)
          {
          tooManyValues = true;
          break;
          }
        }
      }

    // If the union of values is too large, delete the list of values
    if ( tooManyValues )
      {
      this->UniqueValues->erase( i );
      }
    }
}

vtkAbstractArray* vtkPVArrayInformation::GetUniqueComponentValuesIfDiscrete( int component )
{
  vtkVariantArray* va = 0;
  vtkInternalUniqueValues::iterator compEntry;
  vtkIdType nt = 0;
  if (component < 0 && this->NumberOfComponents == 1)
    component = 0;
  if (
    ! this->UniqueValues ||
    (compEntry = this->UniqueValues->find(component)) == this->UniqueValues->end() ||
    (nt = static_cast<vtkIdType>(compEntry->second.size())) == 0)
    {
    return va;
    }

  va = vtkVariantArray::New();
  int nc = (component < 0 ? this->NumberOfComponents : 1);
  va->SetNumberOfComponents(nc);
  va->Allocate(nt * nc);
  vtkInternalUniqueValues::mapped_type::iterator eit;
  for (eit = compEntry->second.begin(); eit != compEntry->second.end(); ++ eit)
    {
    for (int i = 0; i < nc; ++i)
      {
      va->InsertNextValue((*eit)[i]);
      }
    }
  return va;
}
