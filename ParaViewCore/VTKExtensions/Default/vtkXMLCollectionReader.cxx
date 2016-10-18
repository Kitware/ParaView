/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLCollectionReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCollectionReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVInstantiator.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkXMLCollectionReader);

//----------------------------------------------------------------------------

struct vtkXMLCollectionReaderEntry
{
  const char* extension;
  const char* name;
};

class vtkXMLCollectionReaderString : public std::string
{
public:
  typedef std::string Superclass;
  vtkXMLCollectionReaderString()
    : Superclass()
  {
  }
  vtkXMLCollectionReaderString(const char* s)
    : Superclass(s)
  {
  }
  vtkXMLCollectionReaderString(const Superclass& s)
    : Superclass(s)
  {
  }
};
typedef std::vector<vtkXMLCollectionReaderString> vtkXMLCollectionReaderAttributeNames;
typedef std::vector<std::vector<vtkXMLCollectionReaderString> >
  vtkXMLCollectionReaderAttributeValueSets;
typedef std::map<vtkXMLCollectionReaderString, vtkXMLCollectionReaderString>
  vtkXMLCollectionReaderRestrictions;
class vtkXMLCollectionReaderInternals
{
public:
  std::vector<vtkXMLDataElement*> DataSets;
  std::vector<vtkXMLDataElement*> RestrictedDataSets;
  vtkXMLCollectionReaderAttributeNames AttributeNames;
  vtkXMLCollectionReaderAttributeValueSets AttributeValueSets;
  vtkXMLCollectionReaderRestrictions Restrictions;
  std::vector<vtkSmartPointer<vtkXMLReader> > Readers;
  static const vtkXMLCollectionReaderEntry ReaderList[];
};

//----------------------------------------------------------------------------
vtkXMLCollectionReader::vtkXMLCollectionReader()
{
  this->Internal = new vtkXMLCollectionReaderInternals;

  // Setup a callback for the internal readers to report progress.
  this->InternalProgressObserver = vtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(
    &vtkXMLCollectionReader::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->InternalForceMultiBlock = false;
  this->ForceOutputTypeToMultiBlock = 0;

  this->CurrentOutput = -1;
}

//----------------------------------------------------------------------------
vtkXMLCollectionReader::~vtkXMLCollectionReader()
{
  this->InternalProgressObserver->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::SetRestriction(const char* name, const char* value)
{
  this->SetRestrictionImpl(name, value, true);
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::SetRestrictionImpl(const char* name, const char* value, bool doModify)
{
  vtkXMLCollectionReaderRestrictions::iterator i = this->Internal->Restrictions.find(name);
  if (value && value[0])
  {
    // We have a value.  Set the restriction.
    if (i != this->Internal->Restrictions.end())
    {
      if (i->second != value)
      {
        // Replace the existing restriction value on this attribute.
        i->second = value;
        if (doModify)
        {
          this->Modified();
        }
      }
    }
    else
    {
      // Add the restriction on this attribute.
      this->Internal->Restrictions.insert(
        vtkXMLCollectionReaderRestrictions::value_type(name, value));
      if (doModify)
      {
        this->Modified();
      }
    }
  }
  else if (i != this->Internal->Restrictions.end())
  {
    // The value is empty.  Remove the restriction.
    this->Internal->Restrictions.erase(i);
    if (doModify)
    {
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetRestriction(const char* name)
{
  vtkXMLCollectionReaderRestrictions::const_iterator i = this->Internal->Restrictions.find(name);
  if (i != this->Internal->Restrictions.end())
  {
    return i->second.c_str();
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::SetRestrictionAsIndex(const char* name, int index)
{
  this->SetRestriction(name, this->GetAttributeValue(name, index));
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetRestrictionAsIndex(const char* name)
{
  return this->GetAttributeValueIndex(name, this->GetRestriction(name));
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::InternalProgressCallbackFunction(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<vtkXMLCollectionReader*>(clientdata)->InternalProgressCallback();
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::InternalProgressCallback()
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  vtkXMLReader* reader = this->Internal->Readers[this->CurrentOutput].GetPointer();
  float dataProgress = reader->GetProgress();
  float progress = this->ProgressRange[0] + dataProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    reader->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetDataSetName()
{
  return "Collection";
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Count the number of data sets in the file.
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numDataSets = 0;
  int i;
  for (i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "DataSet") == 0)
    {
      ++numDataSets;
    }
  }

  // Now read each data set.  Build the list of data sets, their
  // attributes, and the attribute values.
  this->Internal->AttributeNames.clear();
  this->Internal->AttributeValueSets.clear();
  this->Internal->DataSets.clear();
  for (i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "DataSet") == 0)
    {
      int j;
      this->Internal->DataSets.push_back(eNested);
      for (j = 0; j < eNested->GetNumberOfAttributes(); ++j)
      {
        this->AddAttributeNameValue(eNested->GetAttributeName(j), eNested->GetAttributeValue(j));
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXMLCollectionReader::SetupOutput(const char* filePath, int index)
{
  vtkXMLDataElement* ds = this->Internal->RestrictedDataSets[index];

  // Construct the name of the internal file.
  std::string fileName;
  const char* file = ds->GetAttribute("file");
  if (!(file[0] == '/' || file[1] == ':'))
  {
    fileName = filePath;
    if (fileName.length())
    {
      fileName += "/";
    }
  }
  fileName += file;

  // Get the file extension.
  std::string ext;
  std::string::size_type pos = fileName.rfind('.');
  if (pos != fileName.npos)
  {
    ext = fileName.substr(pos + 1);
  }

  // Search for the reader matching this extension.
  const char* rname = 0;
  for (const vtkXMLCollectionReaderEntry* r = this->Internal->ReaderList; !rname && r->extension;
       ++r)
  {
    if (ext == r->extension)
    {
      rname = r->name;
    }
  }

  // If a reader was found, allocate an instance of it for this output.
  if (rname)
  {
    if (!(this->Internal->Readers[index].GetPointer() &&
          strcmp(this->Internal->Readers[index]->GetClassName(), rname) == 0))
    {
      // Use the instantiator to create the reader.
      vtkObject* o = vtkPVInstantiator::CreateInstance(rname);
      vtkXMLReader* reader = vtkXMLReader::SafeDownCast(o);
      this->Internal->Readers[index] = reader;
      if (reader)
      {
        reader->Delete();
      }
      else
      {
        // The class was not registered with the instantiator.
        vtkErrorMacro("Error creating \"" << rname << "\" using vtkPVInstantiator.");
        if (o)
        {
          o->Delete();
        }
      }
    }
  }
  else
  {
    this->Internal->Readers[index] = 0;
  }

  // If we have a reader for this output, connect its output to our
  // output by sharing the data with a ShallowCopy.
  if (this->Internal->Readers[index].GetPointer())
  {
    // Assign the file name to the internal reader.
    this->Internal->Readers[index]->SetFileName(fileName.c_str());

    // Update the information on the internal reader's output.
    this->Internal->Readers[index]->UpdateInformation();

    // Allocate an instance of the same output type for our output.
    vtkDataObject* out = this->Internal->Readers[index]->GetOutputDataObject(0);

    return out->NewInstance();
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::BuildRestrictedDataSets()
{
  // Build list of data sets fitting the restrictions.
  this->Internal->RestrictedDataSets.clear();
  std::vector<vtkXMLDataElement*>::iterator d;
  for (d = this->Internal->DataSets.begin(); d != this->Internal->DataSets.end(); ++d)
  {
    vtkXMLDataElement* ds = *d;
    int matches = ds->GetAttribute("file") ? 1 : 0;
    vtkXMLCollectionReaderRestrictions::const_iterator r;
    for (r = this->Internal->Restrictions.begin();
         matches && r != this->Internal->Restrictions.end(); ++r)
    {
      const char* value = ds->GetAttribute(r->first.c_str());
      if (!(value && value == r->second))
      {
        matches = 0;
      }
    }
    if (matches)
    {
      this->Internal->RestrictedDataSets.push_back(ds);
    }
  }
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // need to Parse the file first
  if (!this->ReadXMLInformation())
  {
    vtkErrorMacro("Could not read file information");
    return 0;
  }

  vtkInformation* info = outputVector->GetInformationObject(0);

  this->BuildRestrictedDataSets();

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->FileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != filePath.npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }

  // Create the readers for each data set to be read.
  int n = static_cast<int>(this->Internal->RestrictedDataSets.size());
  this->Internal->Readers.resize(n);

  if (n == 1 && !this->ForceOutputTypeToMultiBlock)
  {
    vtkDataObject* output = this->SetupOutput(filePath.c_str(), 0);
    if (!output)
    {
      vtkErrorMacro("Could not determine the data type for the first dataset. "
        << "Please make sure this file format is supported.");
      return 0;
    }
    info->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
    this->InternalForceMultiBlock = false;
  }
  else
  {
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::New();
    info->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
    this->InternalForceMultiBlock = true;
  }

  return 1;
}

//----------------------------------------------------------------------------
// This method overloads the method in vtkXMLReader.  The ReadXMLInformation
// call is made in RequestDataObject (UpdateData is done at the beginning of
// the UpdateInformation pass, before RequestInformation on the algorithm)
int vtkXMLCollectionReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkInformation* dataInfo = info->Get(vtkDataObject::DATA_OBJECT())->GetInformation();
  if (dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
  {
    info->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  }

  size_t nBlocks = this->Internal->Readers.size();
  if (nBlocks == 1 && !this->ForceOutputTypeToMultiBlock)
  {
    this->Internal->Readers[0]->CopyOutputInformation(info, 0);
  }
  else
  {
    info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }

  this->Superclass::RequestInformation(request, inputVector, outputVector);

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::ReadXMLData()
{
  // need to Parse the file first
  if (!this->ReadXMLInformation())
  {
    return;
  }

  this->ReadXMLDataImpl();
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::ReadXMLDataImpl()
{
  this->BuildRestrictedDataSets();

  // Create the readers for each data set to be read.
  int n = static_cast<int>(this->Internal->RestrictedDataSets.size());
  this->Internal->Readers.resize(n);

  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  int updatePiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateNumPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updateGhostLevels =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->FileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != filePath.npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }

  if (!this->InternalForceMultiBlock)
  {
    vtkSmartPointer<vtkDataObject> actualOutput;
    actualOutput.TakeReference(this->SetupOutput(filePath.c_str(), 0));
    vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
    if (!output->IsA(actualOutput->GetClassName()))
    {
      vtkErrorMacro("This reader does not support datatype changing between time steps "
                    "unless the output is forced to be multi-block");
      return;
    }
    this->CurrentOutput = 0;
    this->ReadAFile(0, updatePiece, updateNumPieces, updateGhostLevels, output);
  }
  else
  {
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);

    unsigned int nBlocks = static_cast<unsigned int>(this->Internal->Readers.size());
    output->SetNumberOfBlocks(nBlocks);
    for (unsigned int i = 0; i < nBlocks; ++i)
    {
      vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(i));
      if (!block)
      {
        block = vtkMultiBlockDataSet::New();
        output->SetBlock(i, block);
        block->Delete();
      }

      this->CurrentOutput = i;
      vtkDataObject* actualOutput = this->SetupOutput(filePath.c_str(), i);
      this->ReadAFile(i, updatePiece, updateNumPieces, updateGhostLevels, actualOutput);
      block->SetNumberOfBlocks(updateNumPieces);
      block->SetBlock(updatePiece, actualOutput);

      // Set the block name from the DataSet name attribute, if any
      vtkXMLDataElement* ds = this->Internal->RestrictedDataSets[i];
      const char* name = ds ? ds->GetAttribute("name") : 0;
      if (name)
      {
        output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
      }

      actualOutput->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::ReadAFile(int index, int updatePiece, int updateNumPieces,
  int updateGhostLevels, vtkDataObject* actualOutput)
{
  // If we have a reader for the current output, use it.
  vtkXMLReader* r = this->Internal->Readers[index].GetPointer();
  if (r)
  {
    // Observe the progress of the internal reader.
    r->AddObserver(vtkCommand::ProgressEvent, this->InternalProgressObserver);

    // Give the update request from this output to its internal
    // reader.
    // Read the data.
    r->UpdatePiece(updatePiece, updateNumPieces, updateGhostLevels);

    // The internal reader is finished.  Remove the observer in case
    // we delete the reader later.
    r->RemoveObserver(this->InternalProgressObserver);

    // Share the new data with our output.
    actualOutput->ShallowCopy(r->GetOutputDataObject(0));

    // If a "name" attribute exists, store the name of the output in
    // its field data.
    vtkXMLDataElement* ds = this->Internal->RestrictedDataSets[index];
    const char* name = ds ? ds->GetAttribute("name") : 0;
    if (name)
    {
      vtkCharArray* nmArray = vtkCharArray::New();
      nmArray->SetName("Name");
      size_t len = strlen(name);
      nmArray->SetNumberOfTuples(static_cast<vtkIdType>(len) + 1);
      char* copy = nmArray->GetPointer(0);
      memcpy(copy, name, len);
      copy[len] = '\0';
      actualOutput->GetFieldData()->AddArray(nmArray);
      nmArray->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::AddAttributeNameValue(const char* name, const char* value)
{
  vtkXMLCollectionReaderString s = name;

  // Find an entry for this attribute.
  vtkXMLCollectionReaderAttributeNames::iterator n =
    std::find(this->Internal->AttributeNames.begin(), this->Internal->AttributeNames.end(), name);
  std::vector<vtkXMLCollectionReaderString>* values = 0;
  if (n == this->Internal->AttributeNames.end())
  {
    // Need to create an entry for this attribute.
    this->Internal->AttributeNames.push_back(name);

    this->Internal->AttributeValueSets.resize(this->Internal->AttributeValueSets.size() + 1);
    values = &*(this->Internal->AttributeValueSets.end() - 1);
  }
  else
  {
    values =
      &*(this->Internal->AttributeValueSets.begin() + (n - this->Internal->AttributeNames.begin()));
  }

  // Find an entry within the attribute for this value.
  s = value;
  std::vector<vtkXMLCollectionReaderString>::iterator i =
    std::find(values->begin(), values->end(), s);

  if (i == values->end())
  {
    // Need to add the value.
    values->push_back(value);
  }
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetNumberOfAttributes()
{
  return static_cast<int>(this->Internal->AttributeNames.size());
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetAttributeName(int attribute)
{
  if (attribute >= 0 && attribute < this->GetNumberOfAttributes())
  {
    return this->Internal->AttributeNames[attribute].c_str();
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetAttributeIndex(const char* name)
{
  if (name)
  {
    for (vtkXMLCollectionReaderAttributeNames::const_iterator i =
           this->Internal->AttributeNames.begin();
         i != this->Internal->AttributeNames.end(); ++i)
    {
      if (*i == name)
      {
        return static_cast<int>(i - this->Internal->AttributeNames.begin());
      }
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetNumberOfAttributeValues(int attribute)
{
  if (attribute >= 0 && attribute < this->GetNumberOfAttributes())
  {
    return static_cast<int>(this->Internal->AttributeValueSets[attribute].size());
  }
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetAttributeValue(int attribute, int index)
{
  if (index >= 0 && index < this->GetNumberOfAttributeValues(attribute))
  {
    return this->Internal->AttributeValueSets[attribute][index].c_str();
  }
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetAttributeValue(const char* name, int index)
{
  return this->GetAttributeValue(this->GetAttributeIndex(name), index);
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetAttributeValueIndex(int attribute, const char* value)
{
  if (attribute >= 0 && attribute < this->GetNumberOfAttributes() && value)
  {
    for (vtkXMLCollectionReaderAttributeValueSets::value_type::const_iterator i =
           this->Internal->AttributeValueSets[attribute].begin();
         i != this->Internal->AttributeValueSets[attribute].end(); ++i)
    {
      if (*i == value)
      {
        return static_cast<int>(i - this->Internal->AttributeValueSets[attribute].begin());
      }
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetAttributeValueIndex(const char* name, const char* value)
{
  return this->GetAttributeValueIndex(this->GetAttributeIndex(name), value);
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLCollectionReader::GetOutputXMLDataElement(int index)
{
  // We must call UpdateInformation to make sure the set of outputs is
  // up to date.
  this->UpdateInformation();

  // Make sure the index is in range.
  if (index < 0 || index >= static_cast<int>(this->Internal->RestrictedDataSets.size()))
  {
    vtkErrorMacro("Attempt to get XMLDataElement for output index "
      << index << " from a reader with " << this->Internal->RestrictedDataSets.size()
      << " outputs.");
    return 0;
  }
  return this->Internal->RestrictedDataSets[index];
}

//----------------------------------------------------------------------------
const vtkXMLCollectionReaderEntry vtkXMLCollectionReaderInternals::ReaderList[] = {
  { "vtp", "vtkXMLPolyDataReader" }, { "vtu", "vtkXMLUnstructuredGridReader" },
  { "vti", "vtkXMLImageDataReader" }, { "vtr", "vtkXMLRectilinearGridReader" },
  { "vtm", "vtkXMLMultiBlockDataReader" }, { "vtmb", "vtkXMLMultiBlockDataReader" },
  { "vtmg", "vtkXMLMultiGroupDataReader" },   // legacy reader - produces vtkMultiBlockDataSet.
  { "vthd", "vtkXMLHierarchicalDataReader" }, // legacy reader - produces vtkMultiBlockDataSet.
  { "vthb", "vtkXMLHierarchicalBoxDataReader" }, { "vts", "vtkXMLStructuredGridReader" },
  { "pvtp", "vtkXMLPPolyDataReader" }, { "pvtu", "vtkXMLPUnstructuredGridReader" },
  { "pvti", "vtkXMLPImageDataReader" }, { "pvtr", "vtkXMLPRectilinearGridReader" },
  { "pvts", "vtkXMLPStructuredGridReader" }, { 0, 0 }
};
