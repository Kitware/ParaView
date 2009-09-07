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
#include "vtkStringArray.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSetAttributes.h"
#include "vtkVariantArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"

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
#include <vtksys/ios/sstream>
#include <vtkstd/vector>
#include <vtksys/SystemTools.hxx>
#include <assert.h>
#include <functional>
#include <algorithm>

#include "NetDMFAddressItem.h"
#include "NetDMFConversation.h"
#include "NetDMFEvent.h"
#include "NetDMFNode.h"
#include "NetDMFResult.h"
#include "NetDMFScenario.h"
#include "NetDMFRoot.h"
#include "NetDMFMovement.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#ifdef MIN
#  undef MIN
#endif
#define MIN(A,B) ((A) < (B)) ? (A) : (B)

#ifdef MAX
#  undef MAX
#endif
#define MAX(A,B) ((A) > (B)) ? (A) : (B)


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkNetDmfReader);
vtkCxxRevisionMacro(vtkNetDmfReader, "1.3");

//============================================================================
class vtkNetDmfReaderInternal
{
public:
  vtkNetDmfReaderInternal()
  {
    this->DOM = 0;
    this->DataItem = NULL;
    this->DsmBuffer = NULL;
    this->InputString = 0;
    this->InputStringLength = 0;
    // this->ParallelLevel = 0;
    this->ParallelLevels.clear();
    this->UpdatePiece     = 0;
    this->UpdateNumPieces = 1;
    this->mostChildren = 0;
    this->TimeRange[0] = 0.;
    this->TimeRange[1] = 0.;
    
    this->NameProperties = 0;
    this->TypeProperties = 0;
    this->ClassProperties = 0;
    this->NodeIdProperties = 0;
    this->PositionProperties = 0;
  }

  ~vtkNetDmfReaderInternal()
  {
    if ( this->DataItem )
      {
      delete this->DataItem;
      this->DataItem = 0;
      }
    if ( this->DOM )
      {
      delete this->DOM;
      }
    delete [] this->InputString;
    this->InputString = 0;
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
  void UpdateTimeRange(NetDMFEvent* event);
  // Should probably part of the NetDMF library (class methods ?)
  void GetEventTimeRange(NetDMFEvent* event, double timeRange[2]);
  double GetMovementEndTime(NetDMFMovement* movement, double startTime);
  void GetMovementTimeRange(NetDMFMovement* movement, double timeRange[2]);

  vtkstd::map<vtkStdString, int> ElementCount;
 
  // vtkNetDmfReaderGrid *ParallelLevel;
  vtkstd::vector<vtkNetDmfReaderGrid*> ParallelLevels;
  NetDMFDOM*       DOM;
  vtkNetDmfReader* Reader;

  vtkStringArray*  NameProperties;
  vtkStringArray*  TypeProperties;
  vtkStringArray*  ClassProperties;
  vtkIntArray*     NodeIdProperties;
  vtkFloatArray*   PositionProperties;

  XdmfDataItem*    DataItem;
  XdmfDsmBuffer*   DsmBuffer;
  char*            InputString;
  unsigned int     InputStringLength;
  unsigned int     mostChildren;

  double       TimeRange[2];

  unsigned int UpdatePiece;
  unsigned int UpdateNumPieces;

};

//============================================================================
void vtkNetDmfReaderInternal::UpdateTimeRange(NetDMFEvent* event)
{
  cout << __FUNCTION__ << " " << static_cast<int>(this->TimeRange[0])
       << " " << static_cast<int>(this->TimeRange[0]) << endl;
  cout << "event: " << event << endl;
  double timeRange[2];
  this->GetEventTimeRange(event, timeRange);

  if (this->TimeRange[0] == 0.0)
    {
    this->TimeRange[0] = timeRange[0];
    }
  else if (timeRange[0] != 0.0)
    {
    this->TimeRange[0] = 
      MIN(this->TimeRange[0], timeRange[0]);
    }
  
  if (this->TimeRange[1] == 0.0)
    {
    this->TimeRange[1] = timeRange[1];
    }
  else if (timeRange[1] != 0.0)
    {
    this->TimeRange[1] = 
      MAX(this->TimeRange[1], timeRange[1]);
    }
  cout << __FUNCTION__ << " end " << static_cast<int>(this->TimeRange[0])
       << " " << static_cast<int>(this->TimeRange[1]) << endl;
}

//============================================================================
void vtkNetDmfReaderInternal::GetEventTimeRange(NetDMFEvent* event, double timeRange[2])
{
  timeRange[0] = 0.;
  timeRange[1] = 0.;

  timeRange[0] = event->GetStartTime();
  if (event->GetEndTime() != 0.)
    {
    timeRange[1] =event->GetEndTime();
    }
  else
   {
   int numberOfMovements = event->GetNumberOfMovements();
   for (int i = 0; i < numberOfMovements; ++i)
     {
     double movementEndTime = 
       this->GetMovementEndTime(event->GetMovement(i), timeRange[0]);
     timeRange[1] = MAX(timeRange[1], movementEndTime);
     }
   // TODO: get the starttime/endtime from conversation/traffic
   }
}

//==============================================================================
double vtkNetDmfReaderInternal::GetMovementEndTime(NetDMFMovement* movement, 
                                                   double startTime)
{
  cout << __FUNCTION__ << " " << movement << " " << static_cast<int>(startTime) ;
  // TODO: Support more than Path movement type
  XdmfDataItem* pathData = 
    (movement->GetMovementType() == NETDMF_MOVEMENT_TYPE_PATH)? 
    movement->GetPathData(): 0;
  // warning we need to call UPDATE() here. Would be nice if we don't 
  // have to.
  if (!pathData)
    {
    return startTime;
    }
  pathData->Update();
  XdmfArray* dataArray = pathData->GetArray();
  double endTime = 
    startTime + movement->GetMovementInterval() * pathData->GetDataDesc()->GetDimension(0);
  cout << " end: " << static_cast<int>(endTime) << " interval" << movement->GetMovementInterval() << " elements:" << dataArray->GetNumberOfElements() / pathData->GetDataDesc()->GetDimension(1) << endl;
  return endTime;
}

//============================================================================
void vtkNetDmfReaderInternal::GetMovementTimeRange(NetDMFMovement* movement, double timeRange[2])
{
  timeRange[0] = 0.;
  timeRange[1] = 0.;
  XdmfXmlNode movementNode = movement->GetElement();
  // Get Parent Event
  // TODO: cache all the NetDMFElement for a faster access,
  XdmfXmlNode eventNode = this->DOM->GetParentNode(movementNode);

  NetDMFEvent e;
  while (timeRange[0] == 0. && eventNode && 
         XDMF_WORD_CMP(e.GetElementName(), 
                       this->DOM->GetName(eventNode)))
    {
    NetDMFEvent* event = new NetDMFEvent;
    event->SetDOM(this->DOM);
    event->SetElement(eventNode);
    event->UpdateInformation();
    timeRange[0] = event->GetStartTime();
    eventNode = this->DOM->GetParentNode(eventNode);
    delete event;
    }
  cout << __FUNCTION__ << " start" << static_cast<int>(timeRange[0]) << endl;
  timeRange[1] = this->GetMovementEndTime(movement, timeRange[0]);
}

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
      vtkstd::string nameString(name);
      if (nameString == "Xdenf" || 
          nameString == "NetDMF" || 
          nameString == "Xmn")
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
  this->Internal = new vtkNetDmfReaderInternal;
  this->Internal->Reader = this;

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->TimeStep       = 0;
  this->ActualTimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  // this->DebugOn();
  this->SetNumberOfInputPorts(0);
  
  this->ShowEvents = false;
  this->ShowConversations = false;
  this->ShowMovements = false;
  this->ShowScenarios = false;
  this->ShowResults = false;
  this->ShowNodes = true;
  this->ShowAddresses = false;
}

//----------------------------------------------------------------------------
vtkNetDmfReader::~vtkNetDmfReader()
{
  delete this->Internal;

  H5garbage_collect();
}

void vtkNetDmfReader::SetFileName(const vtkStdString& fileName)
{
  this->FileName = fileName;
  this->Modified();
}
/*
const vtkStdString& vtkNetDmfReader::GetFileName()const
{
  return this->FileName;
}
*/
const char* vtkNetDmfReader::GetFileName()const
{
  return this->FileName.c_str();
}

//----------------------------------------------------------------------------
int vtkNetDmfReader::CanReadFile(const char* fname)
{
  vtkNetDmfReaderTester* tester = vtkNetDmfReaderTester::New();
  tester->SetFileName(fname);
  int res = tester->TestReadFile();
  tester->Delete();

  return res;
}

//----------------------------------------------------------------------------
/*
int vtkNetDmfReader::FillOutputPortInformation(int,
                                             vtkInformation *info)
{ 
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
  return 1;
}
*/
//----------------------------------------------------------------------------
void vtkNetDmfReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkNetDmfReader::ParseXML()
{
  cout << __FUNCTION__ << endl;
  // * Ensure that the required objects have been instantiated.
  if (!this->Internal->DOM)
    {
    this->Internal->DOM = new NetDMFDOM();
    }

  // * Check if the XML needs to be re-read.
  bool modified = false;
  // Parse the file...
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  if (this->FileName.empty() || 
      !vtksys::SystemTools::FileExists(this->FileName))
    {
    vtkErrorMacro("Can't read file: \"" << this->FileName.c_str() << "\"");
    return false;
    }
  
  modified = this->FileName != this->Internal->DOM->GetInputFileName() ||
    vtksys::SystemTools::ModifiedTime(this->FileName) > this->FileParseTime;
     
  if (modified)
    {
    //Tell the parser what the working directory is.
    vtkstd::string directory =
      vtksys::SystemTools::GetFilenamePath(this->FileName) + "/";
    if (directory == "/")
      {
      directory = vtksys::SystemTools::GetCurrentWorkingDirectory() + "/";
      }
    //    directory = vtksys::SystemTools::ConvertToOutputPath(directory.c_str());
    this->Internal->DOM->SetWorkingDirectory(directory.c_str());
    this->Internal->DOM->SetInputFileName(this->FileName);
    this->Internal->DOM->Parse(this->FileName);
    this->FileParseTime = vtksys::SystemTools::ModifiedTime(this->FileName);
    this->ParseTime.Modified();
    cout << __FUNCTION__ << " done" << endl;
    // Since the DOM was re-parsed we need to update the cache for domains
    // and re-populate the grids.
    //this->UpdateDomains();
    //this->UpdateRootGrid();
    }
  return true;
}

//----------------------------------------------------------------------------

int vtkNetDmfReader::RequestDataObject(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  cout << "RequestDataObject: " << endl;
  /*
  if (!this->ParseXML())
    {
    return 0;
    }
  
  for (XdmfXmlNode eventNode = this->Internal->DOM->FindNextRecursiveElement("Event");
       eventNode; 
       eventNode = this->Internal->DOM->FindNextRecursiveElement("Event", eventNode))
    {
    NetDMFEvent* event = new NetDMFEvent();
    event->SetDOM(this->Internal->DOM);
    event->SetElement(eventNode);
    event->UpdateInformation();
    if (event->GetStartTime())
      {
      this->Internal->TimeRange[0] = 
        MIN(this->Internal->TimeRange[0] ? this->Internal->TimeRange[0]
            : event->GetStartTime(), event->GetStartTime());
      }
    if (event->GetEndTime())
      {
      this->Internal->TimeRange[1] = 
        MAX(this->Internal->TimeRange[1] ? this->Internal->TimeRange[1]
            : event->GetEndTime(), event->GetEndTime());
      }
    }
  cout << " Time range: " << this->Internal->TimeRange[0] 
       << " " << this->Internal->TimeRange[1] << endl;
  */
/*
  
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
*/
  return 1;
}


//-----------------------------------------------------------------------------

int vtkNetDmfReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro("RequestInformation");
  
  if (!this->ParseXML())
    {
    return 0;
    }
  
  for (XdmfXmlNode eventNode = this->Internal->DOM->FindNextRecursiveElement("Event");
       eventNode; 
       eventNode = this->Internal->DOM->FindNextRecursiveElement("Event", eventNode))
    {
    NetDMFEvent* event = new NetDMFEvent();
    event->SetDOM(this->Internal->DOM);
    event->SetElement(eventNode);
    event->UpdateInformation();
    this->Internal->UpdateTimeRange(event);
    }
  cout << " Time range: " << static_cast<int>(this->Internal->TimeRange[0]) 
       << " " << static_cast<int>(this->Internal->TimeRange[1]) << endl;

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  //say we can produce as many pieces are are desired
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), 
               this->Internal->TimeRange, 2);
               
  /// missing things...
  this->ActualTimeStep = this->TimeStep;

  return 1;
}


//----------------------------------------------------------------------------
int vtkNetDmfReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // The file has been read in the RequestDataObject method.
  // Here we create the graph based on the parsed XML.
  this->ParseXML();
  //long int starttime = this->GetMTime();

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *outStructure = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outStructure)
    {
    vtkErrorMacro( << "No Output VTK structure");
    return VTK_ERROR;
    }

  this->Internal->UpdatePiece     = 0;
  this->Internal->UpdateNumPieces = 1;

  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    this->Internal->UpdatePiece = 
      outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    this->Internal->UpdateNumPieces =
      outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  
  vtkDebugMacro( << "UpdatePiece " << this->Internal->UpdatePiece 
                 << " : UpdateNumPieces " << this->Internal->UpdateNumPieces
                 );
  vtkMutableDirectedGraph* output = vtkMutableDirectedGraph::New();
  // Vertex Attribute Data Set
  vtkStringArray* names = vtkStringArray::New();
  names->SetName("Name");
  output->GetVertexData()->SetPedigreeIds(names);
  names->Delete();
  this->Internal->NameProperties = names;

  vtkStringArray* types = vtkStringArray::New();
  types->SetName("Type");
  output->GetVertexData()->AddArray(types);
  types->Delete();
  this->Internal->TypeProperties = types;

  vtkStringArray* classes = vtkStringArray::New();
  classes->SetName("Class");
  output->GetVertexData()->AddArray(classes);
  classes->Delete();
  this->Internal->ClassProperties = classes;

  // Vertex Attribute Data Set
  vtkIntArray* nodeIds = vtkIntArray::New();
  nodeIds->SetName("Id");
  output->GetVertexData()->AddArray(nodeIds);
  nodeIds->Delete();
  this->Internal->NodeIdProperties = nodeIds;

  vtkFloatArray* positions = vtkFloatArray::New();
  positions->SetName("Position");
  positions->SetNumberOfComponents(3);
  output->GetVertexData()->AddArray(positions);
  positions->Delete();
  this->Internal->PositionProperties = positions;

  // Edge Attribute Data Set
  vtkStringArray* conversationType = vtkStringArray::New();
  conversationType->SetName("ConversationType");
  output->GetEdgeData()->AddArray(conversationType);
  conversationType->Delete();

  //
  // Find the correct time step
  //
  this->ActualTimeStep = this->TimeStep;

  /*
  // Use in GetElementName()
  this->Internal->ElementCount.clear();
  for (XdmfXmlNode rootNode = this->Internal->DOM->FindRecursiveElement("NetDMF");
       rootNode; 
       rootNode = this->Internal->DOM->FindNextRecursiveElement("NetDMF", rootNode))
    {
    NetDMFRoot* root = new NetDMFRoot();
    root->SetDOM(this->Internal->DOM);
    root->SetElement(rootNode);
    root->UpdateInformation();
    
    this->AddNetDMFElement(output, root);
    //output->AddVertex();
    delete root;
    }
    
  */
  // Get the requested time step. We only supprt requests of a single time
  // step in this reader right now
  double currentTime = 0.;  
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *requestedTimeSteps = 
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int numReqTimeSteps = 
      outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    currentTime = (requestedTimeSteps && numReqTimeSteps)? requestedTimeSteps[0] : 0;
    // not sure why but currentTime is included between 0 and 1 ?!?!
    //currentTime = this->Internal->TimeRange[0] + currentTime * 
    //  (this->Internal->TimeRange[1] - this->Internal->TimeRange[0]);
    }

  this->Internal->ElementCount.clear();
  for (XdmfXmlNode nodeNode = this->Internal->DOM->FindRecursiveElement("Node");
       nodeNode; 
       nodeNode = this->Internal->DOM->FindNextRecursiveElement("Node", nodeNode))
    {
    NetDMFNode* node = new NetDMFNode();
    node->SetDOM(this->Internal->DOM);
    node->SetElement(nodeNode);
    node->UpdateInformation();
    
    this->AddNetDMFElement(output, node);
    //output->AddVertex();
    delete node;
    }

  for (XdmfXmlNode movementNode = this->Internal->DOM->FindRecursiveElement("Movement");
       movementNode; 
       movementNode = this->Internal->DOM->FindNextRecursiveElement("Movement", movementNode))
    {
    NetDMFMovement* movement = new NetDMFMovement();
    movement->SetDOM(this->Internal->DOM);
    movement->SetElement(movementNode);
    movement->UpdateInformation();
    // a movement node can be a nodeId ( == "1" ) or an element description
    // ( == "/NetDMF/Scenario/Node[@Name='foo']" )
    vtkIdType nodeVertexId = 
      this->Internal->NodeIdProperties->LookupValue(movement->GetNodeId());
    if (nodeVertexId == -1)
      {
      NetDMFNode* node = new NetDMFNode;
      node->SetDOM(this->Internal->DOM);
      node->SetElement(this->Internal->DOM->FindElementByPath(movement->GetNodeId()));
      node->UpdateInformation();
      nodeVertexId = this->Internal->NodeIdProperties->LookupValue(node->GetNodeId());
      }
    cout << " nodeId: " << movement->GetNodeId() << " " << nodeVertexId << endl;
    // TODO: support more than NETDMF_MOVEMENT_TYPE_PATH
    if (movement->GetMovementType() == NETDMF_MOVEMENT_TYPE_PATH)
      {
      XdmfDataItem* pathData = movement->GetPathData();
      pathData->Update();
      XdmfArray* dataArray = pathData->GetArray();
      int numberOfTuples = pathData->GetDataDesc()->GetDimension(0);
      int numberOfComponents = pathData->GetDataDesc()->GetDimension(1);
      double timeRange[2];
      this->Internal->GetMovementTimeRange(movement, timeRange);
      double index = static_cast<double>(numberOfTuples) * 
        ((currentTime - timeRange[0]) / (timeRange[1] - timeRange[0]));
      //cout << static_cast<int>(currentTime) << " " << static_cast<int>(timeRange[0])
      //     << " " << index << " " << static_cast<int>(index) <<endl;
      index = MAX(index, 0.);
      index = MIN(index, numberOfTuples-1);
      // TODO: interpolate the position between floor(index) and ceil(index)
      int offset[3] = {0, 1, 2};
      double position[3];
      position[0] = dataArray->GetValueAsFloat32(
        static_cast<int>(index) * numberOfComponents + offset[0]);
      position[1] = dataArray->GetValueAsFloat32(
        static_cast<int>(index) * numberOfComponents + offset[1]);
      position[2] = dataArray->GetValueAsFloat32(
        static_cast<int>(index) * numberOfComponents + offset[2]);
      cout << currentTime << " node: " << nodeVertexId << " index: " << index 
           << " p: " << position[0] << " " << position[1] << " " << position[2] << endl;
      if (nodeVertexId != -1)
        {
        this->Internal->PositionProperties->SetTuple(nodeVertexId, position);
        }
      }
    
    delete movement;
    }

  //long int endtime = this->GetMTime();
  vtkDirectedGraph::SafeDownCast(outStructure)->DeepCopy(output);
  output->Delete();
  
  return 1;
}

vtkStdString vtkNetDmfReader::GetElementName(NetDMFElement* element)
{
  int index = this->Internal->ElementCount[element->GetClassName()]++;
  if (element->GetName())
    {
    return element->GetName();
    }
  vtksys_ios::stringstream uid;
  uid << element->GetElementName() << index;
  return uid.str();
}

void vtkNetDmfReader::AddNetDMFElement(vtkMutableDirectedGraph* graph, 
                                       NetDMFElement* element, 
                                       vtkIdType parentVertexId/*=-1*/)
{
  
  VTK_CREATE(vtkVariantArray, properties);
  VTK_CREATE(vtkVariantArray, edgeProperties);
  vtkIdType elementVertexId = parentVertexId;

  vtkStdString elementName = this->GetElementName(element);

  // ROOT
  if (dynamic_cast<NetDMFRoot*>(element) != 0)
    {
    NetDMFRoot* root = dynamic_cast<NetDMFRoot*>(element);

    vtksys_ios::stringstream rootVersion;
    rootVersion << root->GetVersion();
    vtkStdString rootName(root->GetName()?root->GetName():"");
    vtkStdString rootType(rootVersion.str());

    properties->InsertNextValue(elementName);          // name
    properties->InsertNextValue(rootType);             // type
    properties->InsertNextValue(root->GetClassName()); // class
    elementVertexId = graph->AddVertex(properties);
    //elementVertexId = graph->AddVertex();

    // Get the scenario nodes
    for (XdmfXmlNode scenarioNode = this->Internal->DOM->FindNextRecursiveElement("Scenario", root->GetElement());
         scenarioNode; 
         scenarioNode = this->Internal->DOM->FindNextRecursiveElement("Scenario", scenarioNode, root->GetElement()))
      {
      NetDMFScenario* scenario = new NetDMFScenario();
      scenario->SetDOM(this->Internal->DOM);
      scenario->SetElement(scenarioNode);
      scenario->UpdateInformation();

      this->AddNetDMFElement(graph, scenario, elementVertexId);
      delete scenario;
      }

    // Get the result nodes
    for (XdmfXmlNode resultNode = this->Internal->DOM->FindNextRecursiveElement("Result", root->GetElement());
         resultNode; 
         resultNode = this->Internal->DOM->FindNextRecursiveElement("Result", resultNode, root->GetElement()))
      {
      NetDMFResult* result = new NetDMFResult();
      result->SetDOM(this->Internal->DOM);
      result->SetElement(resultNode);
      result->UpdateInformation();

      this->AddNetDMFElement(graph, result, elementVertexId);
      delete result;
      }

    }
  // SCENARIO
  else if (dynamic_cast<NetDMFScenario*>(element) != 0)
    {
    NetDMFScenario* scenario = dynamic_cast<NetDMFScenario*>(element);
    
    if (this->GetShowScenarios())
      {
      properties->InsertNextValue(elementName);              // name
      properties->InsertNextValue("");                       // type
      properties->InsertNextValue(scenario->GetClassName()); // class
      elementVertexId = graph->AddVertex(properties);
      }
    
    int numberOfNodes = scenario->GetNumberOfNodes();
    for (int i = 0; i < numberOfNodes; ++i)
      {
      this->AddNetDMFElement(graph, scenario->GetNode(i), elementVertexId);
      }
    }
  // RESULT
  else if (dynamic_cast<NetDMFResult*>(element) != 0)
    {
    NetDMFResult* result = dynamic_cast<NetDMFResult*>(element);
    
    if (this->GetShowResults())
      {
      properties->InsertNextValue(elementName);            // name
      properties->InsertNextValue("");                     // type
      properties->InsertNextValue(result->GetClassName()); // class
      elementVertexId = graph->AddVertex(properties);
      }

    int eventNumber = result->GetNumberOfEvents();
    for (int i = 0; i < eventNumber; ++i)
      {
      this->AddNetDMFElement(graph, result->GetEvent(i), elementVertexId);
      }
    }
  // NODE
  else if (dynamic_cast<NetDMFNode*>(element) != 0)
    {
    NetDMFNode* node = dynamic_cast<NetDMFNode*>(element);

    // make sure the name is not already added
    vtkStdString nodeName = elementName;
    if (this->GetShowNodes())
      {
      elementVertexId = graph->FindVertex(nodeName);
      if (elementVertexId == -1)
        {
        vtksys_ios::stringstream nodeId;
        nodeId << node->GetNodeId();
        vtkStdString nodeType(nodeId.str());
        
        this->Internal->NameProperties->InsertNextValue(elementName);          // name
        this->Internal->TypeProperties->InsertNextValue(nodeType);             // type
        this->Internal->ClassProperties->InsertNextValue(node->GetClassName()); // class
        this->Internal->NodeIdProperties->InsertNextValue(node->GetNodeId());
        this->Internal->PositionProperties->InsertNextTuple3(0., 0., 0.);
        elementVertexId = graph->AddVertex();
        }
      }

    int numberOfAddresses = node->GetNumberOfAddresses();
    for (int j=0; j < numberOfAddresses; ++j)
      {
      this->AddNetDMFElement(graph, node->GetAddress(j), elementVertexId);
      }
    }
  // ADDRESSITEM
  else if (dynamic_cast<NetDMFAddressItem*>(element))
    {
    NetDMFAddressItem* address = dynamic_cast<NetDMFAddressItem*>(element);
    // AddressItem::Update() is necessary. It retrieve the AddressItem data.
    address->Update();
    
    if (this->GetShowAddresses())
      {
      // addressType is typically: ETH, IP...
      vtkStdString addressType = address->GetAddressTypeAsString();
      edgeProperties->InsertNextValue(addressType);
    
      int numberOfAddresses = address->GetNumberOfAddresses();
      for (int j=0; j < numberOfAddresses;++j)
        {
        // make sure the address is not already added
        vtkStdString addressName = address->GetAddress(j);
        elementVertexId = graph->FindVertex(addressName);
        if (elementVertexId == -1)
          {
          properties->Initialize();
          properties->InsertNextValue(addressName);             // name
          properties->InsertNextValue(addressType);             // type
          properties->InsertNextValue(address->GetClassName()); // class
          elementVertexId = graph->AddVertex(properties);
          }
        if (parentVertexId != -1 && elementVertexId != -1)
          {
          graph->AddEdge(parentVertexId, elementVertexId, edgeProperties);
          }
        // set to invalid to don't add another edge at the end of the function.
        elementVertexId = -1;
        }
      }
    }
  // EVENT
  else if (dynamic_cast<NetDMFEvent*>(element))
    {
    NetDMFEvent* event = dynamic_cast<NetDMFEvent*>(element);
    vtkStdString eventType(event->GetEventTypeAsString());
    if (this->GetShowEvents())
      {
      properties->InsertNextValue(elementName);           // name
      properties->InsertNextValue(eventType);             // type
      properties->InsertNextValue(event->GetClassName()); // class
      elementVertexId = graph->AddVertex(properties);
      }

    int numberOfAddresses = event->GetNumberOfAddressItems();
    for (int j=0; j < numberOfAddresses; ++j)
      {
      this->AddNetDMFElement(graph, event->GetAddressItem(j), elementVertexId);
      }
    
    int numberOfEvents = event->GetNumberOfChildren();
    for (int j=0; j < numberOfEvents; ++j)
      {
      this->AddNetDMFElement(graph, event->GetChildEvent(j), elementVertexId);
      }
    // Conversations must be done after AddressItems and subEvent to make sure 
    // the addresses already exist.
    int numberOfConversations = event->GetNumberOfConversations();
    for (int j=0; j < numberOfConversations; ++j)
      {
      this->AddNetDMFElement(graph, event->GetConversation(j), elementVertexId);
      }
    int numberOfMovements = event->GetNumberOfMovements();
    for (int j=0; j < numberOfMovements; ++j)
      {
      this->AddNetDMFElement(graph, event->GetMovement(j), elementVertexId);
      }
    }
  // CONVERSATION
  else if (dynamic_cast<NetDMFConversation*>(element))
    {
    NetDMFConversation* conversation = dynamic_cast<NetDMFConversation*>(element);
    
    vtkStdString conversationType(conversation->GetConversationTypeAsString());
    if (this->GetShowConversations())
      {
      properties->InsertNextValue(elementName);                  // name
      properties->InsertNextValue(conversationType);             // type
      properties->InsertNextValue(conversation->GetClassName()); // class
      elementVertexId = graph->AddVertex(properties);
      }

    if (graph->FindVertex(conversation->GetEndPointA()) != -1 && 
        graph->FindVertex(conversation->GetEndPointB()) != -1)
      {
      edgeProperties->InsertNextValue(conversationType);
      if (this->GetShowConversations())
        {
        graph->AddEdge(elementVertexId, 
                       conversation->GetEndPointA(),
                       edgeProperties);
        graph->AddEdge(elementVertexId, 
                       conversation->GetEndPointB(),
                       edgeProperties);
        }
      else
        {
        graph->AddEdge(conversation->GetEndPointA(), 
                       conversation->GetEndPointB(),
                       edgeProperties);
        }
      edgeProperties->Initialize();
      }
    }
  // MOVEMENT
  else if (dynamic_cast<NetDMFMovement*>(element))
    {
    NetDMFMovement* movement = dynamic_cast<NetDMFMovement*>(element);
    
    vtkStdString movementType(movement->GetMovementTypeAsString());
    vtkIdType nodeVertexId = graph->GetVertexData()->GetAbstractArray("Type")
      ->LookupValue(movement->GetNodeId());

    if (this->GetShowMovements())
      {
      properties->InsertNextValue(elementName);              // name
      properties->InsertNextValue(movementType);             // type
      properties->InsertNextValue(movement->GetClassName()); // class
      elementVertexId = graph->AddVertex(properties);

      if (nodeVertexId != -1)
        {
        edgeProperties->InsertNextValue(movementType);    

        graph->AddEdge(elementVertexId, 
                       nodeVertexId,
                       edgeProperties);
        }
      }
    }   
  if (parentVertexId != -1 && 
      elementVertexId != -1 &&
      parentVertexId != elementVertexId)
    {
    if (edgeProperties->GetNumberOfValues()==0)
      {
      edgeProperties->InsertNextValue("");
      }
    graph->AddEdge(parentVertexId, elementVertexId, edgeProperties);
    }
}
