/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDReaderModule.h"

#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVScale.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDReaderModule);
vtkCxxRevisionMacro(vtkPVDReaderModule, "1.11");

//----------------------------------------------------------------------------
vtkPVDReaderModule::vtkPVDReaderModule()
{
  this->HaveTime = 0;
  this->TimeScale = 0;
}

//----------------------------------------------------------------------------
vtkPVDReaderModule::~vtkPVDReaderModule()
{
  if(this->TimeScale)
    {
    this->TimeScale->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVDReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVDReaderModule::Finalize(const char* fname)
{
  // If we have time, we need to behave as an advanced reader module.
  if(this->HaveTime)
    {
    return this->Superclass::Finalize(fname);
    }
  else
    {
    return this->vtkPVReaderModule::Finalize(fname);
    }
}

//----------------------------------------------------------------------------
int vtkPVDReaderModule::ReadFileInformation(const char* fname)
{
  // Make sure the reader's file name is set.
  this->SetReaderFileName(fname);

  // Check whether the input file has a "timestep" attribute.
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    // Since this is a reader, there is only one VTK source. Therefore,
    // we use index 0.
    << this->GetVTKSourceID(0) << "UpdateAttributes"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER);
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->GetVTKSourceID(0) << "GetAttributeIndex" << "timestep"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
  int index = -1;
  this->HaveTime = (pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &index) &&
                    index >= 0)? 1 : 0;

  // If we have time, we need to behave as an advanced reader module.
  if(this->HaveTime)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->GetVTKSourceID(0) << "SetRestrictionAsIndex"
      << "timestep" << 0
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->GetVTKSourceID(0) << "GetNumberOfAttributeValues" << index
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    int numValues = 0;
    if(!pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &numValues))
      {
      vtkErrorMacro("Error getting number of timesteps from reader.");
      numValues = 0;
      }
    this->TimeScale = vtkPVScale::New();
    this->TimeScale->SetLabel("Timestep");
    this->TimeScale->SetPVSource(this);
    this->TimeScale->RoundOn();
    this->TimeScale->SetRange(0, numValues-1);
    this->TimeScale->SetParent(this->GetParameterFrame()->GetFrame());
    this->TimeScale->SetModifiedCommand(this->GetTclName(), 
                                        "SetAcceptButtonColorToModified");
    this->TimeScale->SetVariableName("TimestepAsIndex");
    this->TimeScale->SetDisplayEntryAndLabelOnTop(0);
    this->TimeScale->SetDisplayValueFlag(0);
    this->TimeScale->Create(this->GetPVApplication());
    this->TimeScale->DisplayEntry();
    this->AddPVWidget(this->TimeScale);
    this->Script("pack %s -side top -fill x -expand 1", 
                 this->TimeScale->GetWidgetName());
    return this->Superclass::ReadFileInformation(fname);
    }
  else
    {
    return this->vtkPVReaderModule::ReadFileInformation(fname);
    }
}

//----------------------------------------------------------------------------
int vtkPVDReaderModule::GetNumberOfTimeSteps()
{
  if(this->HaveTime)
    {
    return static_cast<int>(this->TimeScale->GetRangeMax() -
                            this->TimeScale->GetRangeMin())+1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVDReaderModule::SetRequestedTimeStep(int step)
{
  if(this->HaveTime)
    {
    this->TimeScale->SetValue(step + this->TimeScale->GetRangeMin());
    this->AcceptCallback();
    this->GetPVApplication()->GetMainView()->EventuallyRender();
    this->Script("update");
    }
  else
    {
    vtkErrorMacro("Cannot call SetRequestedTimeStep with no time steps.");
    }
}
