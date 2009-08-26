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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkNetDmfReader);
vtkCxxRevisionMacro(vtkNetDmfReader, "1.2");

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
  
  vtkstd::map<vtkStdString, int> ElementCount;
 
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

  this->DOM = 0;

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->TimeStep       = 0;
  this->ActualTimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  // this->DebugOn();
  this->SetNumberOfInputPorts(0);
  
  this->ShowEvents = false;
  this->ShowConversations = true;
  this->ShowMovements = true;
}

//----------------------------------------------------------------------------
vtkNetDmfReader::~vtkNetDmfReader()
{
  delete this->Internal;

  if ( this->DOM )
    {
    delete this->DOM;
    }

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
  if (!this->DOM)
    {
    this->DOM = new NetDMFDOM();
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
  
  modified = this->FileName != this->DOM->GetInputFileName() ||
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
    this->DOM->SetWorkingDirectory(directory.c_str());
    this->DOM->SetInputFileName(this->FileName);
    this->DOM->Parse(this->FileName);
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
/*
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
*/

//-----------------------------------------------------------------------------
/*
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
*/

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
  
  vtkStringArray* types = vtkStringArray::New();
  types->SetName("Type");
  output->GetVertexData()->AddArray(types);
  types->Delete();

  vtkStringArray* classes = vtkStringArray::New();
  classes->SetName("Class");
  output->GetVertexData()->AddArray(classes);
  classes->Delete();


  // Edge Attribute Data Set
  vtkStringArray* conversationType = vtkStringArray::New();
  conversationType->SetName("ConversationType");
  output->GetEdgeData()->AddArray(conversationType);
  conversationType->Delete();
    
  //
  // Find the correct time step
  //
  this->ActualTimeStep = this->TimeStep;
  
//  vtkVariantArray* propertyArr = vtkVariantArray::New();

  // Get the addressItems as vertexs
/*
  for (XdmfXmlNode addressNode = this->DOM->FindNextRecursiveElement("AddressItem");
       addressNode; 
       addressNode = this->DOM->FindNextRecursiveElement("AddressItem", addressNode))
    {
    //cout << " found Address : " << addressNode << endl;
    NetDMFAddressItem* address = new NetDMFAddressItem();
    address->SetDOM(this->DOM);
    address->SetElement(addressNode);
    address->UpdateInformation();
    address->Update();
    
    int numberOfAddresses = address->GetNumberOfAddresses();
    for (int j=0; j < numberOfAddresses;++j)
      {
      // make sure the address is not already added
      vtkStdString addressString = address->GetAddress(j);
      if (output->FindVertex(addressString)!=-1)
        {
        continue;
        }
      //cout << " Address : " << address->GetAddress(j) << endl;
      names->InsertNextValue(addressString);
      types->InsertNextValue(address->GetAddressTypeAsString());
      classes->InsertNextValue(address->GetClassName());
      output->AddVertex();
      }
    delete address;
    }
*/
  this->Internal->ElementCount.clear();
  for (XdmfXmlNode rootNode = this->DOM->FindRecursiveElement("NetDMF");
       rootNode; 
       rootNode = this->DOM->FindNextRecursiveElement("NetDMF", rootNode))
    {
    NetDMFRoot* root = new NetDMFRoot();
    root->SetDOM(this->DOM);
    root->SetElement(rootNode);
    root->UpdateInformation();
    
    this->AddNetDMFElement(output, root);
    //output->AddVertex();
    delete root;
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
  uid << element->GetClassName() << index;
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
    for (XdmfXmlNode scenarioNode = this->DOM->FindNextRecursiveElement("Scenario", root->GetElement());
         scenarioNode; 
         scenarioNode = this->DOM->FindNextRecursiveElement("Scenario", scenarioNode, root->GetElement()))
      {
      NetDMFScenario* scenario = new NetDMFScenario();
      scenario->SetDOM(this->DOM);
      scenario->SetElement(scenarioNode);
      scenario->UpdateInformation();

      this->AddNetDMFElement(graph, scenario, elementVertexId);
      delete scenario;
      }

    // Get the result nodes
    for (XdmfXmlNode resultNode = this->DOM->FindNextRecursiveElement("Result", root->GetElement());
         resultNode; 
         resultNode = this->DOM->FindNextRecursiveElement("Result", resultNode, root->GetElement()))
      {
      NetDMFResult* result = new NetDMFResult();
      result->SetDOM(this->DOM);
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
    
    properties->InsertNextValue(elementName);              // name
    properties->InsertNextValue("");                       // type
    properties->InsertNextValue(scenario->GetClassName()); // class
    elementVertexId = graph->AddVertex(properties);

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
    
    properties->InsertNextValue(elementName);            // name
    properties->InsertNextValue("");                     // type
    properties->InsertNextValue(result->GetClassName()); // class
    elementVertexId = graph->AddVertex(properties);

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
    elementVertexId = graph->FindVertex(nodeName);
    if (elementVertexId == -1)
      {
      vtksys_ios::stringstream nodeId;
      nodeId << node->GetNodeId();
      vtkStdString nodeType(nodeId.str());

      properties->InsertNextValue(elementName);          // name
      properties->InsertNextValue(nodeType);             // type
      properties->InsertNextValue(node->GetClassName()); // class
      elementVertexId = graph->AddVertex(properties);
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
        //names->InsertNextValue(addressString);
        //types->InsertNextValue(address->GetAddressTypeAsString());
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
    edgeProperties->InsertNextValue(movementType);    

    if (this->GetShowMovements())
      {
      properties->InsertNextValue(elementName);              // name
      properties->InsertNextValue(movementType);             // type
      properties->InsertNextValue(movement->GetClassName()); // class
      elementVertexId = graph->AddVertex(properties);
      }

    vtkIdType nodeVertexId = graph->GetVertexData()->GetAbstractArray("Type")
      ->LookupValue(movement->GetNodeId());
    if (nodeVertexId != -1)
      {
      graph->AddEdge(elementVertexId, 
                     nodeVertexId,
                     edgeProperties);
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
