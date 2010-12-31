/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVAlgorithmPortsInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMessage.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#define OUTPUT_PORTNAME_PREFIX "Output-"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSourceProxy);

// This struct will keep all information associated with the output port.
struct vtkSMSourceProxyOutputPort
{
  vtkSmartPointer<vtkSMOutputPort> Port;
  vtkSmartPointer<vtkSMDocumentation> Documentation;
  vtkstd::string Name;
};

struct vtkSMSourceProxyInternals
{
  typedef vtkstd::vector<vtkSMSourceProxyOutputPort> VectorOfPorts;
  VectorOfPorts OutputPorts;

  // Resizes output ports and ensures that Name for each port is initialized to
  // the default.
  void ResizeOutputPorts(unsigned int newsize)
    {
    this->OutputPorts.resize(newsize);
    VectorOfPorts::iterator it = this->OutputPorts.begin();
    for (unsigned int idx=0; it != this->OutputPorts.end(); it++, idx++)
      {
      if (it->Name.empty())
        {
        vtksys_ios::ostringstream nameStream;
        nameStream << OUTPUT_PORTNAME_PREFIX << idx;
        it->Name = nameStream.str();
        }
      }
    }

  void EnsureOutputPortsSize(unsigned int size)
    {
    if (this->OutputPorts.size() < size)
      {
      this->ResizeOutputPorts(size);
      }
    }
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->SetKernelClassName("vtkPMSourceProxy");

  this->PInternals = new  vtkSMSourceProxyInternals;
  this->OutputPortsCreated = 0;

  this->ExecutiveName = 0;
  this->SetExecutiveName("vtkCompositeDataPipeline");

  this->DoInsertExtractPieces = 1;

  this->NumberOfAlgorithmOutputPorts = VTK_UNSIGNED_INT_MAX;
  this->NumberOfAlgorithmRequiredInputPorts = VTK_UNSIGNED_INT_MAX;
  this->ProcessSupport = vtkSMSourceProxy::BOTH;
}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;

  this->SetExecutiveName(0);
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfOutputPorts()
{
  return this->PInternals->OutputPorts.size();
}

//---------------------------------------------------------------------------
vtkSMOutputPort* vtkSMSourceProxy::GetOutputPort(unsigned int idx)
{
  return idx==VTK_UNSIGNED_INT_MAX? NULL:
    this->PInternals->OutputPorts[idx].Port.GetPointer();
}

//---------------------------------------------------------------------------
vtkSMOutputPort* vtkSMSourceProxy::GetOutputPort(const char* portname)
{
  return this->GetOutputPort(this->GetOutputPortIndex(portname));
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetOutputPortIndex(const char* portname)
{
  // Since there are not really going to be hundreds of output ports, the is not
  // going to be any noticable difference in accessing output ports by index or
  // by name.
  vtkSMSourceProxyInternals::VectorOfPorts::iterator it =
    this->PInternals->OutputPorts.begin();
  for (unsigned int idx=0; it != this->PInternals->OutputPorts.end(); it++, idx++)
    {
    if (it->Name == portname)
      {
      return idx;
      }
    }

  return VTK_UNSIGNED_INT_MAX;
}

//---------------------------------------------------------------------------
const char* vtkSMSourceProxy::GetOutputPortName(unsigned int index)
{
  if (index >= this->PInternals->OutputPorts.size())
    {
    return 0;
    }
  
  return this->PInternals->OutputPorts[index].Name.c_str();
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSourceProxy::GetOutputPortDocumentation(
  unsigned int index)
{
  if (index >= this->PInternals->OutputPorts.size())
    {
    return 0;
    }

  return this->PInternals->OutputPorts[index].Documentation;
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSourceProxy::GetOutputPortDocumentation(
  const char* portname)
{
  return this->GetOutputPortDocumentation(
    this->GetOutputPortIndex(portname));
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdatePipelineInformation()
{
  if (this->ObjectsCreated)
    {
    vtkSMMessage message;
    message << pvstream::InvokeRequest() << "UpdateInformation";
    this->Invoke(&message);
    }

  // This is no longer applicable since subproxies don't exists on the
  // client-side.
  // This simply iterates over subproxies and calls UpdatePropertyInformation();
  // this->Superclass::UpdatePipelineInformation();

  this->InvokeEvent(vtkCommand::UpdateInformationEvent);
  // this->MarkModified(this);  
}
//---------------------------------------------------------------------------
int vtkSMSourceProxy::ReadXMLAttributes(vtkSMProxyManager* pm, 
                                        vtkPVXMLElement* element)
{
  const char* executiveName = element->GetAttribute("executive");
  if (executiveName)
    {
    this->SetExecutiveName(executiveName);
    }
  const char* mp = element->GetAttribute("multiprocess_support");
  if (mp)
    {
    if (strcmp(mp, "multiple_processes") == 0)
      {
      this->ProcessSupport = vtkSMSourceProxy::MULTIPLE_PROCESSES;
      }
    else if (strcmp(mp, "single_process") == 0)
      {
      this->ProcessSupport = vtkSMSourceProxy::SINGLE_PROCESS;
      }
    else
      {
      this->ProcessSupport = vtkSMSourceProxy::BOTH;
      }
    }

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (child && child->GetName() && 
      strcmp(child->GetName(), "OutputPort")==0)
      {
      // Load output port configuration.
      int index;
      if (!child->GetScalarAttribute("index", &index))
        {
        vtkErrorMacro("Missing OutputPort attribute 'index'.");
        return 0;
        }
      const char* portname= child->GetAttribute("name");
      if (!portname)
        {
        vtkErrorMacro("Missing OutputPort attribute 'name'.");
        return 0;
        }
      this->PInternals->EnsureOutputPortsSize(index+1); 
      this->PInternals->OutputPorts[index].Name = portname;

      // Load output port documentation.
      for (unsigned int kk=0; kk < child->GetNumberOfNestedElements(); ++kk)
        {
        vtkPVXMLElement* subElem = child->GetNestedElement(kk);
        if (strcmp(subElem->GetName(), "Documentation") == 0)
          {
          this->Documentation->SetDocumentationElement(subElem);
          vtkSMDocumentation* doc = vtkSMDocumentation::New();
          doc->SetDocumentationElement(subElem);
          this->PInternals->OutputPorts[index].Documentation = doc;
          doc->Delete();
          }
        }
      }
    }
  return this->Superclass::ReadXMLAttributes(pm, element);
}

//---------------------------------------------------------------------------
// Call Update() on all sources
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline()
{
  if (!this->NeedsUpdate)
    {
    return;
    }

  this->CreateOutputPorts(); 
  int num = this->GetNumberOfOutputPorts();
  for (int i=0; i < num; ++i)
    {
    this->GetOutputPort(i)->UpdatePipeline();
    }

  this->PostUpdateData();
  //this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
// Call Update() on all sources with given time
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline(double time)
{
  int i;

  this->CreateOutputPorts();
  int num = this->GetNumberOfOutputPorts();
  for (i=0; i < num; ++i)
    {
    this->GetOutputPort(i)->UpdatePipeline(time);
    }

  this->PostUpdateData();
  //this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();

  // We are going to fix the ports such that we don't have to update the
  // pipeline or even UpdateInformation() to create the ports.
  this->CreateOutputPorts();
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfAlgorithmOutputPorts()
{
  if (this->NumberOfAlgorithmOutputPorts != VTK_UNSIGNED_INT_MAX)
    {
    // avoids unnecessary information gathers.
    return this->NumberOfAlgorithmOutputPorts;
    }

  if (this->ObjectsCreated)
    {
    vtkSmartPointer<vtkPVAlgorithmPortsInformation> info =
      vtkSmartPointer<vtkPVAlgorithmPortsInformation>::New();
    this->GatherInformation(info);
    this->NumberOfAlgorithmOutputPorts = info->GetNumberOfOutputs();
    this->NumberOfAlgorithmRequiredInputPorts = info->GetNumberOfRequiredInputs();
    return this->NumberOfAlgorithmOutputPorts;
    }

  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfAlgorithmRequiredInputPorts()
{
  this->GetNumberOfAlgorithmOutputPorts();

  if (this->NumberOfAlgorithmRequiredInputPorts != VTK_UNSIGNED_INT_MAX)
    {
    // avoid unnecessary information gathers.
    return this->NumberOfAlgorithmRequiredInputPorts;
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateOutputPorts()
{
  if (this->OutputPortsCreated && this->GetNumberOfOutputPorts())
    {
    return;
    }
  this->OutputPortsCreated = 1;

  // This will only create objects if they are not already created.
  // This happens when connecting a filter to a source which is not
  // initialized. In other situations, SetInput() creates the VTK
  // objects before this gets called.
  this->CreateVTKObjects();

  // We simply set/replace the Port pointers in the
  // this->PInternals->OutputPorts. This ensures that the port names,
  // port documentation is preserved.

  // Create one output port proxy for each output of the filter
  int numOutputs = this->GetNumberOfAlgorithmOutputPorts();

  // Ensure that output ports size matches the number of output ports provided
  // by the algorithm.
  this->PInternals->ResizeOutputPorts(numOutputs);
  for (int j=0; j<numOutputs; j++)
    {
    vtkSMOutputPort* opPort = vtkSMOutputPort::New();
    opPort->SetPortIndex(j);
    opPort->SetSourceProxy(this);
    this->PInternals->OutputPorts[j].Port = opPort;
    opPort->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::SetOutputPort(unsigned int index, const char* name, 
  vtkSMOutputPort* port, vtkSMDocumentation* doc)
{
  this->PInternals->EnsureOutputPortsSize(index+1);
  this->PInternals->OutputPorts[index].Name = name;
  this->PInternals->OutputPorts[index].Port = port;
  this->PInternals->OutputPorts[index].Documentation = doc;
  if (port && port->GetSourceProxy() == NULL)
    {
    port->SetSourceProxy(this);
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::RemoveAllOutputPorts()
{
  this->PInternals->OutputPorts.clear();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::PostUpdateData()
{
  this->InvalidateDataInformation();
  this->Superclass::PostUpdateData();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetDataInformation(
  unsigned int idx)
{
  this->CreateOutputPorts();
  if (idx >= this->GetNumberOfOutputPorts())
    {
    return 0;
    }

  return this->GetOutputPort(idx)->GetDataInformation();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::InvalidateDataInformation()
{
  if (this->OutputPortsCreated)
    {
    vtkSMSourceProxyInternals::VectorOfPorts::iterator it =
      this->PInternals->OutputPorts.begin();
    for (; it != this->PInternals->OutputPorts.end(); it++)
      {
      it->Port.GetPointer()->InvalidateDataInformation();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputPortsCreated: " << this->OutputPortsCreated << endl;
  os << indent << "ProcessSupport: " << this->ProcessSupport << endl;
}
