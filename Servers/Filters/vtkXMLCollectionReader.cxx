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
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtkstd/map>
#include <vtkstd/algorithm>

vtkCxxRevisionMacro(vtkXMLCollectionReader, "1.11");
vtkStandardNewMacro(vtkXMLCollectionReader);

//----------------------------------------------------------------------------

struct vtkXMLCollectionReaderEntry
{
  const char* extension;
  const char* name;
};

class vtkXMLCollectionReaderString: public vtkstd::string
{
public:
  typedef vtkstd::string Superclass;
  vtkXMLCollectionReaderString(): Superclass() {}
  vtkXMLCollectionReaderString(const char* s): Superclass(s) {}
  vtkXMLCollectionReaderString(const Superclass& s): Superclass(s) {}
};
typedef vtkstd::vector<vtkXMLCollectionReaderString>
        vtkXMLCollectionReaderAttributeNames;
typedef vtkstd::vector<vtkstd::vector<vtkXMLCollectionReaderString> >
        vtkXMLCollectionReaderAttributeValueSets;
typedef vtkstd::map<vtkXMLCollectionReaderString, vtkXMLCollectionReaderString>
        vtkXMLCollectionReaderRestrictions;
class vtkXMLCollectionReaderInternals
{
public:
  vtkstd::vector<vtkXMLDataElement*> DataSets;
  vtkstd::vector<vtkXMLDataElement*> RestrictedDataSets;
  vtkXMLCollectionReaderAttributeNames AttributeNames;
  vtkXMLCollectionReaderAttributeValueSets AttributeValueSets;
  vtkXMLCollectionReaderRestrictions Restrictions;
  vtkstd::vector< vtkSmartPointer<vtkXMLReader> > Readers;
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
int vtkXMLCollectionReader::GetNumberOfOutputs()
{
  // We must call UpdateInformation to make sure the set of outputs is
  // up to date.
  this->UpdateInformation();
  
  // Now return the requested number.
  return this->Superclass::GetNumberOfOutputPorts();
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLCollectionReader::GetOutput(int index)
{
  // We must call UpdateInformation to make sure the set of outputs is
  // up to date.
  this->UpdateInformation();
  
  // Now return the requested output.
  return this->GetOutputAsDataSet(index);
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::Update()
{
  // Update information first to make sure an output exists.
  this->UpdateInformation();
  
  // Now complete the standard Update.
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::MarkGeneratedOutputs(vtkDataObject* output)
{
  if(output)
    {
    output->DataHasBeenGenerated();
    }
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::SetRestriction(const char* name,
                                            const char* value)
{
  vtkXMLCollectionReaderRestrictions::iterator i =
    this->Internal->Restrictions.find(name);
  if(value && value[0])
    {
    // We have a value.  Set the restriction.
    if(i != this->Internal->Restrictions.end())
      {
      if(i->second != value)
        {
        // Replace the existing restriction value on this attribute.
        i->second = value;
        this->Modified();
        }
      }
    else
      {
      // Add the restriction on this attribute.
      this->Internal->Restrictions.insert(
        vtkXMLCollectionReaderRestrictions::value_type(name, value));
      this->Modified();
      }
    }
  else if(i != this->Internal->Restrictions.end())
    {
    // The value is empty.  Remove the restriction.
    this->Internal->Restrictions.erase(i);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetRestriction(const char* name)
{
  vtkXMLCollectionReaderRestrictions::const_iterator i =
    this->Internal->Restrictions.find(name);
  if(i != this->Internal->Restrictions.end())
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
  this->UpdateInformation();
  this->SetRestriction(name, this->GetAttributeValue(name, index));
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetRestrictionAsIndex(const char* name)
{
  this->UpdateInformation();
  return this->GetAttributeValueIndex(name, this->GetRestriction(name));
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::InternalProgressCallbackFunction(vtkObject*,
                                                              unsigned long,
                                                              void* clientdata,
                                                              void*)
{
  reinterpret_cast<vtkXMLCollectionReader*>(clientdata)
    ->InternalProgressCallback();
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::InternalProgressCallback()
{
  float width = this->ProgressRange[1]-this->ProgressRange[0];
  vtkXMLReader* reader =
    this->Internal->Readers[this->CurrentOutput].GetPointer();
  float dataProgress = reader->GetProgress();
  float progress = this->ProgressRange[0] + dataProgress*width;
  this->UpdateProgressDiscrete(progress);
  if(this->AbortExecute)
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
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }
  
  // Count the number of data sets in the file.
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numDataSets = 0;
  int i;
  for(i=0; i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "DataSet") == 0) { ++numDataSets; }
    }
  
  // Now read each data set.  Build the list of data sets, their
  // attributes, and the attribute values.
  this->Internal->AttributeNames.clear();
  this->Internal->AttributeValueSets.clear();
  this->Internal->DataSets.clear();
  for(i=0;i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "DataSet") == 0)
      {
      int j;
      this->Internal->DataSets.push_back(eNested);
      for(j=0; j < eNested->GetNumberOfAttributes(); ++j)
        {
        this->AddAttributeNameValue(eNested->GetAttributeName(j),
                                    eNested->GetAttributeValue(j));
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::SetupEmptyOutput()
{
  this->SetNumberOfOutputPorts(0);
}



int vtkXMLCollectionReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}


//----------------------------------------------------------------------------
int vtkXMLCollectionReader::RequestDataObject(
  vtkInformation *request, 
  vtkInformationVector **inputVector, 
  vtkInformationVector *outputVector)
{
  // need to Parse the file first
  if (!this->ReadXMLInformation())
    {
    return 0;
    }

  // Build list of data sets fitting the restrictions.
  this->Internal->RestrictedDataSets.clear();
  vtkstd::vector<vtkXMLDataElement*>::iterator d;
  for(d=this->Internal->DataSets.begin();
      d != this->Internal->DataSets.end(); ++d)
    {
    vtkXMLDataElement* ds = *d;
    int matches = ds->GetAttribute("file")?1:0;
    vtkXMLCollectionReaderRestrictions::const_iterator r;
    for(r=this->Internal->Restrictions.begin();
        matches && r != this->Internal->Restrictions.end(); ++r)
      {
      const char* value = ds->GetAttribute(r->first.c_str());
      if(!(value && value == r->second))
        {
        matches = 0;
        }
      }
    if(matches)
      {
      this->Internal->RestrictedDataSets.push_back(ds);
      }
    }
  
  // Find the path to this file in case the internal files are
  // specified as relative paths.
  vtkstd::string filePath = this->FileName;
  vtkstd::string::size_type pos = filePath.find_last_of("/\\");
  if(pos != filePath.npos)
    {
    filePath = filePath.substr(0, pos);
    }
  else
    {
    filePath = "";
    }
  
  // Create the readers for each data set to be read.
  int i;
  int n = static_cast<int>(this->Internal->RestrictedDataSets.size());
  vtkDebugMacro("Setting number of outputs to " << n << ".");
  this->SetNumberOfOutputPorts(n);
  this->Internal->Readers.resize(n);
  for(i=0; i < n; ++i)
    {
    this->SetupOutput(filePath.c_str(), i, 
      outputVector->GetInformationObject(i));
    }  

  return 1;
}


//----------------------------------------------------------------------------
void vtkXMLCollectionReader::SetupOutput(const char* filePath, int index,
                                         vtkInformation* outInfo)
{
  vtkXMLDataElement* ds = this->Internal->RestrictedDataSets[index];

  // Construct the name of the internal file.
  vtkstd::string fileName;
  const char* file = ds->GetAttribute("file");
  if(!(file[0] == '/' || file[1] == ':'))
    {
    fileName = filePath;
    if(fileName.length())
      {
      fileName += "/";
      }
    }
  fileName += file;
  
  // Get the file extension.
  vtkstd::string ext;
  vtkstd::string::size_type pos = fileName.rfind('.');
  if(pos != fileName.npos)
    {
    ext = fileName.substr(pos+1);
    }
  
  // Search for the reader matching this extension.
  const char* rname = 0;
  for(const vtkXMLCollectionReaderEntry* r = this->Internal->ReaderList;
      !rname && r->extension; ++r)
    {
    if(ext == r->extension)
      {
      rname = r->name;
      }
    }
  
  // If a reader was found, allocate an instance of it for this
  // output.
  if(rname)
    {
    if(!(this->Internal->Readers[index].GetPointer() &&
         strcmp(this->Internal->Readers[index]->GetClassName(), rname) == 0))
      {
      // Use the instantiator to create the reader.
      vtkObject* o = vtkInstantiator::CreateInstance(rname);
      vtkXMLReader* reader = vtkXMLReader::SafeDownCast(o);
      this->Internal->Readers[index] = reader;
      if(reader)
        {
        reader->Delete();
        }
      else
        {
        // The class was not registered with the instantiator.
        vtkErrorMacro("Error creating \"" << rname
                      << "\" using vtkInstantiator.");
        if(o)
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
  if(this->Internal->Readers[index].GetPointer())
    {
    // Assign the file name to the internal reader.
    this->Internal->Readers[index]->SetFileName(fileName.c_str());
    
    // Update the information on the internal reader's output.
    this->Internal->Readers[index]->UpdateInformation();    
    
    // Allocate an instance of the same output type for our output.
    vtkDataSet* out = this->Internal->Readers[index]->GetOutputAsDataSet();
    vtkDataSet *currentOutput = vtkDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    if(!(currentOutput && strcmp(currentOutput->GetClassName(),
      out->GetClassName()) == 0))
      {
      // Need to create an instance.  This reader is its source.
      vtkDataObject* newOut = out->NewInstance();
      newOut->SetPipelineInformation(outInfo);
      outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), newOut->GetExtentType());
      newOut->Delete();
      }

    // Share the data between the internal reader's output and our
    // output.
    this->GetExecutive()->GetOutputData(index)->ShallowCopy(out);
    }
  else
    {
    // We do not have a reader for this output, remove it.
    this->GetExecutive()->SetOutputData(index, 0);
    }
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::ReadXMLData()
{
  // If we have a reader for the current output, use it.
  vtkXMLReader* r = this->Internal->Readers[this->CurrentOutput].GetPointer();
  if(r)
    {
    // Observe the progress of the internal reader.
    r->AddObserver(vtkCommand::ProgressEvent, this->InternalProgressObserver);
    vtkDataSet* out = r->GetOutputAsDataSet();
    
    // Give the update request from this output to its internal
    // reader.
    if(out->GetExtentType() == VTK_PIECES_EXTENT)
      {
      int p = this->GetExecutive()->GetOutputData(this->CurrentOutput)->GetUpdatePiece();
      int n = this->GetExecutive()->GetOutputData(this->CurrentOutput)->GetUpdateNumberOfPieces();
      int g = this->GetExecutive()->GetOutputData(this->CurrentOutput)->GetUpdateGhostLevel();
      out->SetUpdateExtent(p, n, g);
      }
    else
      {
      int e[6];
      this->GetExecutive()->GetOutputData(this->CurrentOutput)->GetUpdateExtent(e);
      out->SetUpdateExtent(e);
      }
    
    // Read the data.
    out->Update();
    
    // The internal reader is finished.  Remove the observer in case
    // we delete the reader later.
    r->RemoveObserver(this->InternalProgressObserver);
    
    // Share the new data with our output.
    this->GetExecutive()->GetOutputData(this->CurrentOutput)->ShallowCopy(out);

    // If a "name" attribute exists, store the name of the output in
    // its field data.
    vtkXMLDataElement* ds =
      this->Internal->RestrictedDataSets[this->CurrentOutput];
    const char* name = ds? ds->GetAttribute("name") : 0;
    if(name)
      {
      vtkCharArray* nmArray = vtkCharArray::New();
      nmArray->SetName("Name");
      size_t len = strlen(name);
      nmArray->SetNumberOfTuples(static_cast<vtkIdType>(len)+1);
      char* copy = nmArray->GetPointer(0);
      memcpy(copy, name, len);
      copy[len] = '\0';
      this->GetExecutive()->GetOutputData(this->CurrentOutput)->GetFieldData()->AddArray(nmArray);
      nmArray->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLCollectionReader::AddAttributeNameValue(const char* name,
                                                   const char* value)
{
  vtkXMLCollectionReaderString s = name;
  
  // Find an entry for this attribute.
  vtkXMLCollectionReaderAttributeNames::iterator n =
    vtkstd::find(this->Internal->AttributeNames.begin(),
                 this->Internal->AttributeNames.end(), name);
  vtkstd::vector<vtkXMLCollectionReaderString>* values = 0;
  if(n == this->Internal->AttributeNames.end())
    {
    // Need to create an entry for this attribute.
    this->Internal->AttributeNames.push_back(name);
    
    this->Internal->AttributeValueSets.resize(
      this->Internal->AttributeValueSets.size()+1);
    values = &*(this->Internal->AttributeValueSets.end()-1);
    }
  else
    {
    values = &*(this->Internal->AttributeValueSets.begin() +
                (n-this->Internal->AttributeNames.begin()));
    }
  
  // Find an entry within the attribute for this value.
  s = value;
  vtkstd::vector<vtkXMLCollectionReaderString>::iterator i =
    vtkstd::find(values->begin(), values->end(), s);
  
  if(i == values->end())
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
  if(attribute >= 0 && attribute < this->GetNumberOfAttributes())
    {
    return this->Internal->AttributeNames[attribute].c_str();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetAttributeIndex(const char* name)
{
  if(name)
    {
    for(vtkXMLCollectionReaderAttributeNames::const_iterator i =
          this->Internal->AttributeNames.begin();
        i != this->Internal->AttributeNames.end(); ++i)
      {
      if(*i == name)
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
  if(attribute >= 0 && attribute < this->GetNumberOfAttributes())
    {
    return static_cast<int>(this->Internal->AttributeValueSets[attribute].size());
    }
  return 0;  
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetAttributeValue(int attribute, int index)
{
  if(index >= 0 && index < this->GetNumberOfAttributeValues(attribute))
    {
    return this->Internal->AttributeValueSets[attribute][index].c_str();
    }
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkXMLCollectionReader::GetAttributeValue(const char* name,
                                                      int index)
{
  return this->GetAttributeValue(this->GetAttributeIndex(name), index);
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetAttributeValueIndex(int attribute,
                                                   const char* value)
{
  if(attribute >= 0 && attribute < this->GetNumberOfAttributes() && value)
    {
    for(vtkXMLCollectionReaderAttributeValueSets::value_type::const_iterator i
          = this->Internal->AttributeValueSets[attribute].begin();
        i != this->Internal->AttributeValueSets[attribute].end(); ++i)
      {
      if(*i == value)
        {
        return static_cast<int>(i - this->Internal->AttributeValueSets[attribute].begin());
        }
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkXMLCollectionReader::GetAttributeValueIndex(const char* name,
                                                   const char* value)
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
  if(index < 0 ||
     index >= static_cast<int>(this->Internal->RestrictedDataSets.size()))
    {
    vtkErrorMacro("Attempt to get XMLDataElement for output index "
                  << index << " from a reader with "
                  << this->Internal->RestrictedDataSets.size()
                  << " outputs.");
    return 0;
    }
  return this->Internal->RestrictedDataSets[index];
}

//----------------------------------------------------------------------------
const vtkXMLCollectionReaderEntry
vtkXMLCollectionReaderInternals::ReaderList[] =
{
  {"vtp", "vtkXMLPolyDataReader"},
  {"vtu", "vtkXMLUnstructuredGridReader"},
  {"vti", "vtkXMLImageDataReader"},
  {"vtr", "vtkXMLRectilinearGridReader"},
  {"vts", "vtkXMLStructuredGridReader"},
  {"pvtp", "vtkXMLPPolyDataReader"},
  {"pvtu", "vtkXMLPUnstructuredGridReader"},
  {"pvti", "vtkXMLPImageDataReader"},
  {"pvtr", "vtkXMLPRectilinearGridReader"},
  {"pvts", "vtkXMLPStructuredGridReader"},
  {0, 0}
};
