/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYPlotActorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMXYPlotActorProxy.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkClientServerID.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPart.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkMath.h"
vtkStandardNewMacro(vtkSMXYPlotActorProxy);
vtkCxxRevisionMacro(vtkSMXYPlotActorProxy, "1.1.2.1");
vtkCxxSetObjectMacro(vtkSMXYPlotActorProxy, Input, vtkSMSourceProxy);

//-----------------------------------------------------------------------------
vtkSMXYPlotActorProxy::vtkSMXYPlotActorProxy()
{
  this->Input = 0;
}

//-----------------------------------------------------------------------------
vtkSMXYPlotActorProxy::~vtkSMXYPlotActorProxy()
{
  this->SetInput(0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::AddInput(vtkSMSourceProxy* input,
  const char* method, int portIdx, int hasMultipleInputs)
{
  if (!input)
    {
    return;
    }
  input->CreateParts();
  this->SetInput(input);
  this->CreateVTKObjects(1);

/*
  int numInputs = input->GetNumberOfParts();
  this->CreateVTKObjects(1);
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID sourceID = this->GetID(0);

  int partIdx;
 
  int total_numArrays = 0;
  // Determine total number of arrays from each part.
  for (partIdx = 0; partIdx  < numInputs; partIdx++)
    {
    vtkSMPart* part = input->GetPart(partIdx);
    vtkPVDataInformation* dataInfo = part->GetDataInformation();
    vtkPVDataSetAttributesInformation* pdi = dataInfo->
      GetPointDataInformation();
    total_numArrays += pdi->GetNumberOfArrays();
    }

  if (total_numArrays == 0)
    {
    vtkErrorMacro("No arrays in PointData. Cannot plot input!");
    return;
    }
  
  // To assign unique plot color to each array.
  double color_step = 1.0 / total_numArrays;
  double color = 0;
  
  int arrayCount = 0;
  const char* arrayname;

  // This feels like a very improper place to build the XYplot inputs.
  // In this case, if the dataset arrays change, one needs to set the input again
  // (even if the input object hasn't been replaced, but merely filtered differently).
  // Probably I should do this in MarkConsumersAsModified (or Update).
  for (partIdx = 0; partIdx < numInputs; ++partIdx)
    {
    vtkSMPart* part = input->GetPart(partIdx);
    vtkPVDataInformation* dataInfo = part->GetDataInformation();
    vtkPVDataSetAttributesInformation* pdi = dataInfo->
      GetPointDataInformation();
    int numArrays = pdi->GetNumberOfArrays();
    for (int arr = 0; arr < numArrays; ++arr)
      {
      vtkPVArrayInformation* arrayInfo = pdi->GetArrayInformation(arr);
      arrayname = arrayInfo->GetName();
      if (arrayname && arrayInfo->GetNumberOfComponents() != 1)
        {
        continue; // it's very easy to extend it to plot all components,
                  // but I will leave that for the time being, as I am
                  // not sure that's what we want.
        }
      stream << vtkClientServerStream::Invoke
        << sourceID  << method  << part->GetID(0) << arrayname << 0
        << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
        << sourceID << "SetPlotLabel" << arrayCount << arrayname
        << vtkClientServerStream::End;

      double r, g , b;
      vtkMath::HSVToRGB(color, 1.0, 1.0, &r, &g, &b);

      stream << vtkClientServerStream::Invoke
        << sourceID << "SetPlotColor"
        << arrayCount << r << g << b 
        << vtkClientServerStream::End;

      color += color_step;
      arrayCount++;
      }
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("LegendVisibility"));
  if (ivp)
    {
    ivp->SetElement(0, ( (arrayCount > 1)? 1 : 0));
    }
  else
    {
    vtkErrorMacro("Failed to find property LegendVisibility.");
    }
  if (arrayCount == 1)
    {
    stream << vtkClientServerStream::Invoke
      << sourceID << "SetYTitle" << arrayname 
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << sourceID << "SetPlotColor" << 0 << 1 << 1 << 1
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->GetServers(), stream);
  this->UpdateVTKObjects();
  */
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
