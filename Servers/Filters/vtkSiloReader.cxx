/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSiloReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

 Note: Parts of the code found in this file are verbatim VisIt source code
 from the VisIt source files avtSiloFileFormat.C and avtSiloFileFormat.h.
 The authors of these files include:

    Kathleen Bonnell
    Eric Brugger
    Hank Childs
    Cyrus Harrison
    Jeremy Meredith
    Mark C. Miller

 The following copyright notice is included with avtSiloFileFormat...

=========================================================================*/

/*****************************************************************************
*
* Copyright (c) 2000 - 2007, The Regents of the University of California
* Produced at the Lawrence Livermore National Laboratory
* All rights reserved.
*
* This file is part of VisIt. For details, see http://www.llnl.gov/visit/. The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or materials provided with the distribution.
*  - Neither the name of the UC/LLNL nor  the names of its contributors may be
*    used to  endorse or  promote products derived from  this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
* CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
* ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/


#include "vtkSiloReader.h"
#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtksys/ios/sstream>
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkErrorCode.h"
#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"
#include "vtkShortArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkMultiProcessController.h"
#include "vtkInformationIntegerKey.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSiloReader);

//----------------------------------------------------------------------------

struct SiloMeshBlock
{
  int Domain; // which domain the mesh belongs to, i.e., which processor should handle it (piece number)
  int Type;
  vtkstd::string FileName;
  vtkstd::string Name;
  vtkstd::vector<vtkstd::string> Variables;
  vtkstd::vector<vtkstd::string> Materials;
};

//----------------------------------------------------------------------------

struct SiloMesh
{
  vtkstd::vector< SiloMeshBlock* > blocks;
  vtkstd::string Name;
};

//----------------------------------------------------------------------------

class vtkSiloTableOfContents : public vtkObject
{

public:

  static vtkSiloTableOfContents *New();

  SiloMesh * AddMesh(const char*);
  SiloMesh * GetMesh(const char*);
  SiloMeshBlock * AddMeshBlock(const char*, SiloMesh*);
  SiloMeshBlock * GetMeshBlock(const char*);
  vtkXMLDataElement * ToXML();
  void FromXML(vtkXMLDataElement *);
  const char * GetTOCString();
  void Clear();
  int AddFile(const char *);
  DBfile *GetFile(int);
  DBfile *OpenFile(int);
  DBfile *OpenFile(const char *);
  void CloseFile(int);
  void CloseAllFiles();
  int DetermineFileIndex(DBfile *);
  int DetermineFileNameIndex(const char *);

  vtkstd::vector< vtkstd::string > Filenames;
  vtkstd::vector< DBfile * > DBFiles;
  vtkstd::map< vtkstd::string, SiloMesh*> Meshes;
  vtkstd::map< vtkstd::string, SiloMeshBlock*> MeshBlocks;
  vtkstd::set< vtkstd::string > DomainDirs;
  // TOCPath is the path to the directory containing TOCFile
  vtkstd::string TOCPath;
  vtkstd::string TOCFile;
  vtkstd::string TOCString;

protected:

  vtkSiloTableOfContents();
  ~vtkSiloTableOfContents();

private:

  vtkSiloTableOfContents(const vtkSiloTableOfContents&);  // Not implemented.
  void operator=(const vtkSiloTableOfContents&);  // Not implemented.

};

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSiloTableOfContents);

//----------------------------------------------------------------------------

typedef vtkstd::map< vtkstd::string, SiloMesh*>::const_iterator ConstMeshIter;
typedef vtkstd::map< vtkstd::string, SiloMeshBlock*>::const_iterator ConstMeshBlockMapIter;
typedef vtkstd::map< vtkstd::string, SiloMesh*>::iterator MeshIter;
typedef vtkstd::map< vtkstd::string, SiloMeshBlock*>::iterator MeshBlockMapIter;
typedef vtkstd::vector<SiloMeshBlock*>::iterator MeshBlockIter;
typedef vtkstd::vector< vtkstd::string >::const_iterator StringListIter;

//----------------------------------------------------------------------------

// This class will pull data from Silo files and return vtkDataSets and vtkDataArrays
class vtkSiloReaderHelper
{
public:
  vtkSiloReaderHelper() { }
  ~vtkSiloReaderHelper() { }

private:

  vtkSiloReaderHelper(const vtkSiloReaderHelper&);  // Not implemented.
  void operator=(const vtkSiloReaderHelper&);  // Not implemented.

};

//----------------------------------------------------------------------------

// A basic class to expose the protected vtkXML::Parse method.
class vtkSiloReaderXMLParser: public vtkXMLDataParser
{
public:
  static vtkSiloReaderXMLParser *New();

  virtual int Parse(const char* string, unsigned int len)
  {
    return this->vtkXMLParser::Parse(string, len);
  }
protected:
  vtkSiloReaderXMLParser() {};
  ~vtkSiloReaderXMLParser() {};

private:
  vtkSiloReaderXMLParser(const vtkSiloReaderXMLParser&);  // Not implemented.
  void operator=(const vtkSiloReaderXMLParser&);  // Not implemented.
};
vtkStandardNewMacro(vtkSiloReaderXMLParser);

//----------------------------------------------------------------------------

// Silo doesn't seem to have a consistent
// way of handling centering information.
// The silo representation depends on mesh
// type.  So we'll make our own enum.
enum SiloCenteringType {
  NODE_CENT = 0,
  CELL_CENT,
};

//----------------------------------------------------------------------------

// Utility functions
vtkstd::string PrepareDirName(const char *, const char *);
inline char *CXX_strdup(char const * const);
static char *GenerateName(const char *, const char *);
int SiloZoneTypeToVTKZoneType(int);
void TranslateSiloWedgeToVTKWedge(const int *, vtkIdType [6]);
void TranslateSiloPyramidToVTKPyramid(const int *, vtkIdType [5]);
void TranslateSiloTetrahedronToVTKTetrahedron(const int *, vtkIdType [4]);
bool TetsAreInverted(const int *, vtkUnstructuredGrid *);
// PrintErrorMessage is a callback used by the
// the Silo library to report errors.
// TODO: Is there a better way to implement this function?
static void PrintErrorMessage(char *);

//----------------------------------------------------------------------------

// Flag to indicate if we have previously made global silo calls
bool vtkSiloReader::MadeGlobalSiloCalls = false;

//----------------------------------------------------------------------------

vtkSiloReader::vtkSiloReader()
{
  vtkDebugMacro("Constructor");

  this->SetNumberOfInputPorts(0);
  this->FileName = 0;
  this->TableOfContentsRead = false;
  this->TableOfContentsFileIndex = 0;
  this->Helper = new vtkSiloReaderHelper;
  this->TOC = vtkSiloTableOfContents::New();
  this->Controller = vtkMultiProcessController::GetGlobalController();

  if (this->Controller)
    {
    this->ProcessId = this->Controller->GetLocalProcessId();
    this->NumProcesses = this->Controller->GetNumberOfProcesses();
    vtkDebugMacro("I am process " << this->ProcessId
                  << " out of " << this->NumProcesses);
    }
  else
    {
    this->ProcessId = 0;
    this->NumProcesses = 1;
    }

  if (!vtkSiloReader::MadeGlobalSiloCalls)
    {

    // Visit comment- this is primarily used for testing.
    // Take no chances with precision errors.
    DBForceSingle(1);

    // Tell Silo library to report all errors
    // Second argument is a pointer to an error reporting function
    // If null, Silo will print errors to std error stream.
    DBShowErrors(DB_ALL, PrintErrorMessage);

    // Turn on silo's checksumming feature. This is harmless if the
    // underlying file DOES NOT contain checksums and will cause only
    // a small performance hit if it does.
    DBSetEnableChecksums(1);

    // Set flag indicating that we have made global silo calls.
    vtkSiloReader::MadeGlobalSiloCalls = true;
    }
}

//----------------------------------------------------------------------------

vtkSiloReader::~vtkSiloReader()
{
  delete this->Helper;
  this->TOC->Delete();
}

//----------------------------------------------------------------------------

int vtkSiloReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro(<<"Request Info");

  // Make sure filename has been set
  if(!this->FileName)
    {
    vtkErrorMacro("Filename has to be specified!");
    return 0;
    }

  // Set number of pieces
  // Claim we can produce as many pieces as desired
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  int result = 1;
  if( !this->TableOfContentsRead )
    {
    if (this->ProcessId == 0)
      {
      // If we are process 0 then read the table of contents
      // and sotre the result.  Even if there was an error
      // and result is 0, we still have to call BroadcastTableOfContents
      // because the other processes are expecting it.
      result = this->ReadTableOfContents();
      }
    if (this->NumProcesses > 1)
      {
      //TODO
      // Should we broadcast the value of result, so that if node 0
      // returns 0 for RequestInformation, every node returns 0?
      this->BroadcastTableOfContents();
      }
    }

  return result;
}

//----------------------------------------------------------------------------

int vtkSiloReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro(<<"Request Data");

  int piece, numPieces;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkDebugMacro("Piece number: " << piece);
  vtkDebugMacro("Number of pieces: " << numPieces);

  // Sanity check, make sure table of contents has been read.
  if(!this->TableOfContentsRead)
    {
    vtkErrorMacro("RequestedData called before reading table of contents!");
    return 0;
    }

  // Get the vtkMultiBlockDataSet from outputVector
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

  //Copy data from Silo files to vtkMultiBlockDataSet
  this->CreateDataSet(output);

  return 1;
}

//----------------------------------------------------------------------------

void vtkSiloReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDebugMacro(<<"Print Self");
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  return;
}

//----------------------------------------------------------------------------

int vtkSiloReader::ReadTableOfContents()
{
  vtkDebugMacro("ReadTableOfContents");

  // Open the table of contents file
  if (!this->TOC->OpenFile(this->TOC->TOCFile.c_str()))
    {
    return 0;
    }

  // Set data read mask so that only metadata will be read and not
  // all data contained in meshes.
  DBSetDataReadMask(DBMatMatnames|DBMatMatnos|DBMatMatcolors);

  // Call ReadDir to parse all metadata in table of contents file
  this->ReadDir(this->TOC->OpenFile(this->TableOfContentsFileIndex), "/");

  // Reset data read mask.
  DBSetDataReadMask(DBAll);

  // Set TableOfContentsRead flag.
  this->TableOfContentsRead = true;
  return 1;
}

//----------------------------------------------------------------------------
/// Returns an XML string representation of the table of contents
const char * vtkSiloReader::GetTOCString()
{
  return this->TOC->GetTOCString();
}

//----------------------------------------------------------------------------

vtkSiloTableOfContents::vtkSiloTableOfContents()
{
}

//----------------------------------------------------------------------------

vtkSiloTableOfContents::~vtkSiloTableOfContents()
{
  this->Clear();
  this->CloseAllFiles();
}

//----------------------------------------------------------------------------
/// Returns an XML string represention of the table of contents
const char * vtkSiloTableOfContents::GetTOCString()
{
  if (this->TOCString == "")
    {
    // TOCString hasn't been generated yet, ToXML will generate it.
    // ToXML() returns an XMLElement, which we then delete.
    this->ToXML()->Delete();
    }
  return this->TOCString.c_str();
}

//----------------------------------------------------------------------------

void vtkSiloTableOfContents::Clear()
{
  MeshBlockMapIter itr = this->MeshBlocks.begin();
  for ( ; itr != this->MeshBlocks.end(); ++itr)
    {
    delete itr->second;
    }
  this->MeshBlocks.clear();

  MeshIter meshItr = this->Meshes.begin();
  for ( ; meshItr != this->Meshes.end(); ++meshItr)
    {
    delete meshItr->second;
    }
  this->Meshes.clear();
}

//----------------------------------------------------------------------------
/// Adds a toplevel mesh to the table of contents by inputing the name.
/// The created mesh is returned.
SiloMesh * vtkSiloTableOfContents::AddMesh(const char * meshName)
{
  //search map first for mesh name

  SiloMesh * mesh = new SiloMesh;
  mesh->Name = meshName;
  this->Meshes.insert( vtkstd::make_pair(meshName, mesh));
  return mesh;
}

//----------------------------------------------------------------------------
/// Adds a mesh block to a toplevel mesh in the table of contents.
/// Input is the meshblock name and mesh parent, and the created block is returned.
SiloMeshBlock * vtkSiloTableOfContents::AddMeshBlock(const char * meshName, SiloMesh * parent)
{
 // error check for null parent
 // search map for meshname

  SiloMeshBlock * meshBlock = new SiloMeshBlock;
  meshBlock->Name = meshName;
  this->MeshBlocks.insert( vtkstd::make_pair(meshName, meshBlock));
  parent->blocks.push_back(meshBlock);
  return meshBlock;
}

//----------------------------------------------------------------------------
/// Finds a toplevel mesh in the table of contents by name
SiloMesh * vtkSiloTableOfContents::GetMesh(const char * meshName)
{
  SiloMesh * mesh = 0;
  MeshIter itr = this->Meshes.find(meshName);
  if( itr != this->Meshes.end() )
    {
    mesh = itr->second;
    }
  return mesh;
}

//----------------------------------------------------------------------------
/// Finds a meshblock in the table of contents
SiloMeshBlock * vtkSiloTableOfContents::GetMeshBlock(const char * meshName)
{
  SiloMeshBlock * meshBlock = 0;
  MeshBlockMapIter itr = this->MeshBlocks.find(meshName);
  if( itr != this->MeshBlocks.end() )
    {
    meshBlock = itr->second;
    }
  return meshBlock;
}

//----------------------------------------------------------------------------
/// Adds a filename to the table of contents
int vtkSiloTableOfContents::AddFile(const char * filename)
{
  //Add the filename to TOC->Filenames vector
  this->Filenames.push_back(filename);
  //Add placeholder to TOC->DBFiles
  this->DBFiles.push_back(0);
  //Return the index of the filename we just added
  return this->Filenames.size() - 1;
}

//----------------------------------------------------------------------------
/// Open a file for reading
DBfile *vtkSiloTableOfContents::OpenFile(int index)
{
  vtkDebugMacro("OpenFile");

  // Check that the index argument is within bounds
  int nFiles = this->Filenames.size();
  if (index < 0 || index >= nFiles)
    {
    vtkErrorMacro("Index out of range.");
    return 0;
    }

  // Check to see if the file has already been opened.
  if (this->DBFiles[index] != NULL)
    {
    vtkDebugMacro("Returning file: " << index);
    return this->DBFiles[index];
    }

  vtkstd::string fullName = this->TOCPath + this->Filenames[index];
  const char * fname = fullName.c_str();
  vtkDebugMacro("Opening silo file: " << fname);

  // Open the Silo file. Impose priority order on drivers by first
  // trying PDB, then HDF5, then fall-back to UNKNOWN
  DBfile * dbfile = 0;
  if (((dbfile = DBOpen(fname, DB_PDB, DB_READ)) == NULL) &&
      ((dbfile = DBOpen(fname, DB_HDF5, DB_READ)) == NULL) &&
      ((dbfile = DBOpen(fname, DB_UNKNOWN, DB_READ)) == NULL))
    {
    vtkErrorMacro("Could not open file: " << fname);
    return 0;
    }

  // Store the dbfile pointer.
  this->DBFiles[index] = dbfile;

  // Lets try to make absolutely sure this is really a Silo file and
  // not just a PDB file that PD_Open succeeded on.
  if (!DBInqVarExists(dbfile, "_silolibinfo")) // newer silo files have this
    {
    // Count how many silo objects we have
    DBtoc *toc = DBGetToc(dbfile);
    int nSiloObjects = toc->nmultimesh + toc->ncsgmesh + toc->nqmesh +
              toc->nucdmesh + toc->nptmesh + toc->nmultivar + toc->ncsgvar +
              toc->nqvar + toc->nucdvar + toc->nptvar + toc->nmat +
              toc->nmultimat + toc->nmatspecies + toc->nmultimatspecies +
              toc->ndir + toc->ndefvars + toc->ncurve;
    if (nSiloObjects <= 0)
      {
      // We don't appear to have any silo objects, so we'll invalidate it
      vtkErrorMacro("Although the Silo library succesfully opened "
                     << fname
                     << " the file contains no silo objects. It may be a PDB file.");
      this->CloseFile(index);
      return 0;
      }
    }

  return this->DBFiles[index];
}

//----------------------------------------------------------------------------
/// Open a file for reading.
DBfile *vtkSiloTableOfContents::OpenFile(const char *filename)
{
  vtkDebugMacro("OpenFile");
  int fileIndex = this->DetermineFileNameIndex(filename);
  if (fileIndex < 0)
    {
    vtkDebugMacro(<< filename << " is an unseen file, adding to filenames.");
    // We have asked for a previously unseen file.
    // Add it to the list and continue.
    fileIndex = this->AddFile(filename);
    }
  return this->OpenFile(fileIndex);
}

//----------------------------------------------------------------------------
/// Get a pointer to a previously opened file.
DBfile *vtkSiloTableOfContents::GetFile(int index)
{
  vtkDebugMacro("GetFile");

  int nFiles = this->Filenames.size();
  if (index < 0 || index >= nFiles)
    {
    vtkErrorMacro("Index out of range.");
    return 0;
    }

  // Check that the DBfile at index is valid
  if (this->DBFiles[index] == NULL)
    {
    vtkErrorMacro("File index " << index << " is null. It probably has not been opened.");
    return 0;
    }

  return this->DBFiles[index];
}

//----------------------------------------------------------------------------
/// Close all files
void vtkSiloTableOfContents::CloseAllFiles()
{
  int nFiles = this->Filenames.size();
  for (int i = 0; i < nFiles; ++i)
    {
    this->CloseFile(i);
    }
}

//----------------------------------------------------------------------------
/// Close individual file
void vtkSiloTableOfContents::CloseFile(int index)
{
  vtkDebugMacro("CloseFile");

  //Check that the index argument is within bounds
  int nFiles = this->Filenames.size();
  if (index < 0 || index >= nFiles)
    {
    vtkErrorMacro("Index out of range.");
    return;
    }

  if (this->DBFiles[index] != NULL)
    {
    vtkDebugMacro("Closing silo file " << this->Filenames[index]);
    if (DBClose(this->DBFiles[index]) != 0)
      {
      vtkErrorMacro("Error closing dbfile: " << this->Filenames[index]);
      }
    this->DBFiles[index] = 0;
    }
}

//----------------------------------------------------------------------------
/// Set the filename of the root data file (table of contents file)
void vtkSiloReader::SetFileName(char * filename)
{
  this->FileName = filename;
  if (!filename)
    {
    this->TOC->TOCFile = "";
    this->TOC->TOCPath = "";
    return;
    }

  vtkstd::string fullName(filename);

  #ifdef _WIN32
  char slashChar[] = "\\";
  #else
  char slashChar[] = "/";
  #endif

  unsigned int i = fullName.find_last_of(slashChar);
  if (i == vtkstd::string::npos)
    {
    this->TOC->TOCFile = fullName;
    this->TOC->TOCPath = "";
    }
  else
    {
    ++i; //increment i past the slash character
    this->TOC->TOCFile = fullName.substr(i);
    this->TOC->TOCPath = fullName.substr(0, i);
    }
}

//----------------------------------------------------------------------------
/// Determine the index of a silo file given a pointer to the silo file
int vtkSiloTableOfContents::DetermineFileIndex(DBfile * file)
{
  for (unsigned int i = 0; i < this->DBFiles.size(); ++i)
    {
    if (this->DBFiles[i] == file)
      {
      return i;
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
/// Determine the index of a silo file given the filename
int vtkSiloTableOfContents::DetermineFileNameIndex(const char * filename)
{
  int nFiles = this->Filenames.size();
  for (int i = 0; i < nFiles; ++i)
    {
    if (this->Filenames[i] == filename)
      {
      return i;
      }
    }
  return -1;
}


//----------------------------------------------------------------------------
/// Check if a file should be read by this process or skipped
int vtkSiloReader::FileIsInDomain(int index)
{
  vtkDebugMacro("FileIsInDomain: " << index);

  int nFiles = this->TOC->Filenames.size();
  int filesPerProcess = (nFiles / this->NumProcesses);
  int startFileIndex = filesPerProcess * this->ProcessId;
  int endFileIndex = startFileIndex + filesPerProcess;
  if (this->ProcessId+1 == this->NumProcesses) endFileIndex = nFiles;

 // vtkDebugMacro("Process Id: " << this->ProcessId);
 // vtkDebugMacro("Nfiles: " << nFiles);
 // vtkDebugMacro("Files per process: " << filesPerProcess);
 // vtkDebugMacro("Start file index: " << startFileIndex);
 // vtkDebugMacro("End file index: " << endFileIndex);

  if (index < startFileIndex || index >= endFileIndex)
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------

// All xml elements have the tag "e" to conserve space
// Attributes:
// t - type
// n - name
//
// XML Structure:
/*
<e>                       <!-- root node -->
  <e>                       <!-- filenames -->
    <e n="filename.silo"/>
    <e n="more-filenames.silo"/>
  </e>
  <e>                       <!-- meshes --!>
    <e n="mesh1">             <!-- mesh name --!>
      <e n="mesh1" t="510">     <!-- mesh block name and type -->
        <e>                       <!-- variables -->
          <e n="var1"/>             <!-- var name-->
          <e n="var2"/>
        </e>
        <e>                       <!-- materials -->
          <e n="mat1"/>             <!-- material name -->
          <e n="mat2"/>
        </e>
      </e>
              <!-- more mesh blocks could go here... -->
    </e>
          <!-- more meshes could go here... -->
  </e>
</e>
*/
//
// The returned vtkXMLDAtaElement must be deleted by the caller.
vtkXMLDataElement * vtkSiloTableOfContents::ToXML()
{
  vtkXMLDataElement * root = vtkXMLDataElement::New();
  vtkXMLDataElement * filenamesNode = vtkXMLDataElement::New();
  vtkXMLDataElement * meshesNode = vtkXMLDataElement::New();

  root->SetName("e");
  filenamesNode->SetName("e");
  meshesNode->SetName("e");

  // For each filename
  StringListIter filenameIter = this->Filenames.begin();
  for (; filenameIter != this->Filenames.end(); ++filenameIter)
    {
    vtkXMLDataElement * filenameNode = vtkXMLDataElement::New();
    filenameNode->SetName("e");
    filenameNode->SetAttribute("n", (*filenameIter).c_str());
    filenamesNode->AddNestedElement(filenameNode);
    filenameNode->Delete();
    }

  // For each mesh
  MeshIter meshIter = this->Meshes.begin();
  for (; meshIter != this->Meshes.end(); ++meshIter)
    {
    SiloMesh * mesh = meshIter->second;
    vtkXMLDataElement * meshNode = vtkXMLDataElement::New();
    meshNode->SetName("e");
    meshNode->SetAttribute("n", mesh->Name.c_str());

    // For each mesh block
    MeshBlockIter meshBlockIter = mesh->blocks.begin();
    for (; meshBlockIter != mesh->blocks.end(); ++meshBlockIter)
      {
      SiloMeshBlock * meshBlock = *meshBlockIter;
      vtkXMLDataElement * meshBlockNode = vtkXMLDataElement::New();
      vtkXMLDataElement * variablesNode = vtkXMLDataElement::New();
      vtkXMLDataElement * materialsNode = vtkXMLDataElement::New();
      meshBlockNode->SetName("e");
      variablesNode->SetName("e");
      materialsNode->SetName("e");
      meshBlockNode->SetAttribute("n", meshBlock->Name.c_str());
      meshBlockNode->SetIntAttribute("t", meshBlock->Type);

      // For each variable
      StringListIter varIter = meshBlock->Variables.begin();
      for (; varIter != meshBlock->Variables.end(); ++varIter)
        {
        vtkXMLDataElement * varNode = vtkXMLDataElement::New();
        varNode->SetName("e");
        varNode->SetAttribute("n", (*varIter).c_str());
        variablesNode->AddNestedElement(varNode);
        varNode->Delete();
        }

      // For each material
      StringListIter matIter = meshBlock->Materials.begin();
      for (; matIter != meshBlock->Materials.end(); ++matIter)
        {
        vtkXMLDataElement * matNode = vtkXMLDataElement::New();
        matNode->SetName("e");
        matNode->SetAttribute("n", (*matIter).c_str());
        materialsNode->AddNestedElement(matNode);
        matNode->Delete();
        }

      meshBlockNode->AddNestedElement(variablesNode);
      meshBlockNode->AddNestedElement(materialsNode);
      meshNode->AddNestedElement(meshBlockNode);
      meshBlockNode->Delete();
      variablesNode->Delete();
      materialsNode->Delete();
      }
    meshesNode->AddNestedElement(meshNode);
    meshNode->Delete();
    }

  root->AddNestedElement(filenamesNode);
  root->AddNestedElement(meshesNode);
  filenamesNode->Delete();
  meshesNode->Delete();

  // Store XML string
  vtksys_ios::ostringstream oss;
  root->PrintXML(oss, vtkIndent());
  this->TOCString = oss.str();

  return root;
}

// All xml elements have the name "e" to conserve space
// Attributes:
// t - type
// n - name
//
void vtkSiloTableOfContents::FromXML(vtkXMLDataElement * root)
{
  this->CloseAllFiles();
  this->Filenames.clear();
  this->DBFiles.clear();
  this->Clear();

  int size = root->GetNumberOfNestedElements();
  if (size != 2)
    {
    vtkGenericWarningMacro("XML table of contents has invalid structure!");
    return;
    }

  vtkXMLDataElement * filenamesNode = root->GetNestedElement(0);
  vtkXMLDataElement * meshesNode = root->GetNestedElement(1);

  // For each filename
  int filenamesSize = filenamesNode->GetNumberOfNestedElements();
  for (int i = 0; i < filenamesSize; ++i)
    {
    vtkXMLDataElement * filenameNode = filenamesNode->GetNestedElement(i);
    this->AddFile(filenameNode->GetAttribute("n"));
    }

  // For each mesh
  int meshesSize = meshesNode->GetNumberOfNestedElements();
  for (int i = 0; i < meshesSize; ++i)
    {
    vtkXMLDataElement * meshNode = meshesNode->GetNestedElement(i);
    SiloMesh * mesh = this->AddMesh(meshNode->GetAttribute("n"));

    // For each mesh block
    int meshBlocksSize = meshNode->GetNumberOfNestedElements();
    for (int j = 0; j < meshBlocksSize; ++j)
      {
      vtkXMLDataElement * meshBlockNode = meshNode->GetNestedElement(j);
      vtkXMLDataElement * variablesNode = meshBlockNode->GetNestedElement(0);
      vtkXMLDataElement * materialsNode = meshBlockNode->GetNestedElement(1);

      int meshBlockType =  0;
      meshBlockNode->GetScalarAttribute("t", meshBlockType);
      SiloMeshBlock * meshBlock = this->AddMeshBlock(
           meshBlockNode->GetAttribute("n"), mesh);
      meshBlock->Type = meshBlockType;

      // For each variable
      int variablesSize = variablesNode->GetNumberOfNestedElements();
      for (int k = 0; k < variablesSize; ++k)
        {
        vtkXMLDataElement * variableNode = variablesNode->GetNestedElement(k);
        meshBlock->Variables.push_back(variableNode->GetAttribute("n"));
        }

      // For each material
      int materialsSize = materialsNode->GetNumberOfNestedElements();
      for (int k = 0; k < materialsSize; ++k)
        {
        vtkXMLDataElement * materialNode = materialsNode->GetNestedElement(k);
        meshBlock->Materials.push_back(materialNode->GetAttribute("n"));
        }
      }
    }

}

//----------------------------------------------------------------------------
/// Broadcast the table of contents.
/// If process id equals 0 then broadcast, else receive.
void vtkSiloReader::BroadcastTableOfContents()
{
  vtkDebugMacro("BroadcastTableOfContents");

  vtkXMLDataElement * root = 0;
  vtkIdType length = 0;
  if (this->ProcessId == 0)
    {
    root = this->TOC->ToXML();
    vtksys_ios::ostringstream oss;
    root->PrintXML(oss, vtkIndent());
    root->Delete();
    root = 0;
    const vtkstd::string & str = oss.str();
    length = str.length();
    vtkDebugMacro("Broadcasting xml string length: " << length);
    vtkDebugMacro(<<str);
    this->Controller->Broadcast(&length, 1, 0);
    this->Controller->Broadcast(const_cast<char*>(str.c_str()), length, 0);
    }
  else
    {
    this->Controller->Broadcast(&length, 1, 0);
    char * xmlStr = new char[length+1];
    this->Controller->Broadcast(xmlStr, length, 0);
    vtkDebugMacro("Received xml string length: " << length);
    vtkDebugMacro(<<xmlStr);
    xmlStr[length] = 0; // add null terminating byte
    vtkSiloReaderXMLParser *parser = vtkSiloReaderXMLParser::New();
    parser->Parse(xmlStr, length);
    delete xmlStr;
    root = parser->GetRootElement();
    this->TOC->FromXML(root);
    parser->Delete();
    this->TableOfContentsRead = true;
    }
}

//----------------------------------------------------------------------------

// This method is useful for testing the serialization of the TOC.
// It converts the table of contents to an xml string,
// clears the table of contents, and then recreates the
// table of contents from the xml string.  This method is called
// when testing with only 1 process to make sure the table of
// contents is not corrupted when serialized.
void vtkSiloReader::TestBroadcastTableOfContents()
{
  vtkDebugMacro("TestBroadcastTableOfContents");

  // Create an XML string from TOC
  vtkXMLDataElement * root = 0;
  vtkIdType length = 0;
  root = this->TOC->ToXML();
  vtksys_ios::ostringstream oss;
  root->PrintXML(oss, vtkIndent());
  root->Delete();
  root = 0;
  const vtkstd::string & str = oss.str();
  length = str.length();

  vtkDebugMacro("Broadcasting xml string length: " << length);
  vtkDebugMacro(<<str);

  // Recreate the TOC from XML string
  vtkSiloReaderXMLParser *parser = vtkSiloReaderXMLParser::New();
  parser->Parse(str.c_str(), length);
  root = parser->GetRootElement();
  this->TOC->FromXML(root);
  parser->Delete();
  this->TableOfContentsRead = true;
}

//----------------------------------------------------------------------------
/// This method is called by RequestData.
/// It creates the multiblock dataset output.
int vtkSiloReader::CreateDataSet(vtkMultiBlockDataSet *output)
{
  vtkDebugMacro(<<"CreateDataSet");


  MeshBlockMapIter itr = this->TOC->MeshBlocks.begin();
  for ( ; itr != this->TOC->MeshBlocks.end(); ++itr)
    {
    SiloMeshBlock * meshBlock = itr->second;
    const char * meshName = itr->first.c_str();
    int meshType = meshBlock->Type;

    int index = 0;
    char * correctName;
    DetermineFileIndexAndDirectory(const_cast<char *>(meshName), index, 0, correctName);
    if ( !this->FileIsInDomain(index) )
      {
      output->SetBlock( output->GetNumberOfBlocks(), 0);
      continue;
      }

    vtkDebugMacro("Process " << this->ProcessId << " of "
      << this->NumProcesses << " in file " << index << " processing mesh: " << meshName);

    vtkDataSet *mesh = 0;
    DBfile * correctFile = 0;
    DetermineFileAndDirectory(const_cast<char *>(meshName), correctFile, 0, correctName);
    if (!correctFile) // correctFile is table of contents file
      {
      correctFile = this->TOC->OpenFile(0);
      }

    switch (meshType)
      {
      case DB_POINTMESH:
        mesh = this->GetPointMesh(correctFile, correctName);
      break;
      case DB_QUADMESH:
      case DB_QUAD_RECT:
      case DB_QUAD_CURV:
        mesh = this->GetQuadMesh(correctFile, correctName);
      break;
      case DB_UCDMESH:
       mesh = this->GetUnstructuredMesh(correctFile, correctName);
      break;
      case DB_CURVE:
        mesh = this->GetCurve(correctFile, correctName);
      break;
      default:
        vtkErrorMacro("Mesh \"" << meshName << "\" has unhandled type");
      }

    // early exit the loop if mesh is null
    if (!mesh)
      {
      vtkErrorMacro("Error creating dataset from mesh: " << meshName);
      continue;
      }


    // Convert silo variables to vtk data arrays
    // and add arrays to vtk data set as point or cell data

    // Loop for each variable associated with the mesh
    vtkstd::vector< vtkstd::string >::const_iterator varIter;
    for (varIter = meshBlock->Variables.begin(); varIter != meshBlock->Variables.end(); ++varIter)
      {

      int cellCentered = 0;
      vtkDataArray * varArray = 0;
      const char * varName = (*varIter).c_str();

      DBfile * correctFile = 0;
      char * correctName;
      DetermineFileAndDirectory(const_cast<char *>(varName), correctFile, 0, correctName);
      if (!correctFile) // correctFile is table of contents file
        {
        correctFile = this->TOC->OpenFile(0);
        }

      switch (meshType)
        {
        case DB_POINTMESH:
          varArray = GetPointVar(correctFile, correctName, cellCentered);
        break;
        case DB_QUADMESH:
        case DB_QUAD_RECT:
        case DB_QUAD_CURV:
          varArray = GetQuadVar(correctFile, correctName, cellCentered);
        break;
        case DB_UCDMESH:
          varArray = GetUcdVar(correctFile, correctName, cellCentered);
        break;
        default:
          vtkErrorMacro("Mesh \"" << meshName << "\" has unhandled type");
        }

      // early exit the loop if array is null
      if (!varArray)
        {
        vtkErrorMacro("Error creating data array from variable: " << varName);
        continue;
        }

      // Set array name using the var name unqualified with dirname
      const char * varWithoutDir = strrchr(varName, '/');
      if (varWithoutDir)
        {
        varArray->SetName(varWithoutDir+1);
        }
      else
        {
        varArray->SetName(varName);
        }

      if (cellCentered)
        {
        // Sanity check
        if (varArray->GetNumberOfTuples() != mesh->GetNumberOfCells())
          {
          vtkErrorMacro("Number of cells in mesh does not match number of tuples in array: "
            << varName);
          }
        else
          {
          mesh->GetCellData()->AddArray(varArray);
          }
        }
      else
        {
        // Sanity check
        if (varArray->GetNumberOfTuples() != mesh->GetNumberOfPoints())
          {
          vtkErrorMacro("Number of points in mesh does not match number of tuples in array: "
            << varName);
          }
        else
          {
          mesh->GetPointData()->AddArray(varArray);
          }
        }
      varArray->Delete();
      }

    // Create data sets from material cell lists
    // Loop for each variable associated with the mesh
    vtkstd::vector< vtkstd::string >::const_iterator matIter;
    for (matIter = meshBlock->Materials.begin(); matIter != meshBlock->Materials.end(); ++matIter)
      {

      const char * matName = (*matIter).c_str();

      DBfile * correctFile = 0;
      char * correctName;
      DetermineFileAndDirectory(const_cast<char *>(matName), correctFile, 0, correctName);
      if (!correctFile) // correctFile is table of contents file
        {
        correctFile = this->TOC->OpenFile(0);
        }

      int meshNumCells = mesh->GetNumberOfCells();
      vtkDataArray * materialArray = 0;

      materialArray = this->GetMaterialArray(correctFile, correctName, meshNumCells);
      if (!materialArray)
        {
        vtkErrorMacro("Error creating material array from variable: " << matName);
        continue;
        }

      // Set array name using the var name unqualified with dirname
      const char * matWithoutDir = strrchr(matName, '/');
      if (matWithoutDir)
        {
        materialArray->SetName(matWithoutDir+1);
        }
      else
        {
        materialArray->SetName(matName);
        }
      mesh->GetCellData()->AddArray(materialArray);
      materialArray->Delete();
      }

    output->SetBlock(output->GetNumberOfBlocks(), mesh);
    mesh->Delete();
    }

  return 1;
}








//============================================================================
//============================================================================
//============================================================================
//============================================================================
//============================================================================
//============================================================================
//
//
//
// Many of the remaining methods are copied from Visit source code and
// need to be cleaned to match vtk style guidelines.  Many of them contain
// large blocks of commented out code.  The commented out code is for
// reference while this class is under development.
//
//
//
//============================================================================
//============================================================================
//============================================================================
//============================================================================
//============================================================================
//============================================================================


vtkDataArray * vtkSiloReader::GetMaterialArray(DBfile * file, const char * matName, int zones)
{
  DBmaterial *mat = DBGetMaterial(file, matName);
  if (!mat) return 0;

  vtkIntArray * material = vtkIntArray::New();
  material->SetNumberOfTuples(zones);

  // Sanity check
  // The total number of indices in matlist should
  // be equal to the number of mesh zones
  int totalZones = 0;
  for (int i = 0; i < mat->ndims; i++)
    {
    if (i == 0)
      {
      totalZones = mat->dims[i];
      }
    else
      {
      totalZones *= mat->dims[i];
      }
    }

  if (totalZones != zones)
    {
    vtkErrorMacro("Material " << matName << " has a matlist size not equal to mesh number of zones.");
    return 0;
    }

  // This loop collects the material names
  //for (int i = 0; i < mat->nmat; ++i)
    //{
    //printf("mat name: %s\n", mat->matnames[i]);
    //}

  for (int i = 0; i < zones; ++i)
    {
    material->SetValue(i, mat->matlist[i]);
    }

  DBFreeMaterial(mat);
  return material;
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Method: avtSiloFileFormat::RegisterDomainDirs
//
//  Purpose:
//      Registers directories that we know have blocks from a domain in them.
//      This way we won't traverse that directory later.
//
//  Arguments:
//      dirlist   A list of directories (with appended variable names).
//      nList     The number of elements in dirlist.
//      curDir    The name of the current directory.
// ****************************************************************************

void vtkSiloReader::RegisterDomainDirs(char **dirlist, int nList,
                                      const char *curDir)
{
  for (int i = 0 ; i < nList ; i++)
    {
    if (strcmp(dirlist[i], "EMPTY") == 0)
      {
      continue;
      }
      vtkstd::string str = PrepareDirName(dirlist[i], curDir);
      this->TOC->DomainDirs.insert(str);
    }
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Method: avtSiloFileFormat::ShouldGoToDir
//
//  Purpose:
//      Determines if the directory is one we should go to.  If we have already
//      seen a multi-var that has a block in that directory, don't bother going
//      in there.
//
//  Arguments:
//      dirname   The directory of interest.
//
//  Returns:      true if we should go in the directory, false otherwise.
// ****************************************************************************

bool vtkSiloReader::ShouldGoToDir(const char *dirname)
{
  if (this->TOC->DomainDirs.count(dirname) == 0)
    {
    vtkDebugMacro("Deciding to go into dir \"" << dirname << "\"");
    return true;
    }
  else
    {
    vtkDebugMacro("Skipping dir \"" << dirname << "\"");
    return false;
    }
}

//----------------------------------------------------------------------------

//
// This method is from Visit but slightly modified.
// This method used to take a third parameter- int domain
//
vtkDataSet * vtkSiloReader::GetQuadMesh(DBfile *dbfile, const char *mn)
{

  vtkDebugMacro("GetQuadMesh");

  //
  // Allow empty data sets
  //
  if (!mn)
    return 0;

  //
  // It's ridiculous, but Silo does not have all of the 'const's in their
  // library, so let's cast it away.
  //
  char *meshname  = const_cast<char *>(mn);

  //
  // Get the Silo construct.
  //
  DBquadmesh  *qm = DBGetQuadmesh(dbfile, meshname);
  if (qm == NULL)
    {
    vtkErrorMacro("No such mesh exists: " << mn);
    return 0;
    }

  if ( !VerifyQuadmesh(qm, meshname) )
    {
    // This block of code is untested, I wrote it to
    // replace a Visit style exception.  Instead of
    // throwing exception I call DBFreeQuadmesh and
    // return.
    vtkErrorMacro("Could not verify quadmesh.");
    DBFreeQuadmesh(qm);
    return 0;
    }

  vtkDataSet *ds = NULL;
  if (qm->coordtype == DB_COLLINEAR)
    {
    ds = CreateRectilinearMesh(qm);
    }
  else
    {
    // Not tested yet
    ds = CreateCurvilinearMesh(qm);
    }

  GetQuadGhostZones(qm, ds);

  //
  // Add group id as field data
  //
  vtkIntArray *grp_id_arr = vtkIntArray::New();
  grp_id_arr->SetNumberOfTuples(1);
  grp_id_arr->SetValue(0, qm->group_no);
  grp_id_arr->SetName("group_id");
  ds->GetFieldData()->AddArray(grp_id_arr);
  grp_id_arr->Delete();

  //
  // Determine the indices of the mesh within its group.  Add that to the
  // VTK dataset as field data.
  //
  vtkIntArray *arr = vtkIntArray::New();
  arr->SetNumberOfTuples(3);
  arr->SetValue(0, qm->base_index[0]);
  arr->SetValue(1, qm->base_index[1]);
  arr->SetValue(2, qm->base_index[2]);
  arr->SetName("base_index");

/* Comment out this stuff for now...

    //
    //  If we're not really sure that the base_index was set in the file,
    //  check for connectivity info.
    //
    if (qm->base_index[0] == 0 &&
        qm->base_index[1] == 0 &&
        qm->base_index[2] == 0)
    {
        void_ref_ptr vr = cache->GetVoidRef("any_mesh",
                        AUXILIARY_DATA_DOMAIN_BOUNDARY_INFORMATION, -1, -1);
        if (*vr != NULL)
        {
            avtStructuredDomainBoundaries *dbi =
                (avtStructuredDomainBoundaries*)*vr;
            if (dbi != NULL)
            {
                int ext[6];
                dbi->GetExtents(domain, ext);
                arr->SetValue(0, ext[0]);
                arr->SetValue(1, ext[2]);
                arr->SetValue(2, ext[4]);
            }
        }
    }
*/

  ds->GetFieldData()->AddArray(arr);
  arr->Delete();

  DBFreeQuadmesh(qm);

  return ds;
}

//----------------------------------------------------------------------------

//
// This method is from Visit.  I simply replaced
// Visit style exceptions with vtkErrorMacros
//
int vtkSiloReader::VerifyQuadmesh(DBquadmesh *qm, const char *meshname)
{
  vtkDebugMacro("VerifyQuadmesh");

    if (qm->ndims == 3)
    {
        //
        // Make sure the dimensions are correct.
        //
        if (qm->nnodes != qm->dims[0]*qm->dims[1]*qm->dims[2])
        {
            if (qm->dims[0] > 100000 || qm->dims[1] > 100000
                || qm->dims[2] > 100000)
            {
                int orig[3];
                orig[0] = qm->dims[0];
                orig[1] = qm->dims[1];
                orig[2] = qm->dims[2];

                //
                // See if the max_index has any clues, first without ghost
                // zones, then with.
                //
                if (qm->nnodes == qm->max_index[0] * qm->max_index[1]
                                  * qm->max_index[2])
                {
                    qm->dims[0] = qm->max_index[0];
                    qm->dims[1] = qm->max_index[1];
                    qm->dims[2] = qm->max_index[2];
                }
                else if (qm->nnodes == (qm->max_index[0]+1) *
                                   (qm->max_index[1]+1) * (qm->max_index[2]+1))
                {
                    qm->dims[0] = qm->max_index[0]+1;
                    qm->dims[1] = qm->max_index[1]+1;
                    qm->dims[2] = qm->max_index[2]+1;
                }
                else if (qm->nnodes == (qm->max_index[0]) *(qm->max_index[1]+1)
                                        * (qm->max_index[2]+1))
                {
                    qm->dims[0] = qm->max_index[0];
                    qm->dims[1] = qm->max_index[1]+1;
                    qm->dims[2] = qm->max_index[2]+1;
                }
                else if (qm->nnodes == (qm->max_index[0]+1) *(qm->max_index[1])
                                        * (qm->max_index[2]+1))
                {
                    qm->dims[0] = qm->max_index[0]+1;
                    qm->dims[1] = qm->max_index[1];
                    qm->dims[2] = qm->max_index[2]+1;
                }
                else if (qm->nnodes == (qm->max_index[0]+1) *
                                     (qm->max_index[1]+1) * (qm->max_index[2]))
                {
                    qm->dims[0] = qm->max_index[0]+1;
                    qm->dims[1] = qm->max_index[1]+1;
                    qm->dims[2] = qm->max_index[2];
                }
                else if (qm->nnodes == (qm->max_index[0]) * (qm->max_index[1])
                                        * (qm->max_index[2]+1))
                {
                    qm->dims[0] = qm->max_index[0];
                    qm->dims[1] = qm->max_index[1];
                    qm->dims[2] = qm->max_index[2]+1;
                }
                else if (qm->nnodes == (qm->max_index[0]) *(qm->max_index[1]+1)
                                        * (qm->max_index[2]))
                {
                    qm->dims[0] = qm->max_index[0];
                    qm->dims[1] = qm->max_index[1]+1;
                    qm->dims[2] = qm->max_index[2];
                }
                else if (qm->nnodes == (qm->max_index[0]+1) *(qm->max_index[1])
                                        * (qm->max_index[2]))
                {
                    qm->dims[0] = qm->max_index[0]+1;
                    qm->dims[1] = qm->max_index[1];
                    qm->dims[2] = qm->max_index[2];
                }
                else
                {
                    vtkErrorMacro("The dimensions of the mesh did not match the "
                           << "number of nodes, cannot continue");
                    return 0;
                }

                    vtkDebugMacro("A quadmesh had dimensions " << orig[0] << ", "
                                   << orig[1] << ", " << orig[2] << ", which did "
                                   << "not result in the specified number of nodes: "
                                   << qm->nnodes << ". The dimensions are being adjusted to "
                                   << qm->dims[0] << ", " << qm->dims[1] << ", " << qm->dims[2]
                                   << ".");
            }
            else if (qm->nnodes > 10000000)
            {
                int orig = qm->nnodes;
                qm->nnodes = qm->dims[0]*qm->dims[1]*qm->dims[2];
                if (orig < 10000000)
                {
                    vtkDebugMacro("The number of nodes is stored as " << orig << ", but "
                                 << "that does not agree with the dimensions "
                                 << qm->dims[0] << ", " << qm->dims[1] << ", " << qm->dims[2]
                                 << ".  Correcting to: " << qm->nnodes << ".");
                }
                else
                {
                    vtkErrorMacro("The dimensions of the mesh did not match the "
                           << "number of nodes, cannot continue.");
                    return 0;
                }
            }
            else
            {
                vtkErrorMacro("The dimensions of the mesh did not match the "
                       << "number of nodes, cannot continue.");
                return 0;
            }
        }
    }
    else if (qm->ndims == 2)
    {
        //
        // Make sure the dimensions are correct.
        //
        if (qm->nnodes != qm->dims[0]*qm->dims[1])
        {
            if (qm->dims[0] > 100000 || qm->dims[1] > 100000)
            {
                int orig[2];
                orig[0] = qm->dims[0];
                orig[1] = qm->dims[1];

                //
                // See if the max_index has any clues, first without ghost
                // zones, then with.
                //
                if (qm->nnodes == qm->max_index[0] * qm->max_index[1])
                {
                    qm->dims[0] = qm->max_index[0];
                    qm->dims[1] = qm->max_index[1];
                }
                else if (qm->nnodes == (qm->max_index[0]+1) *
                                       (qm->max_index[1]+1))
                {
                    qm->dims[0] = qm->max_index[0]+1;
                    qm->dims[1] = qm->max_index[1]+1;
                }
                else if (qm->nnodes == (qm->max_index[0])*(qm->max_index[1]+1))
                {
                    qm->dims[0] = qm->max_index[0];
                    qm->dims[1] = qm->max_index[1]+1;
                }
                else if (qm->nnodes == (qm->max_index[0]+1)*(qm->max_index[1]))
                {
                    qm->dims[0] = qm->max_index[0]+1;
                    qm->dims[1] = qm->max_index[1];
                }
                else
                {
                vtkErrorMacro("The dimensions of the mesh did not match the "
                       << "number of nodes, cannot continue.");
                return 0;
                }
                    vtkDebugMacro("A quadmesh had dimensions " << orig[0] << ", "
                                   << orig[1] << ", which did "
                                   << "not result in the specified number of nodes: "
                                   << qm->nnodes << ". The dimensions are being adjusted to "
                                   << qm->dims[0] << ", " << qm->dims[1] << ".");

            }
            else if (qm->nnodes > 10000000)
            {
                int orig = qm->nnodes;
                qm->nnodes = qm->dims[0]*qm->dims[1];
                if (orig < 10000000)
                {
                    vtkDebugMacro("The number of nodes is stored as " << orig << ", but "
                                 << "that does not agree with the dimensions "
                                 << qm->dims[0] << ", " << qm->dims[1]
                                 << ".  Correcting to: " << qm->nnodes << ".");
                }
                else
                {
                vtkErrorMacro("The dimensions of the mesh did not match the "
                       << "number of nodes, cannot continue.");
                return 0;
                }
            }
            else
            {
                vtkErrorMacro("The dimensions of the mesh did not match the "
                       << "number of nodes, cannot continue.");
                return 0;
            }
        }
    }

  return 1;
}

//----------------------------------------------------------------------------

//
// This method is from Visit.
//
vtkDataSet *vtkSiloReader::CreateRectilinearMesh(DBquadmesh *qm)
{

  vtkDebugMacro("CreateRectilinearMesh");

    int   i, j;

    vtkRectilinearGrid   *rgrid   = vtkRectilinearGrid::New();

    //
    // Populate the coordinates.  Put in 3D points with z=0 if the mesh is 2D.
    //
    int           dims[3];
    vtkFloatArray   *coords[3];
    float ** floatCoords = reinterpret_cast<float **>(qm->coords);
    for (i = 0 ; i < 3 ; i++)
    {
        // Default number of components for an array is 1.
        coords[i] = vtkFloatArray::New();
        //printf("i = %d\n",i);
        if (i < qm->ndims)
        {
            //printf("  qm->dims[%d] = %d\n",i,qm->dims[i]);
            dims[i] = qm->dims[i];
            coords[i]->SetNumberOfTuples(dims[i]);
            for (j = 0 ; j < dims[i] ; j++)
            {
                //printf("      qm->coords[%d][%d] = %f\n", i,j,qm->coords[i][j]);
                coords[i]->SetComponent(j, 0, floatCoords[i][j]);
            }
        }
        else
        {
            dims[i] = 1;
            coords[i]->SetNumberOfTuples(1);
            coords[i]->SetComponent(0, 0, 0.);
        }
    }
    rgrid->SetDimensions(dims);
    rgrid->SetXCoordinates(coords[0]);
    coords[0]->Delete();
    rgrid->SetYCoordinates(coords[1]);
    coords[1]->Delete();
    rgrid->SetZCoordinates(coords[2]);
    coords[2]->Delete();

    return rgrid;
}

//----------------------------------------------------------------------------

template <class T>
static void CopyQuadCoordinates(T *dest, int nx, int ny, int nz, int morder,
    const T *const c0, const T *const c1, const T *const c2)
{
    int i, j, k;

    if (morder == DB_ROWMAJOR)
    {
        int nxy = nx * ny;
        for (k = 0; k < nz; k++)
        {
            for (j = 0; j < ny; j++)
            {
                for (i = 0; i < nx; i++)
                {
                    int idx = k*nxy + j*nx + i;
                    *dest++ = c0 ? c0[idx] : 0.;
                    *dest++ = c1 ? c1[idx] : 0.;
                    *dest++ = c2 ? c2[idx] : 0.;
                }
            }
        }
    }
    else
    {
        int nyz = ny * nz;
        for (k = 0; k < nz; k++)
        {
            for (j = 0; j < ny; j++)
            {
                for (i = 0; i < nx; i++)
                {
                    int idx = k + j*nz + i*nyz;
                    *dest++ = c0 ? c0[idx] : 0.;
                    *dest++ = c1 ? c1[idx] : 0.;
                    *dest++ = c2 ? c2[idx] : 0.;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------

vtkDataSet *vtkSiloReader::CreateCurvilinearMesh(DBquadmesh *qm)
{

  vtkDebugMacro("CreateCurvilinearMesh");
    //
    // Create the VTK objects and connect them up.
    //
    vtkStructuredGrid    *sgrid   = vtkStructuredGrid::New();
    vtkPoints            *points  = vtkPoints::New();
    sgrid->SetPoints(points);
    points->Delete();

    //
    // Tell the grid what its dimensions are and populate the points array.
    //
    int dims[3];
    dims[0] = (qm->dims[0] > 0 ? qm->dims[0] : 1);
    dims[1] = (qm->dims[1] > 0 ? qm->dims[1] : 1);
    dims[2] = (qm->dims[2] > 0 ? qm->dims[2] : 1);
    sgrid->SetDimensions(dims);

    //
    // vtkPoints assumes float data type
    //
    if (qm->datatype == DB_DOUBLE)
        points->SetDataTypeToDouble();

    //
    // Populate the coordinates.  Put in 3D points with z=0 if the mesh is 2D.
    //
    int nx = qm->dims[0];
    int ny = qm->dims[1];
    int nz = qm->ndims <= 2 ? 1 : qm->dims[2];
    points->SetNumberOfPoints(qm->nnodes);
    void *pts = points->GetVoidPointer(0);
    if (qm->datatype == DB_DOUBLE)
    {
        CopyQuadCoordinates((double *) pts, nx, ny, nz, qm->major_order,
            (double *) qm->coords[0], (double *) qm->coords[1],
            qm->ndims <= 2 ? 0 : (double *) qm->coords[2]);
    }
    else
    {
        CopyQuadCoordinates((float *) pts, nx, ny, nz, qm->major_order,
            (float *) qm->coords[0], (float *) qm->coords[1],
            qm->ndims <= 2 ? 0 : (float *) qm->coords[2]);
    }

    return sgrid;
}

//----------------------------------------------------------------------------

void vtkSiloReader::GetQuadGhostZones(DBquadmesh *qm, vtkDataSet *ds)
{

  vtkDebugMacro("GetQuadGhostZones");
    //
    // Find the dimensions of the quad mesh.
    //
    int dims[3];
    dims[0] = (qm->dims[0] > 0 ? qm->dims[0] : 1);
    dims[1] = (qm->dims[1] > 0 ? qm->dims[1] : 1);
    dims[2] = (qm->dims[2] > 0 ? qm->dims[2] : 1);

    //
    //  Determine if we have ghost points
    //
    int first[3];
    int last[3];
    bool ghostPresent = false;
    bool badIndex = false;
    for (int i = 0; i < 3; i++)
    {
        first[i] = (i < qm->ndims ? qm->min_index[i] : 0);
        last[i]  = (i < qm->ndims ? qm->max_index[i] : 0);

        if (first[i] < 0 || first[i] >= dims[i])
          {
          vtkDebugMacro("bad index on first[" << i << "] dims is: " << dims[i] << ".");
          badIndex = true;
          }

        if (last[i] < 0 || last[i] >= dims[i])
          {
          vtkDebugMacro("bad index on last[" << i << "] dims is: " << dims[i] << ".");
          badIndex = true;
          }

        if (first[i] != 0 || last[i] != dims[i] -1)
        {
            ghostPresent = true;
        }
    }

    //
    //  Create the ghost zones array if necessary
    //
    if (ghostPresent && !badIndex)
    {
        bool *ghostPoints = new bool[qm->nnodes];
        //
        // Initialize as all ghost levels
        //
        for (int ii = 0; ii < qm->nnodes; ii++)
            ghostPoints[ii] = true;

        //
        // Set real values
        //
        for (int k = first[2]; k <= last[2]; k++)
            for (int j = first[1]; j <= last[1]; j++)
                for (int i = first[0]; i <= last[0]; i++)
                {
                    int index = k*dims[1]*dims[0] + j*dims[0] + i;
                    ghostPoints[index] = false;
                }

        //
        //  okay, now we have ghost points, but what we really want
        //  are ghost cells ... convert:  if all points associated with
        //  cell are 'real' then so is the cell.
        //
        unsigned char realVal = 0;
        unsigned char ghostVal = 0;

    //    avtGhostData::AddGhostZoneType(ghostVal,
    //                                   DUPLICATED_ZONE_INTERNAL_TO_PROBLEM);


        int ncells = ds->GetNumberOfCells();
        vtkIdList *ptIds = vtkIdList::New();
        vtkUnsignedCharArray *ghostCells = vtkUnsignedCharArray::New();
        ghostCells->SetName("avtGhostZones");
        ghostCells->Allocate(ncells);

        for (int i = 0; i < ncells; i++)
        {
            ds->GetCellPoints(i, ptIds);
            bool ghost = false;
            for (int idx = 0; idx < ptIds->GetNumberOfIds(); idx++)
                ghost |= ghostPoints[ptIds->GetId(idx)];

            if (ghost)
                ghostCells->InsertNextValue(ghostVal);
            else
                ghostCells->InsertNextValue(realVal);

        }
        ds->GetCellData()->AddArray(ghostCells);
        delete [] ghostPoints;
        ghostCells->Delete();
        ptIds->Delete();

        vtkIntArray *realDims = vtkIntArray::New();
        realDims->SetName("avtRealDims");
        realDims->SetNumberOfValues(6);
        realDims->SetValue(0, first[0]);
        realDims->SetValue(1, last[0]);
        realDims->SetValue(2, first[1]);
        realDims->SetValue(3, last[1]);
        realDims->SetValue(4, first[2]);
        realDims->SetValue(5, last[2]);
        ds->GetFieldData()->AddArray(realDims);
        ds->GetFieldData()->CopyFieldOn("avtRealDims");
        realDims->Delete();

        ds->SetUpdateGhostLevel(0);
    }
}

//----------------------------------------------------------------------------

vtkDataSet * vtkSiloReader::GetCurve(DBfile *dbfile, const char *cn)
{
    int i;

    //
    // It's ridiculous, but Silo does not have all of the `const's in their
    // library, so let's cast it away.
    //
    char *curvename  = const_cast<char *>(cn);

    //
    // Get the Silo construct.
    //
    DBcurve *cur = DBGetCurve(dbfile, curvename);
    if (cur == NULL)
      {
      vtkErrorMacro("No such mesh exists: " << cn);
      return 0;
      }

    //
    // Add all of the points to an array.
    //
    vtkPolyData *pd  = vtkPolyData::New();
    vtkPoints   *pts = vtkPoints::New();
    pd->SetPoints(pts);

    // DBForceSingle assures that all double data is converted to float
    // So, both are handled as float, here
    if (cur->datatype == DB_FLOAT ||
        cur->datatype == DB_DOUBLE)
    {
        vtkFloatArray *farr= vtkFloatArray::New();
        farr->SetNumberOfComponents(3);
        farr->SetNumberOfTuples(cur->npts);
        float * curX = reinterpret_cast<float *>(cur->x);
        float * curY = reinterpret_cast<float *>(cur->y);
        for (i = 0 ; i < cur->npts; i++)
            farr->SetTuple3(i, curX[i], curY[i], 0.0);
        pts->SetData(farr);
        farr->Delete();
    }
    else if (cur->datatype == DB_INT)
    {
        int *px = (int *) cur->x;
        int *py = (int *) cur->y;
        vtkIntArray *iarr= vtkIntArray::New();
        iarr->SetNumberOfComponents(3);
        iarr->SetNumberOfTuples(cur->npts);
        for (i = 0 ; i < cur->npts; i++)
            iarr->SetTuple3(i, px[i], py[i], 0);
        pts->SetData(iarr);
        iarr->Delete();
    }
    else if (cur->datatype == DB_SHORT)
    {
        short *px = (short *) cur->x;
        short *py = (short *) cur->y;
        vtkShortArray *sarr= vtkShortArray::New();
        sarr->SetNumberOfComponents(3);
        sarr->SetNumberOfTuples(cur->npts);
        for (i = 0 ; i < cur->npts; i++)
            sarr->SetTuple3(i, px[i], py[i], 0);
        pts->SetData(sarr);
        sarr->Delete();
    }
    else if (cur->datatype == DB_CHAR)
    {
        char *px = (char *) cur->x;
        char *py = (char *) cur->y;
        vtkCharArray *carr= vtkCharArray::New();
        carr->SetNumberOfComponents(3);
        carr->SetNumberOfTuples(cur->npts);
        for (i = 0 ; i < cur->npts; i++)
            carr->SetTuple3(i, px[i], py[i], 0);
        pts->SetData(carr);
        carr->Delete();
    }

    //
    // Connect the points up with line segments.
    //
    vtkCellArray *line = vtkCellArray::New();
    pd->SetLines(line);
    for (i = 1 ; i < cur->npts; i++)
    {
        line->InsertNextCell(2);
        line->InsertCellPoint(i-1);
        line->InsertCellPoint(i);
    }

    pts->Delete();
    line->Delete();

    DBFreeCurve(cur);

    return pd;
}

//----------------------------------------------------------------------------

vtkDataArray * vtkSiloReader::GetQuadVar(DBfile * dbfile, const char * vname, int &cellCentered)
{
  // It's ridiculous, but Silo does not have all of the `const's in their
  // library, so let's cast it away.
  char *varname  = const_cast<char *>(vname);

  // Get the Silo quadvar struct
  DBquadvar  *qv = DBGetQuadvar(dbfile, varname);
  if (qv == NULL)
    {
    vtkErrorMacro("Invalid variable: " << vname);
    return 0;
    }

  vtkDataArray * array;

  int dim = qv->nvals;
  if (dim == 1)
    array = this->GetQuadScalarVar(qv);
  else
    array = this->GetQuadVectorVar(qv);

  if (array == NULL)
    {
    DBFreeQuadvar(qv);
    return 0;
    }

  // This is how Visit determines centering for quadvars.
  // Using == on floats seems dangerous.
  if (qv->align[0] == 0.0f) cellCentered = 0;
  else cellCentered = 1;

  // Free the silo quad var struct
  DBFreeQuadvar(qv);
  return array;
}

//----------------------------------------------------------------------------

template <class T>
static void CopyAndReorderQuadVar(T* var2, int nx, int ny, int nz, const void *const src)
{
    const T *const var  = (const T *const) src;
    int nxy = nx * ny;
    int nyz = ny * nz;

    for (int k = 0; k < nz; k++)
    {
        for (int j = 0; j < ny; j++)
        {
            for (int i = 0; i < nx; i++)
            {
                var2[k*nxy + j*nx + i] = var[k + j*nz + i*nyz];
            }
        }
    }
}

//----------------------------------------------------------------------------

vtkDataArray *vtkSiloReader::GetQuadScalarVar(DBquadvar *qv)
{

  if (qv == NULL)
    {
    vtkErrorMacro("Given null pointer.");
    return 0;
    }

    // Populate the variable.  This assumes it is a scalar variable.
    vtkDataArray *scalars;
    if (qv->datatype == DB_DOUBLE)
        scalars = vtkDoubleArray::New();
    else
        scalars = vtkFloatArray::New();
    scalars->SetNumberOfTuples(qv->nels);
    if (qv->major_order == DB_ROWMAJOR || qv->ndims <= 1)
    {
        int size = sizeof(float);
        if (qv->datatype == DB_DOUBLE)
            size = sizeof(double);
        void *ptr = scalars->GetVoidPointer(0);
        memcpy(ptr, qv->vals[0], size*qv->nels);
    }
    else
    {
        void *var2 = scalars->GetVoidPointer(0);
        void *var  = qv->vals[0];

        int nx = qv->dims[0];
        int ny = qv->dims[1];
        int nz = qv->ndims == 3 ? qv->dims[2] : 1;
        if (qv->datatype == DB_DOUBLE)
            CopyAndReorderQuadVar((double *) var2, nx, ny, nz, var);
        else
            CopyAndReorderQuadVar((float *) var2, nx, ny, nz, var);
    }

    return scalars;
}

//----------------------------------------------------------------------------


vtkDataArray * vtkSiloReader::GetQuadVectorVar(DBquadvar *qv)
{

  if (qv == NULL)
    {
    vtkErrorMacro("Given null pointer.");
    return 0;
    }

    // Populate the variable.
    vtkDataArray *vectors;
    if (qv->datatype == DB_DOUBLE)
        vectors = vtkDoubleArray::New();
    else
        vectors = vtkFloatArray::New();
    vectors->SetNumberOfComponents(3);
    vectors->SetNumberOfTuples(qv->nels);
    if (qv->datatype == DB_DOUBLE)
    {
        double *v1 = (double *) qv->vals[0];
        double *v2 = (double *) qv->vals[1];
        double *v3 = (double *) (qv->nvals == 3 ? qv->vals[2] : 0);
        for (int i = 0 ; i < qv->nels ; i++)
            vectors->SetTuple3(i, v1[i], v2[i], v3 ? v3[i] : 0.);
    }
    else
    {
        float ** qvvals = reinterpret_cast<float **>(qv->vals);
        for (int i = 0 ; i < qv->nels ; i++)
        {
            float v3 = (qv->nvals == 3 ? qvvals[2][i] : 0.);
            vectors->SetTuple3(i, qvvals[0][i], qvvals[1][i], v3);
        }
    }

    return vectors;
}

//----------------------------------------------------------------------------

vtkDataSet * vtkSiloReader::GetPointMesh(DBfile *dbfile, const char *mn)
{
    int   i, j;

  vtkDebugMacro("GetPointMesh: " << mn);

    //
    // Allow empty data sets
    //
    if (!mn)
      return 0;

    //
    // It's ridiculous, but Silo does not have all of the `const's in their
    // library, so let's cast it away.
    //
    char *meshname  = const_cast<char *>(mn);

    //
    // Get the Silo construct.
    //
    DBpointmesh  *pm = DBGetPointmesh(dbfile, meshname);
    if (pm == NULL)
      {
      vtkErrorMacro("Could not find mesh: " << meshname);
      return 0;
      }

    //
    // Populate the coordinates.  Put in 3D points with z=0 if the mesh is 2D.
    //
    vtkPoints *points  = vtkPoints::New();
    points->SetNumberOfPoints(pm->nels);
    float *pts = (float *) points->GetVoidPointer(0);
    float ** pmcoords = reinterpret_cast<float **>(pm->coords);
    for (i = 0 ; i < 3 ; i++)
    {
        float *tmp = pts + i;
        if (pm->coords[i] != NULL)
        {
            for (j = 0 ; j < pm->nels ; j++)
            {
                *tmp = pmcoords[i][j];
                tmp += 3;
            }
        }
        else
        {
            for (j = 0 ; j < pm->nels ; j++)
            {
                *tmp = 0.;
                tmp += 3;
            }
        }
    }

    //
    // Create the VTK objects and connect them up.
    //
    vtkUnstructuredGrid    *ugrid   = vtkUnstructuredGrid::New();
    ugrid->SetPoints(points);
    ugrid->Allocate(pm->nels);
    vtkIdType onevertex[1];
    for (i = 0 ; i < pm->nels ; i++)
    {
        onevertex[0] = i;
        ugrid->InsertNextCell(VTK_VERTEX, 1, onevertex);
    }

    points->Delete();
    DBFreePointmesh(pm);
    return ugrid;
}

//----------------------------------------------------------------------------

vtkDataArray * vtkSiloReader::GetPointVectorVar(DBmeshvar *mv)
{
  if (mv == NULL)
    {
    vtkErrorMacro("Given null pointer.");
    return 0;
    }

  // Even if silo vector var is 2 dimensions we
  // will create a vtkFloatArray of 3 dimensions
  // and clamp the third value at zero
  vtkFloatArray   *vectors = vtkFloatArray::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(mv->nels);
  float ** mvvals = reinterpret_cast<float **>(mv->vals);
  for (int i = 0 ; i < mv->nels ; i++)
    {
    float v3 = (mv->nvals == 3 ? mvvals[2][i] : 0.);
    vectors->SetTuple3(i, mvvals[0][i], mvvals[1][i], v3);
    }
  return vectors;
}

//----------------------------------------------------------------------------

vtkDataArray *vtkSiloReader::GetPointScalarVar(DBmeshvar *mv)
{
  if (mv == NULL)
    {
    vtkErrorMacro("Given null pointer.");
    return 0;
    }

  // Populate the variable.  This assumes it is a scalar variable.
  vtkFloatArray   *scalars = vtkFloatArray::New();
  scalars->SetNumberOfTuples(mv->nels);
  float        *ptr     = (float *) scalars->GetVoidPointer(0);
  memcpy(ptr, mv->vals[0], sizeof(float)*mv->nels);
  return scalars;
}

//----------------------------------------------------------------------------

vtkDataArray *vtkSiloReader::GetPointVar(DBfile *dbfile, const char *vname, int &cellCentered)
{

  // It's ridiculous, but Silo does not have all of the `const's in their
  // library, so let's cast it away.
  char *varname  = const_cast<char *>(vname);

  // Get the Silo construct.
  DBmeshvar  *mv = DBGetPointvar(dbfile, varname);
  if (mv == NULL)
    {
    vtkErrorMacro("Point variable not found: " << vname);
    return 0;
    }


  vtkDataArray *array = 0;
  if (mv->nvals == 1)
    array = GetPointScalarVar(mv);
  else
    array = GetPointVectorVar(mv);

  if (array == NULL)
    {
    DBFreeMeshvar(mv);
    return 0;
    }

  // Point vars are always cell centered
  cellCentered = 0;

  DBFreeMeshvar(mv);
  return array;
}

//----------------------------------------------------------------------------

vtkDataArray *vtkSiloReader::GetUcdVar(DBfile *dbfile, const char *vname, int &cellCentered)
{
  // It's ridiculous, but Silo does not have all of the `const's in their
  // library, so let's cast it away.
  char *varname  = const_cast<char *>(vname);

  // Get the Silo construct.
  DBucdvar  *uv = DBGetUcdvar(dbfile, varname);
  if (uv == NULL)
    {
    vtkErrorMacro("Ucd variable not found: " << vname);
    return 0;
    }

  vtkDataArray *array = 0;
  if (uv->nvals == 1)
    array = GetUcdScalarVar(uv);
  else
    array = GetUcdVectorVar(uv);

  if (array == NULL)
    {
    DBFreeUcdvar(uv);
    return 0;
    }

  // Set cell centering info
  if (uv->centering == DB_ZONECENT) cellCentered = 1;
  else cellCentered = 0;

  DBFreeUcdvar(uv);
  return array;
}

//----------------------------------------------------------------------------

vtkDataArray *vtkSiloReader::GetUcdScalarVar(DBucdvar * uv)
{

  if (uv == NULL)
    {
    vtkErrorMacro("Given null pointer.");
    return 0;
    }

    vtkFloatArray   *scalars = vtkFloatArray::New();

    //
    // Handle stripping out values for zone types we don't understand
    //
    bool arrayWasRemapped = false;
/*  FIXME - handle skipped zones!
    if (uv->centering == DB_ZONECENT && metadata != NULL)
    {
        string meshName = metadata->MeshForVar(tvn);
        vector<int> zonesRangesToSkip = arbMeshZoneRangesToSkip[meshName];
        if (zonesRangesToSkip.size() > 0)
        {
            int numSkipped = ComputeNumZonesSkipped(zonesRangesToSkip);

            scalars->SetNumberOfTuples(uv->nels - numSkipped);
            float *ptr = (float *) scalars->GetVoidPointer(0);
            RemoveValuesForSkippedZones(zonesRangesToSkip,
                uv->vals[0], uv->nels, ptr);
            arrayWasRemapped = true;
        }
    }
*/

    //
    // Populate the variable as we normally would.
    // This assumes it is a scalar variable.
    //
    if (arrayWasRemapped == false)
    {
        scalars->SetNumberOfTuples(uv->nels);
        float        *ptr     = (float *) scalars->GetVoidPointer(0);
        memcpy(ptr, uv->vals[0], sizeof(float)*uv->nels);
    }

/*
    if (uv->mixvals != NULL && uv->mixvals[0] != NULL)
    {
        avtMixedVariable *mv = new avtMixedVariable(uv->mixvals[0], uv->mixlen,
                                                    tvn);
        void_ref_ptr vr = void_ref_ptr(mv, avtMixedVariable::Destruct);
        cache->CacheVoidRef(tvn, AUXILIARY_DATA_MIXED_VARIABLE, timestep,
                            domain, vr);
    }
*/

    return scalars;
}

//----------------------------------------------------------------------------

vtkDataArray * vtkSiloReader::GetUcdVectorVar(DBucdvar * uv)
{

  if (uv == NULL)
    {
    vtkErrorMacro("Given null pointer.");
    return 0;
    }

    vtkFloatArray   *vectors = vtkFloatArray::New();
    vectors->SetNumberOfComponents(3);

    //
    // Handle cases where we need to remove values for zone types we don't
    // understand
    //
    float *vals[3];
    float ** uvvals = reinterpret_cast<float **>(uv->vals);
    vals[0] = uvvals[0];
    vals[1] = uvvals[1];
    if (uv->nvals == 3)
       vals[2] = uvvals[2];
    int numSkipped = 0;

/*  FIXME - handled skipped zones
    if (uv->centering == DB_ZONECENT && metadata != NULL)
    {
        string meshName = metadata->MeshForVar(tvn);
        vector<int> zonesRangesToSkip = arbMeshZoneRangesToSkip[meshName];
        if (zonesRangesToSkip.size() > 0)
        {
            numSkipped = ComputeNumZonesSkipped(zonesRangesToSkip);
            vals[0] = new float[uv->nels - numSkipped];
            vals[1] = new float[uv->nels - numSkipped];
            if (uv->nvals == 3)
                vals[2] = new float[uv->nels - numSkipped];

            RemoveValuesForSkippedZones(zonesRangesToSkip,
                uv->vals[0], uv->nels, vals[0]);
            RemoveValuesForSkippedZones(zonesRangesToSkip,
                uv->vals[1], uv->nels, vals[1]);
            if (uv->nvals == 3)
                RemoveValuesForSkippedZones(zonesRangesToSkip,
                    uv->vals[2], uv->nels, vals[2]);
        }
    }
*/
    vectors->SetNumberOfTuples(uv->nels - numSkipped);
    for (int i = 0 ; i < uv->nels - numSkipped; i++)
    {
        float v3 = (uv->nvals == 3 ? vals[2][i] : 0.);
        vectors->SetTuple3(i, vals[0][i], vals[1][i], v3);
    }

    if (vals[0] != uv->vals[0])
    {
        delete [] vals[0];
        delete [] vals[1];
        if (uv->nvals == 3)
            delete [] vals[2];
    }

    return vectors;
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Method: vtkSiloReader::GetUnstructuredMesh
//
//  Purpose:
//      Creates a VTK unstructured grid from a Silo unstructured mesh.
//
//  Arguments:
//      dbfile     A handle to the file this variable lives in.
//      mn         The name of the mesh.
//      domain     The domain we are operating on.
//      mesh       The unqualified name of the mesh (for caching purposes).
//
//  Returns:      A VTK unstructured grid created from mn.
// ****************************************************************************
vtkDataSet *vtkSiloReader::GetUnstructuredMesh(DBfile *dbfile, const char *mn)
{

    if (mn == 0)
      return NULL;

    //
    // It's ridiculous, but Silo does not have all of the `const's in their
    // library, so let's cast it away.
    //
    char *meshname  = const_cast<char *>(mn);

    //
    // Get the Silo construct.
    //
    DBucdmesh  *um = DBGetUcdmesh(dbfile, meshname);
    if (um == NULL)
      {
      vtkErrorMacro("Could not find mesh: " << meshname);
      return 0;
      }

    //
    // Populate the coordinates.  Put in 3D points with z=0 if the mesh is 2D.
    //
    vtkPoints            *points  = vtkPoints::New();
    points->SetNumberOfPoints(um->nnodes);
    float *pts = (float *) points->GetVoidPointer(0);
    int nnodes = um->nnodes;

    bool dim3 = (um->coords[2] != NULL ? true : false);
    float *tmp = pts;

    float ** umcoords = reinterpret_cast<float **>(um->coords);
    const float *coords0 = umcoords[0];
    const float *coords1 = umcoords[1];
    if (dim3)
    {
        const float *coords2 = umcoords[2];
        for (int i = 0 ; i < nnodes ; i++)
        {
            *tmp++ = *coords0++;
            *tmp++ = *coords1++;
            *tmp++ = *coords2++;
        }
    }
    else
    {
        for (int i = 0 ; i < nnodes ; i++)
        {
            *tmp++ = *coords0++;
            *tmp++ = *coords1++;
            *tmp++ = 0.;
        }
    }

    //
    // We already got the facelist read in free of charge.  Let's use it.
    // This is done before constructing the connectivity because this is used
    // in place of the connectivity in some places.
    //
  //  avtFacelist *fl = NULL;
    if (um->faces != NULL && um->ndims == 3)
      {
      vtkDebugMacro("UCD mesh uses facelists: " << mn);
     /*
        DBfacelist *sfl = um->faces;
        fl = new avtFacelist(sfl->nodelist, sfl->lnodelist,
                             sfl->nshapes, sfl->shapecnt, sfl->shapesize,
                             sfl->zoneno, sfl->origin);
        void_ref_ptr vr = void_ref_ptr(fl, avtFacelist::Destruct);
        cache->CacheVoidRef(mesh, AUXILIARY_DATA_EXTERNAL_FACELIST, timestep,
                            domain, vr);
    */
      }

/*
    //
    // If we have global node ids, set them up and cache 'em
    //
    if (um->gnodeno != NULL)
    {
        //
        // Create a vtkInt array whose contents are the actual gnodeno data
        //
        vtkIntArray *arr = vtkIntArray::New();
        arr->SetNumberOfComponents(1);
        arr->SetNumberOfTuples(nnodes);
        int *ptr = arr->GetPointer(0);
        memcpy(ptr, um->gnodeno, nnodes*sizeof(int));

        //
        // Cache this VTK object but in the VoidRefCache, not the VTK cache
        // so that it can be obtained through the GetAuxiliaryData call
        //
        void_ref_ptr vr = void_ref_ptr(arr, avtVariableCache::DestructVTKObject);
        cache->CacheVoidRef(mesh, AUXILIARY_DATA_GLOBAL_NODE_IDS, timestep,
                            domain, vr);
    }
*/
    // Visit comment-
    // Some ucdmeshes uses facelists instead of zonelists.  I think this is
    // freakish behavior and should not be supported, but if there are files
    // this way then we have to honor that.
    //
    vtkDataSet *rv = NULL;
    if (um->zones != NULL)
    {
        vtkUnstructuredGrid  *ugrid = vtkUnstructuredGrid::New();
        ugrid->SetPoints(points);
        //vtkstd::vector<int> zoneRangesToSkip;

        ReadInConnectivity(ugrid, um->zones, um->zones->origin /*, zoneRangesToSkip*/);
        /*if (zoneRangesToSkip.size() > 0)
        {
            // squirl away knowledge of the zones we've removed
            arbMeshZoneRangesToSkip[mesh] = zoneRangesToSkip;
            int numSkipped = ComputeNumZonesSkipped(zoneRangesToSkip);

            // Issue a warning message about having skipped some zones
            char msg[1024];
            sprintf(msg, "\nIn reading mesh, \"%s\", VisIt encountered "
                "%d arbitrary polyhedral zones accounting for %3d %% "
                "of the zones in the mesh. Those zones have been removed. "
                "Future versions of VisIt will be able to display these "
                "zones. However, the current version cannot.", mesh,
                numSkipped, 100 * numSkipped / um->zones->nzones);
            avtCallback::IssueWarning(msg);
        }*/
        rv = ugrid;

       /* if (um->zones->gzoneno != NULL)
        {
            //
            // Create a vtkInt array whose contents are the actual gzoneno data
            //
            vtkIntArray *arr = vtkIntArray::New();
            arr->SetNumberOfComponents(1);
            arr->SetNumberOfTuples(um->zones->nzones);
            int *ptr = arr->GetPointer(0);
            memcpy(ptr, um->zones->gzoneno, um->zones->nzones*sizeof(int));

            //
            // Cache this VTK object but in the VoidRefCache, not the VTK cache
            // so that it can be obtained through the GetAuxiliaryData call
            //
            void_ref_ptr vr = void_ref_ptr(arr, avtVariableCache::DestructVTKObject);
            cache->CacheVoidRef(mesh, AUXILIARY_DATA_GLOBAL_ZONE_IDS, timestep,
                                domain, vr);
        }*/

    }
    /*else if (fl != NULL)
    {
        vtkPolyData *pd = vtkPolyData::New();
        fl->CalcFacelistFromPoints(points, pd);
        rv = pd;
    }*/
    else
      {
      vtkErrorMacro("UCD mesh has no zonelists: " << mn);
      }

    points->Delete();
    DBFreeUcdmesh(um);

    return rv;
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Method: avtSiloFileFormat::ReadInConnectivity
//
//  Purpose:
//      Reads in the connectivity array.  Also creates ghost zone information.
// ****************************************************************************
void vtkSiloReader::ReadInConnectivity(vtkUnstructuredGrid *ugrid, DBzonelist *zl, int origin)
{
    int   i, j, k;

    //
    // Tell the ugrid how many cells it will have.
    //
    int  numCells = 0;
    int  totalSize = 0;
    for (i = 0 ; i < zl->nshapes ; i++)
    {
        int vtk_zonetype = SiloZoneTypeToVTKZoneType(zl->shapetype[i]);
        if (vtk_zonetype != -2) // don't include arb. polyhedra
        {
            numCells += zl->shapecnt[i];
            totalSize += zl->shapecnt[i] * (zl->shapesize[i]+1);
        }
    }

    //
    // Tell the ugrid what its zones are.
    //
    int *nodelist = zl->nodelist;

    vtkIdTypeArray *nlist = vtkIdTypeArray::New();
    nlist->SetNumberOfValues(totalSize);
    vtkIdType *nl = nlist->GetPointer(0);

    vtkUnsignedCharArray *cellTypes = vtkUnsignedCharArray::New();
    cellTypes->SetNumberOfValues(numCells);
    unsigned char *ct = cellTypes->GetPointer(0);

    vtkIdTypeArray *cellLocations = vtkIdTypeArray::New();
    cellLocations->SetNumberOfValues(numCells);
    vtkIdType *cl = cellLocations->GetPointer(0);

    int zoneIndex = 0;
    int currentIndex = 0;
    bool mustResize = false;
    int minIndexOffset = 0;
    int maxIndexOffset = 0;
    for (i = 0 ; i < zl->nshapes ; i++)
    {
        const int shapecnt = zl->shapecnt[i];
        const int shapesize = zl->shapesize[i];

        int vtk_zonetype = SiloZoneTypeToVTKZoneType(zl->shapetype[i]);
        int effective_vtk_zonetype = vtk_zonetype;
        int effective_shapesize = shapesize;

        //
        // WARNING this code block has not been
        // tested!  I'm not convinced this is a safe
        // way to exit.  In this place, Visit throws
        // an exception.
        if (vtk_zonetype < 0 && vtk_zonetype != -2)
          {
          vtkErrorMacro("Zonetype is invalid.\n");
          nlist->Delete();
          cellTypes->Delete();
          cellLocations->Delete();
          return;
          //EXCEPTION1(InvalidZoneTypeException, zl->shapetype[i]);
          }

        //
        // Some users store out quads as hexahedrons -- they store quad
        // (a,b,c,d) as hex (a,b,c,d,a,b,c,d).  Unfortunately, we have
        // to detect this and account for it.  I think it is safe to
        // assume that if the first hex is that way, they all are.
        // Similarly, if the first hex is not that way, none of them are.
        //
        if (vtk_zonetype == VTK_HEXAHEDRON)
        {
            if (shapecnt > 0) // Make sure there is at least 1 hex.
            {
                if ((nodelist[0] == nodelist[4]) &&
                    (nodelist[1] == nodelist[5]) &&
                    (nodelist[2] == nodelist[6]) &&
                    (nodelist[3] == nodelist[7]))
                {
                    vtk_zonetype = -1;
                    effective_vtk_zonetype = VTK_QUAD;
                    effective_shapesize = 4;
                    mustResize = true;
                }
            }
        }

        //
        // "Handle" arbitrary polyhedra by skipping over them
        //
        if (vtk_zonetype == -2) // DB_ZONETYPE_POLYHEDRON
        {
              printf("Warning, skipping arbitrary polyhedra zones.\n");
           // zoneRangesToSkip.push_back(zoneIndex);
           // zoneRangesToSkip.push_back(zoneIndex + shapecnt - 1);

            // keep track of adjustments we'll need to make to
            // min/max offsets for ghosting
            if (zoneIndex < zl->min_index)
            {
               if (zoneIndex + shapecnt < zl->min_index)
               {
                   minIndexOffset += shapecnt;
                   maxIndexOffset += shapecnt;
               }
               else
               {
                   minIndexOffset += (zl->min_index - zoneIndex);
                   maxIndexOffset += (zl->min_index - zoneIndex);
               }
            }
            else if (zoneIndex + shapecnt <= zl->max_index)
            {
               maxIndexOffset += shapecnt;
            }
            else if (zoneIndex + shapecnt > zl->max_index)
            {
               maxIndexOffset += (zl->max_index - zoneIndex + 1);
            }

            nodelist += shapesize;
            zoneIndex += shapecnt;
        }
        else
        {
            bool tetsAreInverted = false;
            bool firstTet = true;
            for (j = 0 ; j < shapecnt ; j++)
            {
                *ct++ = effective_vtk_zonetype;
                *cl++ = currentIndex;

                if (vtk_zonetype != VTK_WEDGE &&
                    vtk_zonetype != VTK_PYRAMID &&
                    vtk_zonetype != VTK_TETRA &&
                    vtk_zonetype != -1)
                {
                    *nl++ = shapesize;
                    for (k = 0 ; k < shapesize ; k++)
                        *nl++ = *(nodelist+k) - origin;
                }
                else if (vtk_zonetype == VTK_WEDGE)
                {
                    *nl++ = 6;

                    vtkIdType vtk_wedge[6];
                    TranslateSiloWedgeToVTKWedge(nodelist, vtk_wedge);
                    for (k = 0 ; k < 6 ; k++)
                    {
                        *nl++ = vtk_wedge[k]-origin;
                    }
                }
                else if (vtk_zonetype == VTK_PYRAMID)
                {
                    *nl++ = 5;

                    vtkIdType vtk_pyramid[5];
                    TranslateSiloPyramidToVTKPyramid(nodelist, vtk_pyramid);
                    for (k = 0 ; k < 5 ; k++)
                    {
                        *nl++ = vtk_pyramid[k]-origin;
                    }
                }
                else if (vtk_zonetype == VTK_TETRA)
                {
                    *nl++ = 4;

                    if (firstTet)
                    {
                        firstTet = false;
                        tetsAreInverted = TetsAreInverted(nodelist, ugrid);
                        static bool haveIssuedWarning = false;
                        if (tetsAreInverted)
                        {
                            haveIssuedWarning = true;
                            vtkDebugMacro( "An examination of the first tet "
                                "element in this mesh indicates that the node order is "
                                "inverted from Silo's standard conventions. All tets are "
                                "being automatically re-ordered.\n"
                                "Further messages of this issue will be suppressed.");
                        }
                    }

                    vtkIdType vtk_tetra[4];
                    if (tetsAreInverted)
                    {
                        for (k = 0 ; k < 4 ; k++)
                            vtk_tetra[k] = nodelist[k];
                    }
                    else
                    {
                        TranslateSiloTetrahedronToVTKTetrahedron(nodelist,
                                                                 vtk_tetra);
                    }

                    for (k = 0 ; k < 4 ; k++)
                    {
                        *nl++ = vtk_tetra[k]-origin;
                    }
                }
                else if (vtk_zonetype == -1)
                {
                    *nl++ = 4;
                    for (k = 0 ; k < 4 ; k++)
                        *nl++ = *(nodelist+k);
                }

                nodelist += shapesize;
                currentIndex += effective_shapesize+1;
                zoneIndex++;
            }
        }
    }

    //
    // This only comes up when somebody says they have hexahedrons, but they
    // actually have quadrilaterals.  In that case, we have allocated too much
    // memory and need to resize.  If we don't, VTK will get confused.
    //
    if (mustResize)
    {
        vtkIdTypeArray *nlist2 = vtkIdTypeArray::New();
        vtkIdType *nl_orig = nlist->GetPointer(0);
        int nvalues = nl-nl_orig;
        nlist2->SetNumberOfValues(nvalues);
        vtkIdType *nl2 = nlist2->GetPointer(0);
        memcpy(nl2, nl_orig, nvalues*sizeof(vtkIdType));
        nlist->Delete();
        nlist = nlist2;
    }

    vtkCellArray *cells = vtkCellArray::New();
    cells->SetCells(numCells, nlist);
    nlist->Delete();

    ugrid->SetCells(cellTypes, cellLocations, cells);
    cellTypes->Delete();
    cellLocations->Delete();
    cells->Delete();

    //
    //  Tell the ugrid which of its zones are real (avtGhostZone = 0),
    //  which are ghost (avtGhostZone = 1), but only create the ghost
    //  zones array if ghost zones are actually present.
    //
    const int first = zl->min_index - minIndexOffset;  // where the real zones start
    const int last = zl->max_index - maxIndexOffset;   // where the real zones end

    if (first == 0 && last == 0  && numCells > 27)
    {
       vtkDebugMacro( << "Cannot tell if ghost zones are present"
              << " because min_index & max_index are both zero!");
    }
    else if (first < 0 || first >= numCells ||
             last  < 0 || last  >= numCells)
    {
       // bad min or max index
       vtkDebugMacro( << "Invalid min/max index for determining ghost zones:  "
              << "\n\tnumCells: " << numCells
              << "\n\tmin_index: " << zl->min_index
              << "\n\tmax_index: " << zl->max_index);
    }
    else if (first != 0 || last != numCells -1)
    {
        //
        // We now know that ghost zones are present.
        //
        vtkDebugMacro( << "Creating ghost zones, real zones are indexed"
               << " from " << first << " to " << last
               << " of " << numCells << " Cells.");

        //
        //  Give the array the proper name so that other vtk classes will
        //  recognize these as ghost levels.
        //
        vtkUnsignedCharArray *ghostZones = vtkUnsignedCharArray::New();
        ghostZones->SetName("avtGhostZones");
        ghostZones->SetNumberOfTuples(numCells);
        unsigned char *tmp = ghostZones->GetPointer(0);
        for (i = 0; i < first; i++)
        {
            //
            //  ghostZones at the begining of the zone list
            //
            unsigned char val = 0;
           // avtGhostData::AddGhostZoneType(val,
           //                               DUPLICATED_ZONE_INTERNAL_TO_PROBLEM);
            *tmp++ = val;
        }
        for (i = first; i <= last; i++)
        {
            //
            // real zones
            //
            *tmp++ = 0;
        }
        for (i = last+1; i < numCells; i++)
        {
            //
            //  ghostZones at the end of the zone list
            //
            unsigned char val = 0;
           // avtGhostData::AddGhostZoneType(val,
           //                               DUPLICATED_ZONE_INTERNAL_TO_PROBLEM);
            *tmp++ = val;
        }
        ugrid->GetCellData()->AddArray(ghostZones);
        ghostZones->Delete();
        ugrid->SetUpdateGhostLevel(0);
    }
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Function: SiloZoneTypeToVTKZoneType
//
//  Purpose:
//      Converts a zone type in Silo to a zone type in VTK.
//
//  Arguments:
//      zonetype      The zone type in Silo.
//
//  Returns:     The zone type in VTK.
// ****************************************************************************
int SiloZoneTypeToVTKZoneType(int zonetype)
{
    int  vtk_zonetype = -1;

    switch (zonetype)
    {
      case DB_ZONETYPE_POLYGON:
        vtk_zonetype = VTK_POLYGON;
        break;
      case DB_ZONETYPE_TRIANGLE:
        vtk_zonetype = VTK_TRIANGLE;
        break;
      case DB_ZONETYPE_QUAD:
        vtk_zonetype = VTK_QUAD;
        break;
      case DB_ZONETYPE_POLYHEDRON:
        vtk_zonetype = -2;
        break;
      case DB_ZONETYPE_TET:
        vtk_zonetype = VTK_TETRA;
        break;
      case DB_ZONETYPE_PYRAMID:
        vtk_zonetype = VTK_PYRAMID;
        break;
      case DB_ZONETYPE_PRISM:
        vtk_zonetype = VTK_WEDGE;
        break;
      case DB_ZONETYPE_HEX:
        vtk_zonetype = VTK_HEXAHEDRON;
        break;
      case DB_ZONETYPE_BEAM:
        vtk_zonetype = VTK_LINE;
        break;
    }

    return vtk_zonetype;
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Function: TranslateSiloWedgeToVTKWedge
//
//  Purpose:
//      The silo and VTK wedges are stored differently; translate between them.
//
//  Arguments:
//      siloWedge     A list of nodes from a Silo node list.
//      vtkWedge      The list of nodes in VTK ordering.
// ****************************************************************************

void TranslateSiloWedgeToVTKWedge(const int *siloWedge, vtkIdType vtkWedge[6])
{
    //
    // The Silo wedge stores the four base nodes as 0, 1, 2, 3 and the two
    // top nodes as 4, 5.  The VTK wedge stores them as two triangles.  When
    // getting the exact translation, it is useful to look at the face lists
    // and edge lists in vtkWedge.cxx.
    //
    vtkWedge[0] = siloWedge[2];
    vtkWedge[1] = siloWedge[1];
    vtkWedge[2] = siloWedge[5];
    vtkWedge[3] = siloWedge[3];
    vtkWedge[4] = siloWedge[0];
    vtkWedge[5] = siloWedge[4];
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Function: TranslateSiloPyramidToVTKPyramid
//
//  Purpose:
//    The silo and VTK pyramids are stored differently; translate between them.
//
//  Arguments:
//    siloPyramid     A list of nodes from a Silo node list.
//    vtkPyramid      The list of nodes in VTK ordering.
// ****************************************************************************
void TranslateSiloPyramidToVTKPyramid(const int *siloPyramid, vtkIdType vtkPyramid[5])
{
    //
    // The Silo pyramid stores the four base nodes as 0, 1, 2, 3 in
    // opposite order from the VTK wedge. When getting the exact translation,
    // it is useful to look at the face lists and edge lists in
    // vtkPyramid.cxx.
    //
    vtkPyramid[0] = siloPyramid[0];
    vtkPyramid[1] = siloPyramid[3];
    vtkPyramid[2] = siloPyramid[2];
    vtkPyramid[3] = siloPyramid[1];
    vtkPyramid[4] = siloPyramid[4];
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Function: TranslateSiloTetrahedronToVTKTetrahedron
//
//  Purpose:
//    The silo and VTK tetrahedrons are stored differently; translate between
//     them.
//
//  Arguments:
//    siloTetrahedron     A list of nodes from a Silo node list.
//    vtkTetrahedron      The list of nodes in VTK ordering.
// ****************************************************************************
void TranslateSiloTetrahedronToVTKTetrahedron(const int *siloTetrahedron,
                                         vtkIdType vtkTetrahedron[4])
{
    //
    // The Silo and VTK tetrahedrons are inverted.
    //
    vtkTetrahedron[0] = siloTetrahedron[1];
    vtkTetrahedron[1] = siloTetrahedron[0];
    vtkTetrahedron[2] = siloTetrahedron[2];
    vtkTetrahedron[3] = siloTetrahedron[3];
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Function: TetsAreInverted
//
//  Purpose: Determine if Tets in Silo are inverted from Silo's Normal ordering
// ****************************************************************************
bool TetsAreInverted(const int *siloTetrahedron, vtkUnstructuredGrid *ugrid)
{
    //
    // initialize set of 4 points of tet
    //
    float *pts = (float *) ugrid->GetPoints()->GetVoidPointer(0);
    float p[4][3];
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 3; j++)
            p[i][j] = pts[3*siloTetrahedron[i] + j];
    }

    //
    // Compute a vector normal to plane of first 3 points
    //
    float n1[3] = {p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2]};
    float n2[3] = {p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]};
    float n1Xn2[3] = {  n1[1]*n2[2] - n1[2]*n2[1],
                      -(n1[0]*n2[2] - n1[2]*n2[0]),
                        n1[0]*n2[1] - n1[1]*n2[0]};

    //
    // Compute a dot-product of normal with a vector to the 4th point.
    // If the tet is specified as Silo normally expects it, this dot
    // product should be negative. If it is not negative, then tets
    // are inverted
    //
    float n3[3] = {p[3][0] - p[0][0], p[3][1] - p[0][1], p[3][2] - p[0][2]};
    float n3Dotn1Xn2 = n3[0]*n1Xn2[0] + n3[1]*n1Xn2[1] + n3[2]*n1Xn2[2];

    if (n3Dotn1Xn2 > 0)
        return true;
    else
        return false;
}

//----------------------------------------------------------------------------

void PrintErrorMessage(char *msg)
{
    if (msg)
      {
      vtkGenericWarningMacro("The following Silo error occured: " << msg);
      }
    else
      {
      vtkGenericWarningMacro("A Silo error occurred, but the Silo library did not generate an error message.");
      }
}

//----------------------------------------------------------------------------

//#define MyDebug(X) printf X;
#define MyDebug(X)

void vtkSiloReader::ReadDir(DBfile *dbfile, const char *dirname)
{


  MyDebug(("Dir: %s\n", dirname));
  int i;
  DBtoc *toc = DBGetToc(dbfile);
  if (toc == NULL)
    {
    vtkErrorMacro("Silo TOC is null.");
    return;
    }

  // Copy silo TOC

  // Meshes
  int nmultimesh = toc->nmultimesh;
  char **multimesh_names = new char*[nmultimesh];
  for (i = 0 ; i < nmultimesh ; i++)
    {
    multimesh_names[i] = new char[strlen(toc->multimesh_names[i])+1];
    strcpy(multimesh_names[i], toc->multimesh_names[i]);
    MyDebug(("  multimesh name: %s\n", multimesh_names[i]));
    }
  int nqmesh = toc->nqmesh;
  char **qmesh_names = new char*[nqmesh];
  for (i = 0 ; i < nqmesh ; i++)
    {
    qmesh_names[i] = new char[strlen(toc->qmesh_names[i])+1];
    strcpy(qmesh_names[i], toc->qmesh_names[i]);
    MyDebug(("  qmesh name: %s\n", qmesh_names[i]));
    }
  int nucdmesh = toc->nucdmesh;
  char **ucdmesh_names = new char*[nucdmesh];
  for (i = 0 ; i < nucdmesh ; i++)
    {
    ucdmesh_names[i] = new char[strlen(toc->ucdmesh_names[i])+1];
    strcpy(ucdmesh_names[i], toc->ucdmesh_names[i]);
    MyDebug(("  ucdmesh name: %s\n", ucdmesh_names[i]));
    }
  int nptmesh = toc->nptmesh;
  char **ptmesh_names = new char*[nptmesh];
  for (i = 0 ; i < nptmesh ; i++)
    {
    ptmesh_names[i] = new char[strlen(toc->ptmesh_names[i])+1];
    strcpy(ptmesh_names[i], toc->ptmesh_names[i]);
    MyDebug(("  ptmesh name: %s\n", ptmesh_names[i]));
    }
  int ncurves = toc->ncurve;
  char **curve_names = new char*[ncurves];
  for (i = 0 ; i < ncurves; i++)
    {
    curve_names[i] = new char[strlen(toc->curve_names[i])+1];
    strcpy(curve_names[i], toc->curve_names[i]);
    MyDebug(("  curve name: %s\n", curve_names[i]));
    }

  // Vars
  int nmultivar = toc->nmultivar;
  char **multivar_names = new char*[nmultivar];
  for (i = 0 ; i < nmultivar ; i++)
    {
    multivar_names[i] = new char[strlen(toc->multivar_names[i])+1];
    strcpy(multivar_names[i], toc->multivar_names[i]);
    MyDebug(("  multivar name: %s\n", multivar_names[i]));
    }
  int nqvar = toc->nqvar;
  char **qvar_names = new char*[nqvar];
  for (i = 0 ; i < nqvar ; i++)
    {
    qvar_names[i] = new char[strlen(toc->qvar_names[i])+1];
    strcpy(qvar_names[i], toc->qvar_names[i]);
    MyDebug(("  qvar name: %s\n", qvar_names[i]));
    }
  int nucdvar = toc->nucdvar;
  char **ucdvar_names = new char*[nucdvar];
  for (i = 0 ; i < nucdvar ; i++)
    {
    ucdvar_names[i] = new char[strlen(toc->ucdvar_names[i])+1];
    strcpy(ucdvar_names[i], toc->ucdvar_names[i]);
    MyDebug(("  ucdvar name: %s\n", ucdvar_names[i]));
    }
  int nptvar = toc->nptvar;
  char **ptvar_names = new char*[nptvar];
  for (i = 0 ; i < nptvar ; i++)
    {
    ptvar_names[i] = new char[strlen(toc->ptvar_names[i])+1];
    strcpy(ptvar_names[i], toc->ptvar_names[i]);
    MyDebug(("  ptvar name: %s\n", ptvar_names[i]));
    }

  // Materials
  int nmat = toc->nmat;
  char **mat_names = new char*[nmat];
  for (i = 0 ; i < nmat ; i++)
    {
    mat_names[i] = new char[strlen(toc->mat_names[i])+1];
    strcpy(mat_names[i], toc->mat_names[i]);
    MyDebug(("  mat name: %s\n", mat_names[i]));
    }
  int nmultimat = toc->nmultimat;
  char **multimat_names = new char*[nmultimat];
  for (i = 0 ; i < nmultimat ; i++)
    {
    multimat_names[i] = new char[strlen(toc->multimat_names[i])+1];
    strcpy(multimat_names[i], toc->multimat_names[i]);
    MyDebug(("  multimat name: %s\n", multimat_names[i]));
    }

  // Species
  int nmatspecies      = toc->nmatspecies;
  char **matspecies_names = new char*[nmatspecies];
  for (i = 0 ; i < nmatspecies ; i++)
    {
    matspecies_names[i] = new char[strlen(toc->matspecies_names[i])+1];
    strcpy(matspecies_names[i], toc->matspecies_names[i]);
    MyDebug(("  matspecies name: %s\n", matspecies_names[i]));
    }
  int nmultimatspecies = toc->nmultimatspecies;
  char **multimatspecies_names = new char*[nmultimatspecies];
  for (i = 0 ; i < nmultimatspecies ; i++)
    {
    multimatspecies_names[i]
               = new char[strlen(toc->multimatspecies_names[i])+1];
    strcpy(multimatspecies_names[i], toc->multimatspecies_names[i]);
    MyDebug(("  multimat species name: %s\n", multimatspecies_names[i]));
    }

  // Dirs
  int ndir = toc->ndir;
  char **dir_names = new char*[ndir];
  for (i = 0 ; i < ndir ; i++)
    {
    dir_names[i] = new char[strlen(toc->dir_names[i])+1];
    strcpy(dir_names[i], toc->dir_names[i]);
    MyDebug(("  dir name: %s\n", dir_names[i]));
    }


  /////////////////////////////
  // Now read the TOC

  //
  // Multi-meshes
  //
  for (i = 0 ; i < nmultimesh ; i++)
    {
    char   *realvar;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(multimesh_names[i], correctFile, 0, realvar);
    DBmultimesh *mm = DBGetMultimesh(correctFile, realvar);

    if (mm == NULL)
      {
      continue;
      }

    RegisterDomainDirs(mm->meshnames, mm->nblocks, dirname);

    SiloMesh * multiMesh = this->TOC->AddMesh(multimesh_names[i]);


    for (int k = 0; k < mm->nblocks; ++k)
      {
      if ( !strcmp(mm->meshnames[k], "EMPTY") )
        {
        continue;
        }
      int meshType = this->GetMeshType(dbfile, mm->meshnames[k]);
      SiloMeshBlock * meshBlock = this->TOC->AddMeshBlock(mm->meshnames[k], multiMesh);
      meshBlock->Type = meshType;
      }

    DBFreeMultimesh(mm);
    }

  //
  // Quad-meshes
  //
  for (i = 0 ; i < nqmesh ; i++)
    {
    char   *realvar;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(qmesh_names[i], correctFile, 0, realvar);
    DBquadmesh *qm = DBGetQuadmesh(correctFile, realvar);

    if (qm == NULL)
      {
      continue;
      }

    char *name_w_dir = GenerateName(dirname, qmesh_names[i]);

    // Store the quadmesh name associated with mesh type
    SiloMesh * mesh = this->TOC->AddMesh(name_w_dir);
    SiloMeshBlock * meshBlock = this->TOC->AddMeshBlock(name_w_dir, mesh);
    meshBlock->Type = DB_QUADMESH;

    delete [] name_w_dir;
    DBFreeQuadmesh(qm);
    }

  //
  // Unstructured meshes
  //
  for (i = 0 ; i < nucdmesh ; i++)
    {
    char   *realvar;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(ucdmesh_names[i], correctFile, 0, realvar);
    DBucdmesh *um = DBGetUcdmesh(correctFile, realvar);

    if (um == NULL)
      {
      continue;
      }

    char *name_w_dir = GenerateName(dirname, ucdmesh_names[i]);

    // Store the ucdmesh name associated with mesh type
    SiloMesh * mesh = this->TOC->AddMesh(name_w_dir);
    SiloMeshBlock * meshBlock = this->TOC->AddMeshBlock(name_w_dir, mesh);
    meshBlock->Type = DB_UCDMESH;

    delete [] name_w_dir;
    DBFreeUcdmesh(um);
    }

  //
  // Point meshes
  //
  for (i = 0 ; i < nptmesh ; i++)
    {
    char   *realvar;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(ptmesh_names[i], correctFile, 0, realvar);
    DBpointmesh *pm = DBGetPointmesh(correctFile, realvar);

    if (pm == NULL)
      {
      continue;
      }

    char *name_w_dir = GenerateName(dirname, ptmesh_names[i]);

    // Store the pointmesh name associated with mesh type
    SiloMesh * mesh = this->TOC->AddMesh(name_w_dir);
    SiloMeshBlock * meshBlock = this->TOC->AddMeshBlock(name_w_dir, mesh);
    meshBlock->Type = DB_POINTMESH;

    delete [] name_w_dir;
    DBFreePointmesh(pm);
    }

  //
  // Curves
  //
  for (i = 0 ; i < ncurves; i++)
    {
    char   *realvar;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(curve_names[i], correctFile, 0, realvar);
    DBcurve *cur = DBGetCurve(correctFile, realvar);

    if (cur == NULL)
      {
      continue;
      }

    char *name_w_dir = GenerateName(dirname, curve_names[i]);

    // Store the curve name associated with type
    SiloMesh * mesh = this->TOC->AddMesh(name_w_dir);
    SiloMeshBlock * meshBlock = this->TOC->AddMeshBlock(name_w_dir, mesh);
    meshBlock->Type = DB_CURVE;

    delete [] name_w_dir;
    DBFreeCurve(cur);
    }

  //
  // Multi-vars
  //
  for (i = 0 ; i < nmultivar ; i++)
    {
    char   *realvar = NULL;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(multivar_names[i], correctFile, 0, realvar);
    DBmultivar *mv = DBGetMultivar(correctFile, realvar);
    if (mv == NULL)
      {
      continue;
      }

    RegisterDomainDirs(mv->varnames, mv->nvars, dirname);

    for (int k = 0; k < mv->nvars; ++k)
      {
      if ( !strcmp(mv->varnames[k], "EMPTY") )
        {
        continue;
        }

      char meshForVar[256];
      char   *dirvar;
      DBfile *correctFile = dbfile;
      DetermineFileAndDirectory( mv->varnames[k], correctFile, 0, dirvar);
      int rv = DBInqMeshname(correctFile, dirvar, meshForVar);
      if (rv < 0)
        {
        vtkErrorMacro("Cannot find mesh for variable " << mv->varnames[k]);
        continue;
        }

      char meshForVarWithFile[1024];
      GetRelativeVarName(mv->varnames[k], meshForVar, meshForVarWithFile);

      SiloMeshBlock * meshBlock = this->TOC->GetMeshBlock(meshForVarWithFile);
      if (!meshBlock)
        {
        vtkErrorMacro("Error adding var " << mv->varnames[k] << " to nonexisting mesh block" << meshForVarWithFile);
        }
      else
        {
        meshBlock->Variables.push_back(mv->varnames[k]);
        }
      }

    DBFreeMultivar(mv);
    }

  //
  // Quadvars
  //
  for (i = 0 ; i < nqvar ; i++)
    {
    char   *realvar = NULL;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(qvar_names[i], correctFile, 0, realvar);
    DBquadvar *qv = DBGetQuadvar(correctFile, realvar);

    if (qv == NULL)
      {
      continue;
      }

    char meshname[256];
    DBInqMeshname(correctFile, realvar, meshname);

    char *name_w_dir = GenerateName(dirname, qvar_names[i]);
    char *meshname_w_dir = GenerateName(dirname, meshname);

    SiloMeshBlock * meshBlock = this->TOC->GetMeshBlock(meshname_w_dir);
    if (!meshBlock)
      {
      vtkErrorMacro("Error adding var " << name_w_dir << " to nonexisting mesh block" << meshname_w_dir);
      }
    else
      {
      meshBlock->Variables.push_back(name_w_dir);
      }

    delete [] name_w_dir;
    delete [] meshname_w_dir;
    DBFreeQuadvar(qv);
    }

  //
  // Ucdvars
  //
  for (i = 0 ; i < nucdvar ; i++)
    {
    char   *realvar = NULL;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(ucdvar_names[i], correctFile, 0, realvar);
    DBucdvar *uv = DBGetUcdvar(correctFile, realvar);
    if (uv == NULL) continue;

    char meshname[256];
    DBInqMeshname(correctFile, realvar, meshname);

    char *name_w_dir = GenerateName(dirname, ucdvar_names[i]);
    char *meshname_w_dir = GenerateName(dirname, meshname);

    SiloMeshBlock * meshBlock = this->TOC->GetMeshBlock(meshname_w_dir);
    if (!meshBlock)
      {
      vtkErrorMacro("Error adding var " << name_w_dir << " to nonexisting mesh block" << meshname_w_dir);
      }
    else
      {
      meshBlock->Variables.push_back(name_w_dir);
      }

    delete [] name_w_dir;
    delete [] meshname_w_dir;
    DBFreeUcdvar(uv);
    }

  //
  // Point vars
  //
  for (i = 0 ; i < nptvar ; i++)
    {
    char   *realvar = NULL;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(ptvar_names[i], correctFile, 0, realvar);
    DBmeshvar *pv = DBGetPointvar(correctFile, realvar);
    if (pv == NULL)
      {
      continue;
      }

    char meshname[256];
    DBInqMeshname(correctFile, realvar, meshname);

    char *name_w_dir = GenerateName(dirname, ptvar_names[i]);
    char *meshname_w_dir = GenerateName(dirname, meshname);

    SiloMeshBlock * meshBlock = this->TOC->GetMeshBlock(meshname_w_dir);
    if (!meshBlock)
      {
      vtkErrorMacro("Error adding var " << name_w_dir << " to nonexisting mesh block" << meshname_w_dir);
      }
    else
      {
      meshBlock->Variables.push_back(name_w_dir);
      }

    delete [] name_w_dir;
    delete [] meshname_w_dir;
    DBFreeMeshvar(pv);
    }

  //
  // Materials
  //
  for (i = 0 ; i < nmat ; i++)
    {
    char   *realvar = NULL;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(mat_names[i], correctFile, 0, realvar);
    DBmaterial *mat = DBGetMaterial(correctFile, realvar);
    if (mat == NULL)
      {
      continue;
      }

    char meshname[256];
    DBInqMeshname(correctFile, realvar, meshname);

    char *name_w_dir = GenerateName(dirname, mat_names[i]);
    char *meshname_w_dir = GenerateName(dirname, meshname);

    SiloMeshBlock * meshBlock = this->TOC->GetMeshBlock(meshname_w_dir);
    if (!meshBlock)
      {
      vtkErrorMacro("Error adding mat " << name_w_dir << " to nonexisting mesh block" << meshname_w_dir);
      }
    else
      {
      meshBlock->Materials.push_back(name_w_dir);
      }

    delete [] name_w_dir;
    delete [] meshname_w_dir;
    DBFreeMaterial(mat);
    }

  //
  // Multi-mats
  //
  for (i = 0 ; i < nmultimat ; i++)
    {

    char   *realvar = NULL;
    DBfile *correctFile = dbfile;

    DetermineFileAndDirectory(multimat_names[i], correctFile, 0, realvar);
    DBmultimat *mm = DBGetMultimat(correctFile, realvar);
    if (mm == NULL)
      {
      continue;
      }

    RegisterDomainDirs(mm->matnames, mm->nmats, dirname);

    for (int k = 0; k < mm->nmats; ++k)
      {
      if ( !strcmp(mm->matnames[k], "EMPTY") )
        {
        continue;
        }

      char meshForMat[256];
      char   *dirvar;
      DBfile *correctFile = dbfile;
      DetermineFileAndDirectory( mm->matnames[k], correctFile, 0, dirvar);
      int rv = DBInqMeshname(correctFile, dirvar, meshForMat);
      if (rv < 0)
        {
        vtkErrorMacro("Cannot find mesh for material " << mm->matnames[k]);
        continue;
        }

      char meshForMatWithFile[1024];
      GetRelativeVarName(mm->matnames[k], meshForMat, meshForMatWithFile);

      SiloMeshBlock * meshBlock = this->TOC->GetMeshBlock(meshForMatWithFile);
      if (!meshBlock)
        {
        vtkErrorMacro("Error adding mat " << mm->matnames[k] << " to nonexisting mesh block" << meshForMatWithFile);
        }
      else
        {
        meshBlock->Materials.push_back(mm->matnames[k]);
        }
      }

    DBFreeMultimat(mm);
    }

/*
    //
    // Species
    //
    for (i = 0 ; i < nmatspecies ; i++)
    {
        char   *realvar = NULL;
        DBfile *correctFile = dbfile;
        bool valid_var = true;
        DetermineFileAndDirectory(matspecies_names[i], correctFile, 0, realvar);

        DBmatspecies *spec = DBGetMatspecies(correctFile, realvar);
        if (spec == NULL)
        {
            valid_var = false;
            spec = DBAllocMatspecies();
        }

        char meshname[256];
        GetMeshname(dbfile, spec->matname, meshname);

        vector<int>   numSpecies;
        vector<vector<string> > speciesNames;
        for (j = 0 ; j < spec->nmat ; j++)
        {
            numSpecies.push_back(spec->nmatspec[j]);
            vector<string>  tmp_string_vector;

            //
            // Species do not currently have names, so just use their index.
            //
            for (k = 0 ; k < spec->nmatspec[j] ; k++)
            {
                char num[16];
                sprintf(num, "%d", k+1);
                tmp_string_vector.push_back(num);
            }
            speciesNames.push_back(tmp_string_vector);
        }
        char *name_w_dir = GenerateName(dirname, matspecies_names[i]);
        char *meshname_w_dir = GenerateName(dirname, meshname);
        avtSpeciesMetaData *smd = new avtSpeciesMetaData(name_w_dir,
                                  meshname_w_dir, spec->matname, spec->nmat,
                                  numSpecies, speciesNames);
        md->Add(smd);

        delete [] name_w_dir;
        delete [] meshname_w_dir;
        DBFreeMatspecies(spec);
    }

    //
    // Multi-species
    //
    for (i = 0 ; i < nmultimatspecies ; i++)
    {
        DBmultimatspecies *ms = GetMultimatspec(dirname,
                                                     multimatspecies_names[i]);
        if (ms == NULL)
            EXCEPTION1(InvalidVariableException, multimatspecies_names[i]);

        RegisterDomainDirs(ms->specnames, ms->nspec, dirname);

        // Find the first non-empty mesh
        int meshnum = 0;
        bool valid_var = true;
        while (string(ms->specnames[meshnum]) == "EMPTY")
        {
            meshnum++;
            if (meshnum >= ms->nspec)
            {
                debug1 << "Invalidating species \"" << multimatspecies_names[i]
                       << "\" since all its blocks are EMPTY." << endl;
                valid_var = false;
                break;
            }
        }

        string meshname;
        DBmatspecies *spec = NULL;

        if (valid_var)
        {
            // get the associated multimat

            // We can only get this "matname" using GetComponent.  It it not
            // yet a part of the DBmultimatspec structure, so this is the only
            // way we can get this info.
            char *multimatName = (char *) GetComponent(dbfile,
                                              multimatspecies_names[i], "matname");

            // get the multimesh for the multimat
            DBmultimat *mm = GetMultimat(dirname, multimatName);
            if (mm == NULL)
                EXCEPTION1(InvalidVariableException, multimatspecies_names[i]);
            char *material = mm->matnames[meshnum];

            TRY
            {
                meshname = DetermineMultiMeshForSubVariable(dbfile,
                                                          multimatspecies_names[i],
                                                          mm->matnames,
                                                          ms->nspec, dirname);
            }
            CATCH(SiloException)
            {
                debug1 << "Giving up on var \"" << multimatspecies_names[i]
                       << "\" since its first non-empty block (" << material
                       << ") is invalid." << endl;
                valid_var = false;
            }
            ENDTRY

            // get the species info
            char *species = ms->specnames[meshnum];

            char   *realvar = NULL;
            DBfile *correctFile = dbfile;
            DetermineFileAndDirectory(species, correctFile, 0, realvar);

            DBShowErrors(DB_NONE, NULL);
            spec = DBGetMatspecies(correctFile, realvar);
            DBShowErrors(DB_ALL, ExceptionGenerator);
            if (spec == NULL)
            {
                debug1 << "Giving up on species \"" << multimatspecies_names[i]
                       << "\" since its first non-empty block (" << species
                       << ") is invalid." << endl;
                valid_var = false;
            }
        }

        vector<int>              numSpecies;
        vector< vector<string> > speciesNames;
        if (valid_var)
        {
            for (j = 0 ; j < spec->nmat ; j++)
            {
                numSpecies.push_back(spec->nmatspec[j]);
                vector<string>  tmp_string_vector;
                for (k = 0 ; k < spec->nmatspec[j] ; k++)
                {
                    char num[16];
                    sprintf(num, "%d", k+1);
                    tmp_string_vector.push_back(num);
                }
                speciesNames.push_back(tmp_string_vector);
            }
        }
        char *name_w_dir = GenerateName(dirname, multimatspecies_names[i]);
        avtSpeciesMetaData *smd;
        if (valid_var)
            smd = new avtSpeciesMetaData(name_w_dir,
                                         meshname, spec->matname, spec->nmat,
                                         numSpecies, speciesNames);
        else
            smd = new avtSpeciesMetaData(name_w_dir, "", "", 0,
                                         numSpecies, speciesNames);
        smd->validVariable = valid_var;
        md->Add(smd);

        delete [] name_w_dir;
        DBFreeMatspecies(spec);
    }
*/

  // Delete copied TOC

  // Meshes
  for (i = 0 ; i < nmultimesh ; i++)
    {
    delete [] multimesh_names[i];
    }
  delete [] multimesh_names;
  for (i = 0 ; i < nqmesh ; i++)
    {
    delete [] qmesh_names[i];
    }
  delete [] qmesh_names;
  for (i = 0 ; i < nucdmesh ; i++)
    {
    delete [] ucdmesh_names[i];
    }
  delete [] ucdmesh_names;
  for (i = 0 ; i < nptmesh ; i++)
    {
    delete [] ptmesh_names[i];
    }
  delete [] ptmesh_names;
  for (i = 0 ; i < ncurves; i++)
    {
    delete [] curve_names[i];
    }
  delete [] curve_names;

  // Vars
  for (i = 0 ; i < nmultivar ; i++)
    {
    delete [] multivar_names[i];
    }
  delete [] multivar_names;
  for (i = 0 ; i < nqvar ; i++)
    {
    delete [] qvar_names[i];
    }
  delete [] qvar_names;
  for (i = 0 ; i < nucdvar ; i++)
    {
    delete [] ucdvar_names[i];
    }
  delete [] ucdvar_names;
  for (i = 0 ; i < nptvar ; i++)
    {
    delete [] ptvar_names[i];
    }
  delete [] ptvar_names;

  // Materials
  for (i = 0 ; i < nmat ; i++)
    {
    delete [] mat_names[i];
    }
  delete [] mat_names;
  for (i = 0 ; i < nmultimat ; i++)
    {
    delete [] multimat_names[i];
    }
  delete [] multimat_names;

  // Species
  for (i = 0 ; i < nmatspecies ; i++)
    {
    delete [] matspecies_names[i];
    }
  delete [] matspecies_names;
  for (i = 0 ; i < nmultimatspecies ; i++)
    {
    delete [] multimatspecies_names[i];
    }
  delete [] multimatspecies_names;


  //
  // Call ReadDir recursively each subdirectory.
  //
  for (i = 0 ; i < ndir ; i++)
    {
    char path[1024];
    int length = strlen(dirname);
    if (length > 0 && dirname[length-1] != '/')
      {
      sprintf(path, "%s/%s", dirname, dir_names[i]);
      }
    else
      {
      sprintf(path, "%s%s", dirname, dir_names[i]);
      }
    if (!ShouldGoToDir(path))
      {
      continue;
      }
    DBSetDir(dbfile, dir_names[i]);
    ReadDir(dbfile, path);
    DBSetDir(dbfile, "..");
    }


  // Delete dir names
  for (i = 0 ; i < ndir ; i++)
    {
    delete [] dir_names[i];
    }
  delete [] dir_names;

}

//----------------------------------------------------------------------------

/*
vtkstd::string vtkSiloReader::DetermineMultiMeshForSubVariable(DBfile *dbfile,
                                                    const char *name,
                                                    char **varname,
                                                    int nblocks,
                                                    const char *curdir)
{
    int i;
    char subMesh[256];
    char subMeshTmp[256];

    //
    // First, see if we've got the answer in the multivarToMultimeshMap
    //
    vtkstd::map<vtkstd::string,vtkstd::string>::const_iterator cit = multivarToMultimeshMap.find(name);
    if (cit != multivarToMultimeshMap.end())
      {
        vtkDebugMacro("Matched multivar \"" << name << "\" to multimesh \""
               << cit->second << "\" using multivarToMultimeshMap");
        return cit->second;
      }

    // Find the first non-empty mesh
    int meshnum = 0;
    while (vtkstd::string(varname[meshnum]) == "EMPTY")
    {
        meshnum++;
        if (meshnum >= nblocks)
        {
            return "";
        }
    }

   // GetMeshname(dbfile, varname[meshnum], subMesh);

//  char subMesh[256];
  char   *dirvar;
  DBfile *correctFile = dbfile;
  DetermineFileAndDirectory(varname[meshnum], correctFile, 0, dirvar);
  int rv = DBInqMeshname(correctFile, dirvar, subMesh);
  if (rv < 0)
    {
    vtkDebugMacro("Unable to determine mesh for variable: " << var);
    return "";
    }

    //
    // The code involving subMeshTmp is to maintain backward compability
    // with Silo/HDF5 files in which HDF5 driver had a bug in that it
    // *always* added a leading slash to the name of the mesh associated
    // with an object. Eventually, this code can be eliminated
    //
    if (subMesh[0] == '/')
    {
        for (i = 0; i < strlen(subMesh); i++)
            subMeshTmp[i] = subMesh[i+1];
    }

    //
    // varname is very likely qualified with a file name.  We need to figure
    // out what it's mesh's name looks like the prepended file name, so we can
    // meaningfully compare it with our list of submeshes.
    //
    char subMeshWithFile[1024];
    char subMeshWithFileTmp[1024];
    GetRelativeVarName(varname[meshnum], subMesh, subMeshWithFile);
    if (subMesh[0] == '/')
        GetRelativeVarName(varname[meshnum], subMeshTmp, subMeshWithFileTmp);

    //
    // Attempt an "exact" match, where the first mesh for the multivar is
    // an exact match and the number of domains is the same.
    //
    int size = actualMeshName.size();
    for (i = 0 ; i < size ; i++)
    {
        if (firstSubMesh[i] == subMeshWithFile && nblocks == blocksForMesh[i])
        {
            return actualMeshName[i];
        }
    }
    if (subMesh[0] == '/')
    {
        for (i = 0 ; i < size ; i++)
        {
            if (firstSubMesh[i] == subMeshWithFileTmp && nblocks == blocksForMesh[i])
            {
                return actualMeshName[i];
            }
        }
    }

    //
    // Couldn't find an exact match, so try something fuzzier:
    // Look for a multimesh which has the same name as the mesh for
    // the multivar, and match up domains by directory name.
    //
    debug5 << "Using fuzzy logic to match multivar \"" << name << "\" to a multimesh" << endl;
    string dir,varmesh;
    SplitDirVarName(subMesh, curdir, dir, varmesh);
    for (i = 0 ; i < size ; i++)
    {
        if (firstSubMeshVarName[i] == varmesh &&
            blocksForMesh[i] == nblocks)
        {
#ifndef MDSERVER

            string *dirs = new string[nblocks];
            for (int k = 0; k < nblocks; k++)
                SplitDirVarName(varname[k], curdir, dirs[k], varmesh);

            for (int j = 0; j < allSubMeshDirs[i].size(); j++)
            {
                int match = -1;
                for (int k = 0; k < nblocks && match == -1; k++)
                {
                    if (dirs[k] == allSubMeshDirs[i][j])
                    {
                        match = k;
                    }
                }
                blocksForMultivar[name].push_back(match);
            }

            delete [] dirs;

#endif
            return actualMeshName[i];
        }
    }

  // We weren't able to find a match.
  vtkDebugMacro( "Was not able to match multivar \"" << name << "\" and its first"
                 "non-empty submesh \"" << varname[meshnum] << "\" in file "
                 << subMeshWithFile << " to a multi-mesh.");
  return "";
}

//----------------------------------------------------------------------------

void vtkSiloReader::GetMeshname(DBfile *dbfile, char *var, char *meshname)
{
  char   *dirvar;
  DBfile *correctFile = dbfile;
  DetermineFileAndDirectory(var, correctFile, 0, dirvar);
  int rv = DBInqMeshname(correctFile, dirvar, meshname);
  if (rv < 0)
    {
    vtkDebugMacro("Unable to determine mesh for variable: " << var);
    return -1;
    }
}
*/

//----------------------------------------------------------------------------


// ****************************************************************************
//  Method: vtkSiloReader::GetMeshType
//
//  Purpose:
//      Gets the mesh type for a variable, even if it is in a different file.
//
//  Arguments:
//      dbfile    The dbfile that mesh came from.
//      mesh      A mesh name, possibly with a prepended directory and filename
//
// ****************************************************************************

int vtkSiloReader::GetMeshType(DBfile *dbfile, char *mesh)
{
    char   *dirvar;
    DBfile *correctFile = dbfile;
    DetermineFileAndDirectory(mesh, correctFile, 0, dirvar);
    int rv = DBInqMeshtype(correctFile, dirvar);
    if (rv < 0)
    {
        vtkErrorMacro("Unable to determine mesh type for mesh: " << mesh);
    }
    return rv;
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Function: PrepareDirName
//
//  Purpose:
//      Removes the appended variable and '/' from a character string and
//      returns a directory name.
//
//  Arguments:
//      dirvar  The directory and variable name in a string.
//      curdir  The current directory.
// ****************************************************************************

vtkstd::string PrepareDirName(const char *dirvar, const char *curdir)
{
    int len = strlen(dirvar);
    const char *last = dirvar + (len-1);
    while (*last != '/' && last > dirvar)
    {
        last--;
    }

    if (*last != '/')
    {
        //debug1 << "Unexpected case -- no dirs what-so-ever." << endl;
    }

    char str[1024];
    int dirlen = 0;
    if (dirvar[0] != '/')
    {
        //
        // We have a relative path -- prepend the current directory.
        //
        strcpy(str, curdir);
        dirlen = strlen(str);
    }
    strncpy(str+dirlen, dirvar, last-dirvar);
    str[dirlen + (last-dirvar)] = '\0';

    return vtkstd::string(str);
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Method: avtSiloFileFormat::DetermineFilenameAndDirectory
//
//  Purpose:
//      Parses a string from a Silo file that has form "filename:directory/var"
//      and determines which part is "filename" and which is "directory/var".
//
//      input       The input string ("filename:directory/var")
//      filename    The "filename" part of the string.  This will copy a string
//                  into a buffer, so a buffer must be provided as input.
//      location    The "directory/var" part of the string.  This will just
//                  point into "input" so there is no memory management to
//                  worry about.
// ****************************************************************************

void vtkSiloReader::DetermineFilenameAndDirectory(char *input,
    const char *mdirname, char *filename, char *&location,
    bool *allocated_location)
{
    if (allocated_location)
        *allocated_location = false;

    //
    // Figure out if there is a ':' in the string.
    //
    char *p = strstr(input, ":");

    if (p == NULL)
    {
        //
        // There is no colon, so the variable must be in the current file.
        // Leave the file handle alone.
        //
        strcpy(filename, ".");
        if (mdirname == 0 || strcmp(input, "EMPTY") == 0 || input[0] == '/' ||
          (input[0] == '.' && input[1] == '/'))
        {
           location = input;
        }
        else
        {
            if (!strncmp(mdirname, input, strlen(mdirname)) == 0)
            {
                char tmp[1024];
                sprintf(tmp, "%s/%s", mdirname, input);
                location = CXX_strdup(tmp);
                if (allocated_location)
                    *allocated_location = true;
            }
            else
            {
              location = input;
            }
        }
    }
    else
    {
        //
        // Make a copy of the string all the way up to the colon.
        //
        strncpy(filename, input, p-input);
        filename[p-input] = '\0';

        //
        // The location of the variable is *one after* the colon.
        //
        location = p+1;
    }
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Method: vtkSiloReader::DetermineFileAndDirectory
//
//  Purpose:
//      Takes a string from a Silo file that as "filename:directory/var" and
//      returns the correct file as well as the substring that is only
//      directory and var.
//
//  Arguments:
//      input       The input string ("filename:directory/var")
//      cFile       The correct file corresponding to filename.
//      location    The "directory/var" part of the string.  This will just
//                  point into the input so there is no memory management to
//                  worry about, unless argument allocated_location returns true.
// ****************************************************************************
void vtkSiloReader::DetermineFileAndDirectory(char *input, DBfile *&cFile,
       const char *meshDirname, char *&location, bool *allocated_location)
{
  char filename[1024];
  DetermineFilenameAndDirectory(input, meshDirname, filename, location,
                                allocated_location);
  if (strcmp(filename, ".") != 0)
    {
    // The variable is in a different file, so open that file.  This will
    // create the filename and add it to our registry if necessary.
    cFile = this->TOC->OpenFile(filename);
    }
}

//----------------------------------------------------------------------------

// Input is the full name of the mesh.
// Input could look like:
// A) filename:/directory/mesh
// B) directory/mesh
// If A, return the index of filename, otherwise do not change index.
void vtkSiloReader::DetermineFileIndexAndDirectory(char *input, int &index,
       const char *meshDirname, char *&location, bool *allocated_location)
{
  char filename[1024];
  DetermineFilenameAndDirectory(input, meshDirname, filename, location,
                                allocated_location);
  if (strcmp(filename, ".") != 0)
    {
    index = this->TOC->DetermineFileNameIndex(filename);
    }
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Function: GenerateName
//
//  Purpose:
//      Generates a name from a directory and a file name.
//
//  Arguments:
//      dirname   A directory name.
//      varname   The name of the variable.
//
//  Returns:      A string for the directory and variable.
//
//  Notes:        The caller must free the return value.
// ****************************************************************************
char * GenerateName(const char *dirname, const char *varname)
{
    if (varname[0] == '/')
    {
        int len = strlen(varname);
        int num_slash = 0;
        for (int i = 0 ; i < len ; i++)
            if (varname[i] == '/')
                num_slash++;

        //
        // If there are lots of slashes, then we have a fully qualified path,
        // so leave them all in.  If there is only one slash (and it is the
        // first one), then take out the slash -- since the var would be
        // referred to as "Mesh", not "/Mesh".
        //
        int offset = (num_slash > 1 ? 0 : 1);
        char *rv = new char[strlen(varname)+1];
        strcpy(rv, varname+offset);
        return rv;
    }

    int amtForSlash = 1;
    int amtForNullChar = 1;
    int amtForMiddleSlash = 1;
    int len = strlen(dirname) + strlen(varname) - amtForSlash + amtForNullChar
                                                + amtForMiddleSlash;

    char *rv = new char[len];

    const char *dir_without_leading_slash = dirname+1;

    bool needMiddleSlash = false;
    if (strlen(dir_without_leading_slash) > 0)
    {
        needMiddleSlash = true;
    }

    if (needMiddleSlash)
    {
        sprintf(rv, "%s/%s", dir_without_leading_slash, varname);
    }
    else
    {
        sprintf(rv, "%s%s", dir_without_leading_slash, varname);
    }

    return rv;
}

//----------------------------------------------------------------------------

// ****************************************************************************
//  Method: avtSiloFileFormat::DetermineFileAndDirectory
//
//  Purpose:
//      If a variable (like "/block1/mesh") has pointers to other variables
//      (like "fl") then they come unqualified with a path (should be
//      "/block1/fl").  This routine qualifies the variable.
//
//  Arguments:
//      initVar     The original variable ("/block1/mesh").
//      newVar      The variable initVar points to ("fl").
//      outVar      The relative variable name for newVar ("/block1/fl").
// ****************************************************************************
void vtkSiloReader::GetRelativeVarName(const char *initVar, const char *newVar,
                                      char *outVar)
{
    //
    // If the new variable starts with a slash, it is already qualified, so
    // just return that.
    //
    int len = strlen(initVar);
    if (newVar[0] == '/')
    {
        int colonPosition = -1;
        for (int i = 0 ; i < len; i++)
        {
            if (initVar[i] == ':')
            {
                colonPosition = i;
                break;
            }
        }
        int numToCopy = (colonPosition < 0 ? 0 : colonPosition+1);
        strncpy(outVar, initVar, numToCopy);
        strcpy(outVar+numToCopy, newVar);
        return;
    }

    int lastToken = -1;
    for (int i = len-1 ; i >= 0 ; i--)
    {
        if (initVar[i] == '/' || initVar[i] == ':')
        {
            lastToken = i;
            break;
        }
    }

    int numToCopy = (lastToken < 0 ? 0 : lastToken+1);
    strncpy(outVar, initVar, numToCopy);
    strcpy(outVar+numToCopy, newVar);
}

//----------------------------------------------------------------------------

inline char * CXX_strdup(char const * const c)
{
  char *p = new char[strlen(c)+1];
  strcpy(p, c);
  return p;
}

//----------------------------------------------------------------------------

