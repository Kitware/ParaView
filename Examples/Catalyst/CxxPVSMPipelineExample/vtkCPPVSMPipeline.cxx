#include "vtkCPPVSMPipeline.h"

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCommunicator.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPVTrivialProducer.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMWriterProxy.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include <sstream>
#include <string>

vtkStandardNewMacro(vtkCPPVSMPipeline);

//----------------------------------------------------------------------------
vtkCPPVSMPipeline::vtkCPPVSMPipeline()
{
  this->OutputFrequency = 0;
}

//----------------------------------------------------------------------------
vtkCPPVSMPipeline::~vtkCPPVSMPipeline()
{
}

//----------------------------------------------------------------------------
void vtkCPPVSMPipeline::Initialize(int outputFrequency, std::string& fileName)
{
  this->OutputFrequency = outputFrequency;
  this->FileName = fileName;
}

//----------------------------------------------------------------------------
int vtkCPPVSMPipeline::RequestDataDescription(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("dataDescription is NULL.");
    return 0;
  }

  if (this->FileName.empty())
  {
    vtkWarningMacro("No output file name given to output results to.");
    return 0;
  }

  if (dataDescription->GetForceOutput() == true ||
    (this->OutputFrequency != 0 && dataDescription->GetTimeStep() % this->OutputFrequency == 0))
  {
    dataDescription->GetInputDescriptionByName("input")->AllFieldsOn();
    dataDescription->GetInputDescriptionByName("input")->GenerateMeshOn();
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkCPPVSMPipeline::CoProcess(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("DataDescription is NULL");
    return 0;
  }
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(
    dataDescription->GetInputDescriptionByName("input")->GetGrid());
  if (grid == nullptr)
  {
    vtkWarningMacro("DataDescription is missing input unstructured grid.");
    return 0;
  }
  if (this->RequestDataDescription(dataDescription) == 0)
  {
    return 1;
  }

  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* sessionProxyManager = proxyManager->GetActiveSessionProxyManager();

  // Create a vtkPVTrivialProducer and set its output
  // to be the input grid.
  vtkSmartPointer<vtkSMSourceProxy> producer;
  producer.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("sources", "PVTrivialProducer")));
  producer->UpdateVTKObjects();
  vtkObjectBase* clientSideObject = producer->GetClientSideObject();
  vtkPVTrivialProducer* realProducer = vtkPVTrivialProducer::SafeDownCast(clientSideObject);
  realProducer->SetOutput(grid);

  // Create a slice filter and set the cut type to plane
  vtkSmartPointer<vtkSMSourceProxy> slice;
  slice.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->NewProxy("filters", "Cut")));
  vtkSMInputProperty* sliceInputConnection =
    vtkSMInputProperty::SafeDownCast(slice->GetProperty("Input"));

  vtkSMProxyProperty* cutType = vtkSMProxyProperty::SafeDownCast(slice->GetProperty("CutFunction"));
  vtkSmartPointer<vtkSMProxy> cutPlane;
  cutPlane.TakeReference(sessionProxyManager->NewProxy("implicit_functions", "Plane"));
  cutPlane->UpdatePropertyInformation();
  cutPlane->UpdateVTKObjects();
  cutType->SetProxy(0, cutPlane);
  cutType->UpdateAllInputs();

  // We set 4 offsets, i.e. 4 parallel cut planes for the slice filter
  vtkSMDoubleVectorProperty* offsets =
    vtkSMDoubleVectorProperty::SafeDownCast(slice->GetProperty("ContourValues"));
  offsets->SetElements4(1, 11, 21, 31);

  producer->UpdateVTKObjects();
  sliceInputConnection->SetInputConnection(0, producer, 0);
  slice->UpdatePropertyInformation();
  slice->UpdateVTKObjects();

  // Finally, create the parallel poly data writer, set the
  // filename and then update the pipeline.
  vtkSmartPointer<vtkSMWriterProxy> writer;
  writer.TakeReference(
    vtkSMWriterProxy::SafeDownCast(sessionProxyManager->NewProxy("writers", "XMLPPolyDataWriter")));
  vtkSMInputProperty* writerInputConnection =
    vtkSMInputProperty::SafeDownCast(writer->GetProperty("Input"));
  writerInputConnection->SetInputConnection(0, slice, 0);
  vtkSMStringVectorProperty* fileName =
    vtkSMStringVectorProperty::SafeDownCast(writer->GetProperty("FileName"));

  std::ostringstream o;
  o << dataDescription->GetTimeStep();
  std::string name = this->FileName + o.str() + ".pvtp";

  fileName->SetElement(0, name.c_str());
  writer->UpdatePropertyInformation();
  writer->UpdateVTKObjects();
  writer->UpdatePipeline();

  return 1;
}

//----------------------------------------------------------------------------
void vtkCPPVSMPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputFrequency: " << this->OutputFrequency << "\n";
  os << indent << "FileName: " << this->FileName << "\n";
}
