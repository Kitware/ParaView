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

#include "vtkCharArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPLYReader.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"
#include "vtkXMLParser.h"

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

//============================================================================
class vtkNetDmfReaderInternal
{
public:
  vtkNetDmfReaderInternal()
  {
    this->DOM = 0;
    this->TimeRange[0] = 0.;
    this->TimeRange[1] = 0.;
    
    this->NameProperties = 0;
    this->TypeProperties = 0;
    this->ClassProperties = 0;
    this->NodeIdProperties = 0;
    this->DirectionProperties = 0;
  }

  ~vtkNetDmfReaderInternal()
  {
    if ( this->DOM )
      {
      delete this->DOM;
      }
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
  vtkFloatArray*   DirectionProperties;

  double       TimeRange[2];

  unsigned int UpdatePiece;
  unsigned int UpdateNumPieces;

};

//============================================================================
void vtkNetDmfReaderInternal::UpdateTimeRange(NetDMFEvent* event)
{
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
  double endTime = 
    startTime + movement->GetMovementInterval() * pathData->GetDataDesc()->GetDimension(0);
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

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  
  this->CurrentTimeStep = 0;
}

//----------------------------------------------------------------------------
vtkNetDmfReader::~vtkNetDmfReader()
{
  delete this->Internal;

  H5garbage_collect();
}

//----------------------------------------------------------------------------
void vtkNetDmfReader::SetFileName(const vtkStdString& fileName)
{
  this->FileName = fileName;
  this->Modified();
}

//----------------------------------------------------------------------------
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
void vtkNetDmfReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkNetDmfReader::ParseXML()
{
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
    }
  return true;
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

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  //say we can produce as many pieces are are desired
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), 
               this->Internal->TimeRange, 2);
           
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

  vtkFloatArray* directions = vtkFloatArray::New();
  directions->SetName("Direction");
  directions->SetNumberOfComponents(3);
  output->GetVertexData()->AddArray(directions);
  directions->Delete();
  this->Internal->DirectionProperties = directions;

  vtkPoints* points = vtkPoints::New();
  output->SetPoints(points);
  points->Delete();

  // Edge Attribute Data Set
  vtkStringArray* conversationType = vtkStringArray::New();
  conversationType->SetName("ConversationType");
  output->GetEdgeData()->AddArray(conversationType);
  conversationType->Delete();

  // Get the requested time step. We only supprt requests of a single time
  // step in this reader right now
  this->CurrentTimeStep = 0.;  
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *requestedTimeSteps = 
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int numReqTimeSteps = 
      outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    this->CurrentTimeStep = (requestedTimeSteps && numReqTimeSteps)? 
      requestedTimeSteps[0] : 0;
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

    // make sure the name is not already added
    vtkStdString nodeName = this->GetElementName(node);
    vtkIdType elementVertexId = output->FindVertex(nodeName);
    if (elementVertexId == -1)
      {
      vtksys_ios::stringstream nodeId;
      nodeId << node->GetNodeId();
      vtkStdString nodeType(nodeId.str());
        
      this->Internal->NameProperties->InsertNextValue(nodeName);          // name
      this->Internal->TypeProperties->InsertNextValue(nodeType);             // type
      this->Internal->ClassProperties->InsertNextValue(node->GetClassName()); // class
      this->Internal->NodeIdProperties->InsertNextValue(node->GetNodeId());  // nodeId
      this->Internal->DirectionProperties->InsertNextTuple3(0., 0., 0.);     // directions
      points->InsertNextPoint(0., 0., 0.);
      elementVertexId = output->AddVertex();
      }

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
        ((this->CurrentTimeStep - timeRange[0]) / (timeRange[1] - timeRange[0]));
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
      if (nodeVertexId != -1)
        {
        points->SetPoint(nodeVertexId, position);
        }
      double direction[3];
      if (index < numberOfTuples-2)
        {
        direction[0] = dataArray->GetValueAsFloat32(
          static_cast<int>(index+1) * numberOfComponents + offset[0]) - position[0];
        direction[1] = dataArray->GetValueAsFloat32(
          static_cast<int>(index+1) * numberOfComponents + offset[1]) - position[1];
        direction[2] = dataArray->GetValueAsFloat32(
          static_cast<int>(index+1) * numberOfComponents + offset[2]) - position[2];
        }
      else
        {
        direction[0] = position[0] - dataArray->GetValueAsFloat32(
          static_cast<int>(index-1) * numberOfComponents + offset[0]);
        direction[1] = position[1] - dataArray->GetValueAsFloat32(
          static_cast<int>(index-1) * numberOfComponents + offset[1]);
        direction[2] = position[2] - dataArray->GetValueAsFloat32(
          static_cast<int>(index-1) * numberOfComponents + offset[2]);
        }
      if (nodeVertexId != -1)
        {
        vtkMath::Normalize(direction);
        /* From v5.py (doesn't work)
        double degX = -4.5;
        double degY = -vtkMath::DegreesFromRadians(asin(direction[2]))- 2.2;
        double degZ = vtkMath::DegreesFromRadians(acos(direction[1]));
        if (direction[0] < 0)
          {
          degZ = -degZ;
          }
        directions->SetTuple3(nodeVertexId, degX, degY, degZ);
        */
        // Rotations are in the order: RotateZ, RotateX and RotateY.
        directions->SetTuple3(nodeVertexId, 0., 0.,
                              vtkMath::DegreesFromRadians(atan2(direction[1],direction[0])));
        }
      }
    
    delete movement;
    }

  //long int endtime = this->GetMTime();
  vtkDirectedGraph::SafeDownCast(outStructure)->ShallowCopy(output);
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
