/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkXMLPPolyDataWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkSmartPointer.h"
#include "vtkXMLWriter.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

#if defined(_WIN32)
# include <direct.h>
int vtkPVAnimationWriterMakeDirectory(const char* dirname)
{
  return (_mkdir(dirname) >= 0)?1:0;
}
#else
# include <sys/stat.h>
# include <sys/types.h>
int vtkPVAnimationWriterMakeDirectory(const char* dirname)
{
  return (mkdir(dirname, 00755) >= 0)?1:0;
}
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAnimationWriter);
vtkCxxRevisionMacro(vtkPVAnimationWriter, "1.1.2.4");

//----------------------------------------------------------------------------

// Record an entry for every input at every animation step.
struct vtkPVAnimationWriterEntry
{
  double Time;
  int Part;
  vtkstd::string Group;
  vtkstd::string FileName;
};

class vtkPVAnimationWriterInternals
{
public:
  // A writer for each input.
  vtkstd::vector< vtkSmartPointer<vtkXMLWriter> > Writers;
  
  // The name of the group to which each input belongs.
  typedef vtkstd::vector<vtkstd::string> InputGroupNamesType;
  InputGroupNamesType InputGroupNames;
  
  // The part number each input has been assigned in its group.
  typedef vtkstd::vector<int> InputPartNumbersType;
  InputPartNumbersType InputPartNumbers;
  
  // The modified time when each input was last written in a previous
  // animation step.
  typedef vtkstd::vector<unsigned long> InputMTimesType;
  InputMTimesType InputMTimes;
  
  // The number of times each input has changed during this animation
  // sequence.
  typedef vtkstd::vector<int> InputChangeCountsType;
  InputChangeCountsType InputChangeCounts;
  
  // Count the number of parts in each group.
  typedef vtkstd::map<vtkstd::string, int> GroupMapType;
  GroupMapType GroupMap;
  
  // Build a list of entries during animation for writing to the
  // collection file at the end.
  typedef vtkstd::vector<vtkPVAnimationWriterEntry> EntriesType;
  EntriesType Entries;
  
  // The directory containing the output file.
  vtkstd::string FilePath;
  
  // The prefix name from the output file for addition to all internal
  // file names.
  vtkstd::string FilePrefix;
  
  // Create the file name for the given input during this animation
  // step.
  vtkstd::string CreateFileName(int index, const char* path);
};

//----------------------------------------------------------------------------
vtkPVAnimationWriter::vtkPVAnimationWriter()
{
  this->Internal = new vtkPVAnimationWriterInternals;
  this->StartCalled = 0;
  this->FinishCalled = 0;
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->WriteAnimationFileInitialized = 0;
  this->WriteAnimationFile = 0;

}

//----------------------------------------------------------------------------
vtkPVAnimationWriter::~vtkPVAnimationWriter()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "Piece: " << this->Piece << endl;
  
  if(this->GetNumberOfInputs() > 0)
    {
    os << indent << "Input Detail:\n";
    vtkIndent nextIndent = indent.GetNextIndent();
    int i;
    for(i=0; i < this->GetNumberOfInputs(); ++i)
      {
      os << nextIndent << i << ": group \""
         << this->Internal->InputGroupNames[i].c_str()
         << "\" part " << this->Internal->InputPartNumbers[i]
         << "\n";
      }
    }
}

//----------------------------------------------------------------------------
const char* vtkPVAnimationWriter::GetDefaultFileExtension()
{
  return "pvd";
}

//----------------------------------------------------------------------------
const char* vtkPVAnimationWriter::GetDataSetName()
{
  return "Collection";
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::SetWriteAnimationFile(int flag)
{
  this->WriteAnimationFileInitialized = 1;
  vtkDebugMacro(<< this->GetClassName() << " ("
                << this << "): setting WriteAnimationFile to " << flag);
  if(this->WriteAnimationFile != flag)
    {
    this->WriteAnimationFile = flag;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::AddInput(vtkPolyData* pd, const char* group)
{
  // Add the input to the pipeline structure.
  this->vtkProcessObject::AddInput(pd);
  
  // Find the part number for this input.
  int partNum = 0;
  vtkPVAnimationWriterInternals::GroupMapType::iterator s =
    this->Internal->GroupMap.find(group);
  if(s != this->Internal->GroupMap.end())
    {
    partNum = s->second++;
    }
  else
    {
    vtkPVAnimationWriterInternals::GroupMapType::value_type v(group, 1);
    this->Internal->GroupMap.insert(v);
    }
  this->Internal->InputPartNumbers.push_back(partNum);
  
  // Add the group name for this input.
  this->Internal->InputGroupNames.push_back(group);
  
  // Allocate the mtime table entry for this input.
  this->Internal->InputMTimes.push_back(0);
  
  // Allocate the change count entry for this input.
  this->Internal->InputChangeCounts.push_back(0);
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::AddInput(vtkDataObject* d)
{
  vtkPolyData* pd = vtkPolyData::SafeDownCast(d);
  if(pd)
    {
    this->AddInput(pd, "");
    }
  else
    {
    vtkWarningMacro("Attempt to add NULL or non vtkPolyData input.");
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::Start()
{
  // Do not allow double-start.
  if(this->StartCalled)
    {
    vtkErrorMacro("Cannot call Start() twice before calling Finish().");
    return;
    }
  
  // Make sure we have a file name.
  if(!this->FileName || !this->FileName[0])
    {
    vtkErrorMacro("No FileName has been set.");
    return;
    }
  
  // Initialize input change tables.
  int i;
  for(i=0; i < this->GetNumberOfInputs(); ++i)
    {
    this->Internal->InputMTimes[i] = 0;
    this->Internal->InputChangeCounts[i] = 0;
    }
  
  // Clear the animation entries from any previous run.
  this->Internal->Entries.clear();
  
  // Split the file name into a directory and file prefix.
  this->SplitFileName();
  
  // Create a writer for each input.
  this->CreateWriters();
  
  // Create the subdirectory for the internal files.
  vtkstd::string subdir = this->Internal->FilePath;
  subdir += this->Internal->FilePrefix;
  vtkPVAnimationWriterMakeDirectory(subdir.c_str());
  
  this->StartCalled = 1;
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::WriteTime(double time)
{
  if(!this->StartCalled)
    {
    vtkErrorMacro("Must call Start() before WriteTime().");
    return;
    }
  
  // Consider every input.
  int i;
  for(i=0; i < this->GetNumberOfInputs(); ++i)
    {
    // Make sure the pipeline mtime is up to date.
    this->Inputs[i]->UpdateInformation();
    
    // If the input has been modified since the last animation step,
    // increment its file number.
    int changed = 0;
    if(this->Inputs[i]->GetPipelineMTime() > this->Internal->InputMTimes[i])
      {
      this->Internal->InputMTimes[i] = this->Inputs[i]->GetPipelineMTime();
      this->Internal->InputChangeCounts[i] += 1;
      changed = 1;
      }
    
    // Create this animation entry.
    vtkPVAnimationWriterEntry entry;
    entry.Time = time;
    entry.Part = this->Internal->InputPartNumbers[i];
    entry.Group = this->Internal->InputGroupNames[i];    
    entry.FileName = this->Internal->CreateFileName(i, 0);
    vtkstd::string fullName =
      this->Internal->CreateFileName(i, this->Internal->FilePath.c_str());
    
    // Write this animation entry if its input has changed.
    if(changed)
      {
      this->Internal->Writers[i]->SetFileName(fullName.c_str());
      this->Internal->Writers[i]->Write();
      }
    
    // Add an entry for this animation step.
    this->Internal->Entries.push_back(entry);
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::Finish()
{
  if(!this->StartCalled)
    {
    vtkErrorMacro("Must call Start() before Finish().");
    return;
    }
  
  this->StartCalled = 0;
  this->FinishCalled = 1;
  
  // Just write the output file with the current set of entries.
  this->Write();
}

//----------------------------------------------------------------------------
int vtkPVAnimationWriter::WriteInternal()
{
  if(!this->FinishCalled)
    {
    vtkErrorMacro("Do not call Write() directly.  Call Finish() instead.");
    return 0;
    }
  
  this->FinishCalled = 0;
  
  // Decide whether to write the animation file.
  int writeAnimation = 0;
  if(this->WriteAnimationFileInitialized)
    {
    writeAnimation = this->WriteAnimationFile;
    }
  else if(this->Piece == 0)
    {
    writeAnimation = 1;
    }
  
  if(writeAnimation)
    {
    return this->Superclass::WriteInternal();
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVAnimationWriter::WriteData()
{
  // Write the collection file.
  this->StartFile();
  vtkIndent indent = vtkIndent().GetNextIndent();
  
  // Open the primary element.
  ostream& os = *(this->Stream);
  os << indent << "<" << this->GetDataSetName() << ">\n";
  
  // Write a DataSet entry for every input at every time step.
  for(vtkPVAnimationWriterInternals::EntriesType::iterator i =
        this->Internal->Entries.begin();
      i != this->Internal->Entries.end(); ++i)
    {
    os << indent.GetNextIndent()
       << "<DataSet timestep=\"" << i->Time
       << "\" group=\"" << i->Group.c_str()
       << "\" part=\"" << i->Part
       << "\" file=\"" << i->FileName.c_str()
       << "\"/>\n";
    }  
  
  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";
  this->EndFile();
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::SplitFileName()
{
  vtkstd::string fileName = this->FileName;
  vtkstd::string name;
  
  // Split the file name and extension from the path.
  vtkstd::string::size_type pos = fileName.find_last_of("/\\");
  if(pos != fileName.npos)
    {
    // Keep the slash in the file path.
    this->Internal->FilePath = fileName.substr(0, pos+1);
    name = fileName.substr(pos+1);
    }
  else
    {
    this->Internal->FilePath = "./";
    name = fileName;
    }
  
  // Split the extension from the file name.
  pos = name.find_last_of(".");
  if(pos != name.npos)
    {
    this->Internal->FilePrefix = name.substr(0, pos);
    }
  else
    {
    this->Internal->FilePrefix = name;
    
    // Since a subdirectory is used to store the files, we need to
    // change its name if there is no file extension.
    this->Internal->FilePrefix += "_data";
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationWriter::CreateWriters()
{
  int i;
  this->Internal->Writers.resize(this->GetNumberOfInputs());
  for(i=0; i < this->GetNumberOfInputs(); ++i)
    {
    if(this->NumberOfPieces > 1)
      {
      if(!this->Internal->Writers[i].GetPointer() ||
         (strcmp(this->Internal->Writers[i]->GetClassName(),
                 "vtkXMLPPolyDataWriter") != 0))
        {
        vtkXMLPPolyDataWriter* w = vtkXMLPPolyDataWriter::New();
        this->Internal->Writers[i] = w;
        w->Delete();
        }
      vtkXMLPPolyDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
        ->SetInput(vtkPolyData::SafeDownCast(this->Inputs[i]));
      }
    else
      {
      if(!this->Internal->Writers[i].GetPointer() ||
         (strcmp(this->Internal->Writers[i]->GetClassName(),
                 "vtkXMLPolyDataWriter") != 0))
        {
        vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
        this->Internal->Writers[i] = w;
        w->Delete();
        }
      vtkXMLPolyDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
        ->SetInput(vtkPolyData::SafeDownCast(this->Inputs[i]));
      }
    
    // Copy settings to the writer.
    if(vtkXMLWriter* w = this->Internal->Writers[i].GetPointer())
      {
      w->SetDebug(this->GetDebug());
      w->SetByteOrder(this->GetByteOrder());
      w->SetCompressor(this->GetCompressor());
      w->SetBlockSize(this->GetBlockSize());
      w->SetDataMode(this->GetDataMode());
      w->SetEncodeAppendedData(this->GetEncodeAppendedData());
      }
    
    // If this is a parallel writer, set the piece information.
    if(vtkXMLPDataWriter* w = vtkXMLPDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer()))
      {
      w->SetStartPiece(this->Piece);
      w->SetEndPiece(this->Piece);
      w->SetNumberOfPieces(this->NumberOfPieces);
      w->SetGhostLevel(0);
      }
    }
}

//----------------------------------------------------------------------------
vtkstd::string vtkPVAnimationWriterInternals::CreateFileName(int index,
                                                             const char* path)
{ 
  // Start with the directory and file name prefix.
  ostrstream fn_with_warning_C4701;  
  if(path)
    {
    fn_with_warning_C4701 << path;
    }
  fn_with_warning_C4701
    << this->FilePrefix.c_str() << "/"
    << this->FilePrefix.c_str() << "_";
  
  // Add the group name.
  fn_with_warning_C4701 << this->InputGroupNames[index].c_str();
  
  // Construct the part/time portion.  Add a part number if there is
  // more than one part in this group.
  char pt[100];  
  if(this->GroupMap[this->InputGroupNames[index]] > 1)
    {
    sprintf(pt, "P%02dT%04d",
            this->InputPartNumbers[index],
            this->InputChangeCounts[index]);
    }
  else
    {
    sprintf(pt, "T%04d", this->InputChangeCounts[index]);
    }
  fn_with_warning_C4701 << pt;
  
  // Add the file extension.
  fn_with_warning_C4701
    << "." << this->Writers[index]->GetDefaultFileExtension() << ends;
  
  // Return the result.
  vtkstd::string fname = fn_with_warning_C4701.str();
  fn_with_warning_C4701.rdbuf()->freeze(0);
  return fname;
}
