/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDReaderModule.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

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
#include "vtkPVDReaderModule.h"

#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVScale.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDReaderModule);
vtkCxxRevisionMacro(vtkPVDReaderModule, "1.2.2.1");

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
  int fixme;
  
  // Check whether the input file has a "timestep" attribute.
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->ServerScript("%s UpdateAttributes", this->GetVTKSourceID());
  pm->RootScript(
    "namespace eval ::paraview::vtkPVDReaderModule {\n"
    "  proc GetTimeAttributeIndex {reader} {\n"
    "    set n [$reader GetNumberOfAttributes]\n"
    "    for {set i 0} {$i < $n} {incr i} {\n"
    "      if {[$reader GetAttributeName $i] == {timestep}} {\n"
    "        return $i\n"
    "      }\n"
    "    }\n"
    "    return -1\n"
    "  }\n"
    "  GetTimeAttributeIndex {%s}\n"
    "}\n", this->GetVTKSourceTclName());
  int index = atoi(pm->GetRootResult());
  this->HaveTime = (index >= 0)?1:0;

  // If we have time, we need to behave as an advanced reader module.
  if(this->HaveTime)
    {
    pm->ServerScript("%s SetRestrictionAsIndex timestep 0",
                     this->GetVTKSourceTclName());
    pm->RootScript("%s GetNumberOfAttributeValues %d",
                   this->GetVTKSourceTclName(), index);
    int max = atoi(pm->GetRootResult())-1;
    this->TimeScale = vtkPVScale::New();
    this->TimeScale->SetLabel("Timestep");
    this->TimeScale->SetPVSource(this);
    this->TimeScale->RoundOn();
    this->TimeScale->SetRange(0, max);
    this->TimeScale->SetParent(this->GetParameterFrame()->GetFrame());
    this->TimeScale->SetModifiedCommand(this->GetTclName(), 
                                        "SetAcceptButtonColorToRed");
    this->TimeScale->SetVariableName("RestrictionAsIndex timestep");
    this->TimeScale->Create(this->GetPVApplication());
    this->TimeScale->DisplayEntry();
    this->TimeScale->SetDisplayEntryAndLabelOnTop(0);
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
