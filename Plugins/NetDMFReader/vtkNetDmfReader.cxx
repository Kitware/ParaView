  /*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetDmfReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen  
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
  of any contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

  * Modified source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  =========================================================================*/
#include "vtkNetDmfReader.h"

#include "vtkSmartPointer.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkXMLParser.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkCharArray.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "XdmfArray.h"
#include "XdmfAttribute.h"
#include "XdmfDOM.h"
#include "XdmfDataDesc.h"
#include "XdmfDataItem.h"
#include "XdmfGrid.h"
#include "XdmfTopology.h"
#include "XdmfGeometry.h"
#include "XdmfTime.h"
#include "XdmfSet.h"

#include <sys/stat.h>
#include <vtkstd/set>
#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/SystemTools.hxx>
#include <assert.h>
#include <functional>
#include <algorithm>

#include "NetDMFScenario.h"
#include "NetDMFNode.h"

#define USE_IMAGE_DATA // otherwise uniformgrid

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkNetDmfReader);
vtkCxxRevisionMacro(vtkNetDmfReader, "1.1");

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__))
#  include <direct.h>
#  define GETCWD _getcwd
#else
#include <unistd.h>
#  define GETCWD getcwd
#endif

#define vtkMAX(x, y) (((x)>(y))?(x):(y))
#define vtkMIN(x, y) (((x)<(y))?(x):(y))

#define PRINT_EXTENT(x) "[" << (x)[0] << " " << (x)[1] << " " << (x)[2] << " " << (x)[3] << " " << (x)[4] << " " << (x)[5] << "]" 

//============================================================================
class vtkNetDmfReaderInternal
{
public:
  vtkNetDmfReaderInternal()
  {
    this->DataItem = NULL;
    this->DsmBuffer = NULL;
    this->InputString = 0;
    this->InputStringLength = 0;
    // this->ParallelLevel = 0;
    this->ParallelLevels.clear();
    this->UpdatePiece     = 0;
    this->UpdateNumPieces = 1;
    this->mostChildren = 0;
  }

  ~vtkNetDmfReaderInternal()
  {
    if ( this->DataItem )
      {
      delete this->DataItem;
      this->DataItem = 0;
      }
    delete [] this->InputString;
    this->InputString = 0;
  }


  // Returns the domain node for the domain with given name. If none such domain
  // exists, returns the 1st domain, if any.
  XdmfXmlNode GetDomain(const char* domainName)
    {
    if (domainName)
      {
      vtkstd::map<vtkstd::string, XdmfXmlNode>::iterator iter =
        this->DomainMap.find(domainName);
      if (iter != this->DomainMap.end())
        {
        return iter->second;
        }
      }
    if (this->DomainList.size() > 0)
      {
      return this->GetDomain(this->DomainList[0].c_str());
      }
    return 0;
    }

  vtkNetDmfReaderGrid* GetGrid(const char* gridName);
  vtkNetDmfReaderGrid* GetGrid(int idx);
  vtkNetDmfReaderGrid *AddGrid(
    vtkNetDmfReaderGrid *parent,
    const char *gridName);
  void DeleteChildren(vtkNetDmfReaderGrid* parent);

  // Temporary method to update the list of arrays provided by the grid.
  int UpdateArrays(vtkNetDmfReaderGrid *grid);
  int RequestGridInformation(vtkNetDmfReaderGrid *grid, vtkInformation *destInfo);
  int FindParallelism(vtkNetDmfReaderGrid *grid = 0){return 0;}

  int RequestGridData(/*const char* currentGridName,*/
    vtkNetDmfReaderGrid *grid,
    vtkDataObject *output,
    int timeIndex,
    int isSubBlock,
    double progressS, double progressE);
  
 
  vtkstd::vector<XdmfFloat64> TimeValues;
  vtkstd::vector<vtkstd::string> DomainList;
  vtkstd::map<vtkstd::string, XdmfXmlNode> DomainMap;
  // vtkNetDmfReaderGrid *ParallelLevel;
  vtkstd::vector<vtkNetDmfReaderGrid*> ParallelLevels;
  vtkNetDmfReader* Reader;
  XdmfDataItem *DataItem;
  XdmfDsmBuffer *DsmBuffer;
  char *InputString;
  unsigned int InputStringLength;
  unsigned int mostChildren;

  unsigned int UpdatePiece;
  unsigned int UpdateNumPieces;

};

//============================================================================
class vtkNetDmfReaderTester : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkNetDmfReaderTester, vtkXMLParser);
  static vtkNetDmfReaderTester* New();
  int TestReadFile()
    {
      this->Valid = 0;
      if(!this->FileName)
        {
        return 0;
        }

      ifstream inFile(this->FileName);
      if(!inFile)
        {
        return 0;
        }

      this->SetStream(&inFile);
      this->Done = 0;

      this->Parse();

      if(this->Done && this->Valid )
        {
        return 1;
        }
      return 0;
    }
  void StartElement(const char* name, const char**)
    {
      this->Done = 1;
      if(strcmp(name, "Xdenf") == 0)
        {
        this->Valid = 1;
        }
    }

protected:
  vtkNetDmfReaderTester()
    {
      this->Valid = 0;
      this->Done = 0;
    }

private:
  void ReportStrayAttribute(const char*, const char*, const char*) {}
  void ReportMissingAttribute(const char*, const char*) {}
  void ReportBadAttribute(const char*, const char*, const char*) {}
  void ReportUnknownElement(const char*) {}
  void ReportXmlParseError() {}

  int ParsingComplete() { return this->Done; }
  int Valid;
  int Done;
  vtkNetDmfReaderTester(const vtkNetDmfReaderTester&); // Not implemented
  void operator=(const vtkNetDmfReaderTester&); // Not implemented
};

vtkStandardNewMacro(vtkNetDmfReaderTester);

//============================================================================
vtkNetDmfReader::vtkNetDmfReader()
{  
  this->Internals = new vtkNetDmfReaderInternal;
  this->Internals->Reader = this;

  this->DOM = 0;

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->TimeStep       = 0;
  this->ActualTimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  // this->DebugOn();
}

//----------------------------------------------------------------------------
vtkNetDmfReader::~vtkNetDmfReader()
{
  delete this->Internals;

  if ( this->DOM )
    {
    delete this->DOM;
    }

  H5garbage_collect();
}



//----------------------------------------------------------------------------
int vtkNetDmfReader::CanReadFile(const char* fname)
{
  vtkNetDmfReaderTester* tester = vtkNetDmfReaderTester::New();
  tester->SetFileName(fname);
  int res = tester->TestReadFile();
  tester->Delete();


  XdmfDOM* Dom = new XdmfDOM();
  Dom->Parse(fname);
  cout << "Parsed XML" << endl;
  XdmfXmlNode scennode = Dom->FindElement("Scenario");
  NetDMFScenario* scenario = new NetDMFScenario();
  scenario->SetDOM(Dom);
  scenario->SetElement(scennode);
  scenario->UpdateInformation();
  int nnodes = scenario->GetNumberOfNodes();
  for( int i = 0 ; i < nnodes ; ++i)
    {
    cout << scenario->GetNode(i)->GetName() << " has "
              << scenario->GetNode(i)->GetNumberOfDevices() << " devices" 
              << endl;
    }
  cout << "Found " << nnodes << " Nodes" << endl;

  return res;
}

//----------------------------------------------------------------------------

int vtkNetDmfReader::FillOutputPortInformation(int,
                                             vtkInformation *info)
{ 
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
  return 1;
}

//----------------------------------------------------------------------------
void vtkNetDmfReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkNetDmfReader::ParseXML()
{
  cout << "Begin Parsing" << endl;
  // * Ensure that the required objects have been instantiated.
  if (!this->DOM)
    {
    this->DOM = new XdmfDOM();
    }
  if ( !this->Internals->DataItem )
    {
    this->Internals->DataItem = new XdmfDataItem();
    this->Internals->DataItem->SetDOM(this->DOM);
    }

  // * Check if the XML file needs to be re-read.
  bool modified = true;
  if (this->GetReadFromInputString())
    {
    const char* data=0;
    unsigned int data_length=0;
    if (this->InputArray)
      {
      vtkDebugMacro(<< "Reading from InputArray");
      data = this->InputArray->GetPointer(0);
      data_length = static_cast<unsigned int>(
        this->InputArray->GetNumberOfTuples()*
        this->InputArray->GetNumberOfComponents());
      }
    else if (this->InputString)
      {
      data = this->InputString;
      data_length = this->InputStringLength;
      }
    else
      {
      vtkErrorMacro("No input string specified");
      return false;
      }
    vtkDebugMacro("Input Text Changed ...  re-parseing");
    delete [] this->Internals->InputString;
    this->Internals->InputString = new char[data_length+1];
    memcpy(this->Internals->InputString, data, data_length);
    this->Internals->InputString[data_length] = 0; 
    this->Internals->InputStringLength = data_length;

    this->DOM->SetInputFileName(0);
    this->DOM->Parse(this->Internals->InputString);
    }
  else
    {
    // Parse the file...
    if (!this->FileName )
      {
      vtkErrorMacro("File name not set");
      return false;
      }

    // First make sure the file exists.  This prevents an empty file
    // from being created on older compilers.
    if (!vtksys::SystemTools::FileExists(this->FileName))
      {
      vtkErrorMacro("Error opening file " << this->FileName);
      return false;
      }


    if (this->DOM->GetInputFileName() &&
      STRCASECMP(this->DOM->GetInputFileName(), this->FileName) == 0)
      {
      vtkDebugMacro("Filename Unchanged ... skipping re-parse()");
      modified = false;
      }
    else
      {
      vtkDebugMacro("Parsing file: " << this->FileName);

      //Tell the parser what the working directory is.
      vtkstd::string directory =
        vtksys::SystemTools::GetFilenamePath(this->FileName) + "/";
      if (directory == "/")
        {
        directory = vtksys::SystemTools::GetCurrentWorkingDirectory() + "/";
        }
      //    directory = vtksys::SystemTools::ConvertToOutputPath(directory.c_str());
      this->DOM->SetWorkingDirectory(directory.c_str());
      this->DOM->SetInputFileName(this->FileName);
      this->DOM->Parse(this->FileName);
      }
    }
  cout << "Done Parsing" << endl;

  if (modified)
    {
    cout << "ParseXML" << endl;
    // Since the DOM was re-parsed we need to update the cache for domains
    // and re-populate the grids.
    //this->UpdateDomains();
    //this->UpdateRootGrid();
    }
  return true;
}

//----------------------------------------------------------------------------
int vtkNetDmfReader::RequestDataObject(vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **vtkNotUsed(inputVector),
                                       vtkInformationVector *outputVector)
{
  cout << "RequestDataObject: " << endl;
  if (!this->ParseXML())
    {
    return 0;
    }

  //Look at the in memory structures and create an empty vtkDataObject of the
  //proper type for RequestData to fill in later, if needed.

  vtkDataObject *output= vtkDataObject::GetData(outputVector, 0);
  if (!output)
    {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    output = vtkMutableDirectedGraph::New();
    output->SetPipelineInformation(outInfo);
    outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkNetDmfReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro("RequestInformation");
  
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  //say we can produce as many pieces are are desired
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  /// missing things...
  this->ActualTimeStep = this->TimeStep;

  return 1;
}

//----------------------------------------------------------------------------
class WithinTolerance: public vtkstd::binary_function<double, double, bool>
{
public:
    result_type operator()(first_argument_type a, second_argument_type b) const
    {
      bool result = (fabs(a-b)<=(a*1E-6));
      return (result_type)result;
    }
};

//----------------------------------------------------------------------------
int vtkNetDmfReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if ( !this->GetReadFromInputString() && !this->FileName )
    {
    vtkErrorMacro("Not Reading from String and File name not set");
    return 0;
    }
  if ( !this->DOM )
    {
    return 0;
    }
  
  //long int starttime = this->GetMTime();

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *outStructure = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outStructure)
    {
    vtkErrorMacro( << "No Output VTK structure");
    return VTK_ERROR;
    }

  this->Internals->UpdatePiece     = 0;
  this->Internals->UpdateNumPieces = 1;

  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    this->Internals->UpdatePiece = 
      outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    this->Internals->UpdateNumPieces =
      outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  
  vtkDebugMacro( << "UpdatePiece " << this->Internals->UpdatePiece << " : UpdateNumPieces " << this->Internals->UpdateNumPieces);

  //
  // Find the correct time step
  //
  this->ActualTimeStep = this->TimeStep;

  this->Internals->FindParallelism();

  //long int endtime = this->GetMTime();

  return 1;
}
