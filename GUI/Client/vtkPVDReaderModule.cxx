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
vtkCxxRevisionMacro(vtkPVDReaderModule, "1.15");

//----------------------------------------------------------------------------
vtkPVDReaderModule::vtkPVDReaderModule()
{
}

//----------------------------------------------------------------------------
vtkPVDReaderModule::~vtkPVDReaderModule()
{

}

//----------------------------------------------------------------------------
void vtkPVDReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVDReaderModule::Finalize(const char* fname)
{
  vtkPVScale *scale = vtkPVScale::SafeDownCast( this->GetPVWidget("TimeStep") );
// If we have more than 1 timestep, we need to behave as an advanced reader module.
  if(scale && scale->GetRangeMax() > 0)
    {
    return this->Superclass::Finalize(fname);
    }
  else
    {
    return this->vtkPVReaderModule::Finalize(fname);
    }
}

int vtkPVDReaderModule::GetNumberOfTimeSteps()
{
  vtkPVScale *scale = vtkPVScale::SafeDownCast( this->GetPVWidget("TimeStep") );
  if(scale && scale->GetRangeMax() > 0)
    {
    return static_cast<int>(scale->GetRangeMax() - scale->GetRangeMin()) + 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVDReaderModule::SetRequestedTimeStep(int step)
{
  vtkPVScale *scale = vtkPVScale::SafeDownCast( this->GetPVWidget("TimeStep") );
  if(scale && scale->GetRangeMax() > 0)
    {
    scale->SetValue(step + scale->GetRangeMin());
    this->AcceptCallback();
    this->GetPVApplication()->GetMainView()->EventuallyRender();
    this->Script("update");
    }
  else
    {
    vtkErrorMacro("Cannot call SetRequestedTimeStep with no time steps.");
    }
}
