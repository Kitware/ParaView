/*=========================================================================

  Program:   ParaView
  Module:    vtkCPXMLPWriterPipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPXMLPWriterPipeline.h"

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

#include <algorithm>
#include <sstream>
#include <string>

namespace
{
void SetWholeExtent(
  vtkDataObject* grid, vtkCPInputDataDescription* idd, vtkPVTrivialProducer* realProducer)
{
  if (grid->IsA("vtkImageData") || grid->IsA("vtkRectilinearGrid") ||
    grid->IsA("vtkStructuredGrid"))
  {
    realProducer->SetWholeExtent(idd->GetWholeExtent());
  }
}

const char* GetWriterName(vtkDataObject* grid)
{
  std::string name = grid->GetClassName();
  if (name == "vtkImageData")
  {
    return "XMLPImageDataWriter";
  }
  else if (name == "vtkRectilinearGrid")
  {
    return "XMLPRectilinearGridWriter";
  }
  else if (name == "vtkStructuredGrid")
  {
    return "XMLPStructuredGridWriter";
  }
  else if (name == "vtkPolyData")
  {
    return "XMLPPolyDataWriter";
  }
  else if (name == "vtkUnstructuredGrid")
  {
    return "XMLPUnstructuredGridWriter";
  }
  else if (name == "vtkUniformGridAMR")
  {
    return "XMLHierarchicalBoxDataWriter";
  }
  else if (name == "vtkMultiBlockDataSet")
  {
    return "XMLMultiBlockDataWriter";
  }
  vtkGenericWarningMacro("Unknown dataset type " << name);
  return nullptr;
}

const char* GetWriterFileNameExtension(vtkDataObject* grid)
{
  std::string name = grid->GetClassName();
  if (name == "vtkImageData")
  {
    return "pvti";
  }
  else if (name == "vtkRectilinearGrid")
  {
    return "pvtr";
  }
  else if (name == "vtkStructuredGrid")
  {
    return "pvts";
  }
  else if (name == "vtkPolyData")
  {
    return "pvts";
  }
  else if (name == "vtkUnstructuredGrid")
  {
    return "pvtu";
  }
  else if (name == "vtkUniformGridAMR")
  {
    return "vthb";
  }
  else if (name == "vtkMultiBlockDataSet")
  {
    return "vtm";
  }
  vtkGenericWarningMacro("Unknown dataset type " << name);
  return nullptr;
}
} // end anonymous namespace

vtkStandardNewMacro(vtkCPXMLPWriterPipeline);

//----------------------------------------------------------------------------
vtkCPXMLPWriterPipeline::vtkCPXMLPWriterPipeline()
{
  this->OutputFrequency = 1;
  this->PaddingAmount = 0;
}

//----------------------------------------------------------------------------
vtkCPXMLPWriterPipeline::~vtkCPXMLPWriterPipeline() = default;

//----------------------------------------------------------------------------
int vtkCPXMLPWriterPipeline::RequestDataDescription(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("dataDescription is NULL.");
    return 0;
  }

  if (dataDescription->GetForceOutput() == true ||
    dataDescription->GetTimeStep() % this->OutputFrequency == 0)
  {
    for (unsigned int i = 0; i < dataDescription->GetNumberOfInputDescriptions(); i++)
    {
      dataDescription->GetInputDescription(i)->AllFieldsOn();
      dataDescription->GetInputDescription(i)->GenerateMeshOn();
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkCPXMLPWriterPipeline::CoProcess(vtkCPDataDescription* dataDescription)
{
  if (!dataDescription)
  {
    vtkWarningMacro("DataDescription is NULL");
    return 0;
  }

  if (this->RequestDataDescription(dataDescription) == 0)
  {
    return 1;
  }

  int retVal = 1;

  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* sessionProxyManager = proxyManager->GetActiveSessionProxyManager();

  for (unsigned int i = 0; i < dataDescription->GetNumberOfInputDescriptions(); i++)
  {
    std::string inputName = dataDescription->GetInputDescriptionName(i);
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescription(i);
    vtkDataObject* grid = idd->GetGrid();
    if (grid == nullptr)
    {
      vtkErrorMacro("Could not output " << inputName);
      retVal = 0;
    }
    else
    {
      // Create a vtkPVTrivialProducer and set its output
      // to be the input grid.
      vtkSmartPointer<vtkSMSourceProxy> producer;
      producer.TakeReference(vtkSMSourceProxy::SafeDownCast(
        sessionProxyManager->NewProxy("sources", "PVTrivialProducer")));
      producer->UpdateVTKObjects();
      vtkObjectBase* clientSideObject = producer->GetClientSideObject();
      vtkPVTrivialProducer* realProducer = vtkPVTrivialProducer::SafeDownCast(clientSideObject);
      realProducer->SetOutput(grid);
      SetWholeExtent(grid, idd, realProducer);

      if (const char* writerName = GetWriterName(grid))
      {
        vtkSmartPointer<vtkSMWriterProxy> writer;
        writer.TakeReference(
          vtkSMWriterProxy::SafeDownCast(sessionProxyManager->NewProxy("writers", writerName)));
        vtkSMInputProperty* writerInputConnection =
          vtkSMInputProperty::SafeDownCast(writer->GetProperty("Input"));
        writerInputConnection->SetInputConnection(0, producer, 0);
        vtkSMStringVectorProperty* fileName =
          vtkSMStringVectorProperty::SafeDownCast(writer->GetProperty("FileName"));

        // If we have a / in the channel name we take it out of the filename we're going to write to
        inputName.erase(std::remove(inputName.begin(), inputName.end(), '/'), inputName.end());
        std::ostringstream o;
        if (this->Path.empty() == false)
        {
          o << this->Path << "/";
        }
        o << inputName << "_" << std::setw(this->PaddingAmount) << std::setfill('0')
          << dataDescription->GetTimeStep() << "." << GetWriterFileNameExtension(grid);

        fileName->SetElement(0, o.str().c_str());
        writer->UpdatePropertyInformation();
        writer->UpdateVTKObjects();
        writer->UpdatePipeline();
      }
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
void vtkCPXMLPWriterPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputFrequency: " << this->OutputFrequency << "\n";
  os << indent << "PaddingAmount: " << this->PaddingAmount << "\n";
  if (this->Path.empty())
  {
    os << indent << "Path: (empty)\n";
  }
  else
  {
    os << indent << "Path: " << this->Path << "\n";
  }
}
