#include "vtkPythonRepresentation.h"

#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkAppendRectilinearGrid.h"
#include "vtkClientServerMoveData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataGroupFilter.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVMergeTables.h"
#include "vtkPVSession.h"
#include "vtkPassArrays.h"
#include "vtkProcessModule.h"
#include "vtkPythonView.h"
#include "vtkReductionFilter.h"

#include <map>

class vtkPythonRepresentation::vtkPythonRepresentationInternal
{
public:
  vtkPythonRepresentationInternal()
  {
    // One map per attribute type
    this->AttributeArrayEnabled.resize(vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES);
  }

  std::vector<std::map<std::string, bool> > AttributeArrayEnabled;
};

vtkStandardNewMacro(vtkPythonRepresentation);
//----------------------------------------------------------------------------
vtkPythonRepresentation::vtkPythonRepresentation()
{
  this->LocalInput = nullptr;
  this->ClientDataObject = nullptr;
  this->Internal = new vtkPythonRepresentationInternal();
}

//----------------------------------------------------------------------------
vtkPythonRepresentation::~vtkPythonRepresentation()
{
  if (this->LocalInput)
  {
    this->LocalInput->Delete();
  }
  if (this->ClientDataObject)
  {
    this->ClientDataObject->Delete();
  }
  if (this->Internal)
  {
    delete this->Internal;
  }
}

//----------------------------------------------------------------------------
int vtkPythonRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (this->GetVisibility() == false)
  {
    return 0;
  }

  if (request_type == vtkPythonView::REQUEST_DELIVER_DATA_TO_CLIENT())
  {
    this->TransferLocalDataToClient();
  }

  return this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo);
}

//----------------------------------------------------------------------------
int vtkPythonRepresentation::GetNumberOfAttributeArrays(int attributeType)
{
  if (this->LocalInput)
  {
    vtkFieldData* attributeData = this->LocalInput->GetAttributesAsFieldData(attributeType);
    if (attributeData)
    {
      return attributeData->GetNumberOfArrays();
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkPythonRepresentation::GetAttributeArrayName(int attributeType, int arrayIndex)
{
  if (this->LocalInput)
  {
    vtkFieldData* attributeData = this->LocalInput->GetAttributesAsFieldData(attributeType);
    if (attributeData)
    {
      if (arrayIndex < 0 || arrayIndex >= attributeData->GetNumberOfArrays())
      {
        vtkErrorMacro(<< "Invalid array index " << arrayIndex);
        return nullptr;
      }
      return attributeData->GetArrayName(arrayIndex);
    }
    else
    {
      vtkErrorMacro(<< "No attribute "
                    << vtkDataSetAttributes::GetAttributeTypeAsString(attributeType)
                    << " available in the input");
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPythonRepresentation::SetAttributeArrayStatus(
  int attributeType, const char* name, int status)
{
  std::string nameStr(name);
  this->Internal->AttributeArrayEnabled[attributeType][name] = (status != 0);
}

//----------------------------------------------------------------------------
int vtkPythonRepresentation::GetAttributeArrayStatus(int attributeType, const char* name)

{
  std::string nameStr(name);
  if (this->Internal->AttributeArrayEnabled[attributeType].count(nameStr) > 0)
  {
    return this->Internal->AttributeArrayEnabled[attributeType][name];
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPythonRepresentation::EnableAllAttributeArrays()
{
  for (int attributeType = 0; attributeType < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
       ++attributeType)
  {
    for (int i = 0; i < this->GetNumberOfAttributeArrays(attributeType); ++i)
    {
      const char* attributeName = this->GetAttributeArrayName(attributeType, i);
      this->SetAttributeArrayStatus(attributeType, attributeName, 1);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPythonRepresentation::DisableAllAttributeArrays()
{
  for (size_t attributeType = 0; attributeType < this->Internal->AttributeArrayEnabled.size();
       ++attributeType)
  {
    this->Internal->AttributeArrayEnabled[attributeType].clear();
  }
}

//----------------------------------------------------------------------------
int vtkPythonRepresentation::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//#define VTK_PYTHON_REPRESENTATION_DEBUG

#if defined(VTK_PYTHON_REPRESENTATION_DEBUG)
#define vtkPythonRepresentationDebug(x)                                                            \
  {                                                                                                \
    vtkMultiProcessController* globalController =                                                  \
      vtkMultiProcessController::GetGlobalController();                                            \
                                                                                                   \
    int myId = globalController->GetLocalProcessId();                                              \
    int numProcs = globalController->GetNumberOfProcesses();                                       \
    std::cout << "Proc (" << myId << ", " << numProcs << "): " << x << std::endl;                  \
  }
#else
#define vtkPythonRepresentationDebug(x)
#endif

//----------------------------------------------------------------------------
int vtkPythonRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPythonRepresentationDebug("Starting RequestData");

  // Make a shallow copy of the input data
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  if (input)
  {
    if (!this->LocalInput)
    {
      vtkPythonRepresentationDebug(
        "Creating new instance of " << input->GetClassName() << " for LocalInput");
      this->LocalInput = input->NewInstance();
    }
    else if (!this->LocalInput->IsA(input->GetClassName()))
    {
      vtkPythonRepresentationDebug(
        "Replacing LocalInput with new instance of " << input->GetClassName());
      this->LocalInput->Delete();
      this->LocalInput = input->NewInstance();
    }
    else
    {
      this->LocalInput->Initialize();
    }
    this->LocalInput->ShallowCopy(input);
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkPythonRepresentation::InitializePreGatherHelper(
  vtkReductionFilter* reductionFilter, vtkDataObject* input)
{
  if (input == nullptr)
  {
    reductionFilter->SetPreGatherHelper(nullptr);
    return;
  }

  if (input->IsA("vtkPolyData") || input->IsA("vtkTable") || input->IsA("vtkRectilinearGrid") ||
    input->IsA("vtkStructuredGrid") || input->IsA("vtkUnstructuredGrid") ||
    input->IsA("vtkCompositeDataSet"))
  {
    vtkPythonRepresentationDebug("vtkPassArrays used as pregather helper in ReductionFilter");
    vtkPassArrays* passArraysHelper = vtkPassArrays::New();
    passArraysHelper->UseFieldTypesOn();
    reductionFilter->SetPreGatherHelper(passArraysHelper);
    passArraysHelper->Delete();

    // Set which arrays to pass
    for (int attributeType = 0; attributeType < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
         ++attributeType)
    {
      passArraysHelper->AddFieldType(attributeType);
      std::map<std::string, bool>::iterator arrayIter =
        this->Internal->AttributeArrayEnabled[attributeType].begin();
      while (arrayIter != this->Internal->AttributeArrayEnabled[attributeType].end())
      {
        std::string arrayName = arrayIter->first;
        int enabled = arrayIter->second;
        if (enabled) // enabled
        {
          passArraysHelper->AddArray(attributeType, arrayName.c_str());
        }
        ++arrayIter;
      }
    }
  }
  else
  {
    vtkErrorMacro(<< "Unhandled input data type '" << input->GetClassName()
                  << "' for deciding pre-gather helper");
    reductionFilter->SetPreGatherHelper(nullptr);
  }
}

//----------------------------------------------------------------------------
void vtkPythonRepresentation::InitializePostGatherHelper(
  vtkReductionFilter* reductionFilter, vtkDataObject* input)
{
  if (input == nullptr)
  {
    reductionFilter->SetPostGatherHelper(nullptr);
    return;
  }

  // Set up the correct filter for putting together the datasets on
  // different server nodes.
  if (input->IsA("vtkPolyData"))
  {
    vtkPythonRepresentationDebug("vtkAppendPolyData used in ReductionFilter");
    vtkAppendPolyData* appendPolyDataHelper = vtkAppendPolyData::New();
    reductionFilter->SetPostGatherHelper(appendPolyDataHelper);
    appendPolyDataHelper->Delete();
  }
  else if (input->IsA("vtkCompositeDataSet"))
  {
    vtkPythonRepresentationDebug("vtkMultiBlockDataGroupFilter used in ReductionFilter");
    vtkMultiBlockDataGroupFilter* groupHelper = vtkMultiBlockDataGroupFilter::New();
    reductionFilter->SetPostGatherHelper(groupHelper);
    groupHelper->Delete();
  }
  else if (input->IsA("vtkRectilinearGrid"))
  {
    vtkPythonRepresentationDebug("vtkAppendRectilinearGrid used in ReductionFilter");
    vtkAppendRectilinearGrid* gridHelper = vtkAppendRectilinearGrid::New();
    reductionFilter->SetPostGatherHelper(gridHelper);
    gridHelper->Delete();
  }
  else if (input->IsA("vtkDataSet"))
  {
    vtkPythonRepresentationDebug("vtkAppendFilter used in ReductionFilter");
    vtkAppendFilter* appendHelper = vtkAppendFilter::New();
    reductionFilter->SetPostGatherHelper(appendHelper);
    appendHelper->Delete();
  }
  else if (input->IsA("vtkTable"))
  {
    vtkPythonRepresentationDebug("vtkPVMergeTable used in ReductionFilter");
    vtkPVMergeTables* tablesHelper = vtkPVMergeTables::New();
    reductionFilter->SetPostGatherHelper(tablesHelper);
    tablesHelper->Delete();
  }
  else
  {
    vtkErrorMacro(<< "Unhandled input data type '" << input->GetClassName()
                  << "' for deciding post-gather helper");
    reductionFilter->SetPostGatherHelper(nullptr);
  }
}

//----------------------------------------------------------------------------
bool vtkPythonRepresentation::HasProcessRole(vtkTypeUInt32 role)
{
  vtkPVSession* session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession());
  if (!session)
  {
    vtkErrorMacro("No active ParaView session");
    return 0;
  }

  return session->HasProcessRole(role);
}

//----------------------------------------------------------------------------
bool vtkPythonRepresentation::IsClientProcess()
{
  return this->HasProcessRole(vtkPVSession::CLIENT);
}

//----------------------------------------------------------------------------
bool vtkPythonRepresentation::IsDataServerProcess()
{
  return this->HasProcessRole(vtkPVSession::DATA_SERVER);
}

//----------------------------------------------------------------------------
int vtkPythonRepresentation::SendDataTypeToClient(int& dataType)
{
  vtkMultiProcessController* controller = nullptr;
  int processType = 0;
  enum
  {
    CLIENT,
    SERVER
  };

  vtkPVSession* session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession());
  if (!session)
  {
    vtkErrorMacro("No active ParaView session");
    return 0;
  }
  if (this->IsClientProcess())
  {
    controller = session->GetController(vtkPVSession::DATA_SERVER);
    processType = CLIENT;
  }
  // else if (vtkProcessModule::GetProcessType() ==
  //         vtkProcessModule::PROCESS_DATA_SERVER)
  else if (this->IsDataServerProcess())
  {
    controller = session->GetController(vtkPVSession::CLIENT);
    processType = SERVER;
  }

  int tag = 353848;
  if (controller)
  {
    if (processType == SERVER)
    {
      vtkDebugMacro("Server Root: Send data type to the client.");
      controller->Send(&dataType, 1, 1, tag);
    }
    else if (processType == CLIENT)
    {
      vtkDebugMacro("Client: Get data type from server.");
      controller->Receive(&dataType, 1, 1, tag);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonRepresentation::TransferLocalDataToClient()
{
  vtkSmartPointer<vtkReductionFilter> reductionFilter = vtkSmartPointer<vtkReductionFilter>::New();
  reductionFilter->SetController(vtkMultiProcessController::GetGlobalController());

  vtkSmartPointer<vtkClientServerMoveData> dataMover =
    vtkSmartPointer<vtkClientServerMoveData>::New();

  int dataType = -1;
  if (this->LocalInput)
  {
    vtkDataObject* input = this->LocalInput;
    if (input)
    {
      dataType = input->GetDataObjectType();

      // Tell the reduction filter how to process data on the server
      // nodes before merging it.
      this->InitializePreGatherHelper(reductionFilter, input);

      // Tell the reduction filter how it should merge data after gathering it
      this->InitializePostGatherHelper(reductionFilter, input);

      // The reduction should happen only among processes with input
      // data, i.e. server processes.
      vtkPythonRepresentationDebug("Setting input on ReductionFilter");
      reductionFilter->SetInputData(input);
      reductionFilter->Update();
      vtkPythonRepresentationDebug("Setting input to DataMover");
      dataMover->SetInputConnection(reductionFilter->GetOutputPort());
    }
    else
    {
      vtkErrorMacro(<< "No input data");
    }
  }

  // Tell DataMover what the output type of the data should be
  this->SendDataTypeToClient(dataType);
  vtkPythonRepresentationDebug(
    "dataType is " << vtkDataObjectTypes::GetClassNameFromTypeId(dataType));
  dataMover->SetOutputDataType(dataType);

  if (this->IsClientProcess() || this->IsDataServerProcess())
  {
    dataMover->Update();
  }

  // Copy on the client side
  if (this->IsClientProcess())
  {
    if (this->ClientDataObject)
    {
      this->ClientDataObject->Delete();
    }

    this->ClientDataObject = vtkDataObjectTypes::NewDataObject(dataType);
    if (this->ClientDataObject)
    {
      this->ClientDataObject->ShallowCopy(dataMover->GetOutput());
    }
    else
    {
      vtkErrorMacro(<< "Could not create LocalInput on client");
    }
  }
}

//----------------------------------------------------------------------------
void vtkPythonRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
