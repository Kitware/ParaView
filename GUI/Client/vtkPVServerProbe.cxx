/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerProbe.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerProbe.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPProbeFilter.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVProcessModule.h"
#include "vtkSocketController.h"

vtkStandardNewMacro(vtkPVServerProbe);
vtkCxxRevisionMacro(vtkPVServerProbe, "1.1");

#define PV_TAG_PROBE_OUTPUT 759362

vtkPVServerProbe::vtkPVServerProbe()
{
}

vtkPVServerProbe::~vtkPVServerProbe()
{
}

void vtkPVServerProbe::SendPolyDataOutput(vtkPProbeFilter* probe)
{
  // Make sure we have a process module and probe
  if (this->ProcessModule && probe)
    {
    vtkPVClientServerModule *csm =
      vtkPVClientServerModule::SafeDownCast(this->ProcessModule);
    if (csm)
      {
      csm->GetSocketController()->Send(probe->GetOutput(), 1,
                                       PV_TAG_PROBE_OUTPUT);
      }
    else
      {
      vtkErrorMacro("SendPolyDataOutput should not be called except in "
                    << "client/server mode");
      }
    }
  else
    {
    if (!this->ProcessModule)
      {
      vtkErrorMacro("SendPolyDataOutput requires a ProcessModule.");
      }
    if (!probe)
      {
      vtkErrorMacro("SendPolyDataOutput cannot work with a NULL probe.");
      }
    }
}

void vtkPVServerProbe::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
