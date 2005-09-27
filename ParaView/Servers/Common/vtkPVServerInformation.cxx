/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerInformation.h"

#include "vtkClientServerStream.h"
#include "vtkPVConfig.h"
#include "vtkPVProcessModule.h"
#include "vtkPVServerOptions.h"
#include "vtkPVServerOptionsInternals.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"

vtkStandardNewMacro(vtkPVServerInformation);
vtkCxxRevisionMacro(vtkPVServerInformation, "1.6");

//----------------------------------------------------------------------------
vtkPVServerInformation::vtkPVServerInformation()
{
  this->RemoteRendering = 1;
  this->TileDimensions[0] = this->TileDimensions[1] = 0;
  this->UseOffscreenRendering = 0;
#if defined(PARAVIEW_USE_ICE_T) && defined(VTK_USE_MPI)
  this->UseIceT = 1;
#else
  this->UseIceT = 0;
#endif
  this->RenderModuleName = NULL;
  this->MachinesInternals = new vtkPVServerOptionsInternals;
}

//----------------------------------------------------------------------------
vtkPVServerInformation::~vtkPVServerInformation()
{
  this->SetRenderModuleName(NULL);
  delete this->MachinesInternals;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RemoteRendering: " << this->RemoteRendering << endl;
  os << indent << "UseOffscreenRendering: " << this->UseOffscreenRendering << endl;
  os << indent << "TileDimensions: " << this->TileDimensions[0]
     << ", " << this->TileDimensions[1] << endl;
  os << indent << "UseIceT: " << this->UseIceT << endl;
  os << indent << "RenderModuleName: "
     << (this->RenderModuleName ? this->RenderModuleName : "(none)") << endl;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::DeepCopy(vtkPVServerInformation *info)
{
  this->RemoteRendering = info->GetRemoteRendering();
  info->GetTileDimensions(this->TileDimensions);
  this->UseOffscreenRendering = info->GetUseOffscreenRendering();
  this->UseIceT = info->GetUseIceT();
  this->SetRenderModuleName(info->GetRenderModuleName());
  this->SetNumberOfMachines(info->GetNumberOfMachines());
  unsigned int idx;
  for (idx = 0; idx < info->GetNumberOfMachines(); idx++)
    {
    this->SetEnvironment(idx, info->GetEnvironment(idx));
    this->SetLowerLeft(idx, info->GetLowerLeft(idx));
    this->SetLowerRight(idx, info->GetLowerRight(idx));
    this->SetUpperLeft(idx, info->GetUpperLeft(idx));
    }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromObject(vtkObject* obj)
{
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(obj);
  if(!pm)
    {
    vtkErrorMacro("Cannot downcast to vtkPVProcessModule.");
    return;
    }
    
  this->DeepCopy(pm->GetServerInformation());

  vtkPVServerOptions *options =
    vtkPVServerOptions::SafeDownCast(pm->GetOptions());
  options->GetTileDimensions(this->TileDimensions);
  this->UseOffscreenRendering = options->GetUseOffscreenRendering();
  this->SetRenderModuleName(options->GetRenderModuleName());
  this->SetNumberOfMachines(options->GetNumberOfMachines());
  unsigned int idx;
  for (idx = 0; idx < options->GetNumberOfMachines(); idx++)
    {
    this->SetEnvironment(idx, options->GetDisplayName(idx));
    this->SetLowerLeft(idx, options->GetLowerLeft(idx));
    this->SetLowerRight(idx, options->GetLowerRight(idx));
    this->SetUpperLeft(idx, options->GetUpperLeft(idx));
    }
}

//----------------------------------------------------------------------------
// Consider an option added if it is a non-default option that the user
// has probably selected.
void vtkPVServerInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVServerInformation* serverInfo;
  serverInfo = vtkPVServerInformation::SafeDownCast(info);
  if (serverInfo)
    {
    if (!serverInfo->GetRemoteRendering())
      {
      this->RemoteRendering = 0;
      }
    if (serverInfo->GetTileDimensions()[0])
      {
      serverInfo->GetTileDimensions(this->TileDimensions);
      }
    if (serverInfo->GetUseOffscreenRendering())
      {
      this->UseOffscreenRendering = 1;
      }
    // ICE-T either is there or is not.
    this->UseIceT = serverInfo->GetUseIceT();
    this->SetRenderModuleName(serverInfo->GetRenderModuleName());
    this->SetNumberOfMachines(serverInfo->GetNumberOfMachines());
    unsigned int idx;
    for (idx = 0; idx < serverInfo->GetNumberOfMachines(); idx++)
      {
      this->SetEnvironment(idx, serverInfo->GetEnvironment(idx));
      this->SetLowerLeft(idx, serverInfo->GetLowerLeft(idx));
      this->SetLowerRight(idx, serverInfo->GetLowerRight(idx));
      this->SetUpperLeft(idx, serverInfo->GetUpperLeft(idx));
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->RemoteRendering;
  *css << this->TileDimensions[0] << this->TileDimensions[1];
  *css << this->UseOffscreenRendering;
  *css << this->UseIceT;
  *css << this->RenderModuleName;
  *css << this->GetNumberOfMachines();
  unsigned int idx;
  for (idx = 0; idx < this->GetNumberOfMachines(); idx++)
    {
    *css << this->GetEnvironment(idx);
    *css << this->GetLowerLeft(idx)[0] << this->GetLowerLeft(idx)[1]
         << this->GetLowerLeft(idx)[2];
    *css << this->GetLowerRight(idx)[0] << this->GetLowerRight(idx)[1]
         << this->GetLowerRight(idx)[2];
    *css << this->GetUpperLeft(idx)[0] << this->GetUpperLeft(idx)[1]
         << this->GetUpperLeft(idx)[2];
    }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromStream(const vtkClientServerStream* css)
{
  if(!css->GetArgument(0, 0, &this->RemoteRendering))
    {
    vtkErrorMacro("Error parsing RemoteRendering from message.");
    return;
    }
  if(   !css->GetArgument(0, 1, &this->TileDimensions[0])
     || !css->GetArgument(0, 2, &this->TileDimensions[1]) )
    {
    vtkErrorMacro("Error parsing TileDimensions from message.");
    return;
    }
  if(!css->GetArgument(0, 3, &this->UseOffscreenRendering))
    {
    vtkErrorMacro("Error parsing UseOffscreenRendering from message.");
    return;
    }
  if (!css->GetArgument(0, 4, &this->UseIceT))
    {
    vtkErrorMacro("Error parsing ICE-T flag from message.");
    return;
    }
  const char *rmName;
  if (!css->GetArgument(0, 5, &rmName))
    {
    vtkErrorMacro("Error parsing render module name from message.");
    return;
    }
  this->SetRenderModuleName(rmName);
  unsigned int numMachines;
  if (!css->GetArgument(0, 6, &numMachines))
    {
    vtkErrorMacro("Error parsing number of machines from message.");
    return;
    }
  this->SetNumberOfMachines(numMachines);
  unsigned int idx;
  const char* env;
  for (idx = 0; idx < numMachines; idx++)
    {
    if (!css->GetArgument(0, 7 + idx*10, &env))
      {
      vtkErrorMacro("Error parsing display environment from message.");
      return;
      }
    this->MachinesInternals->MachineInformationVector[idx].Environment = env;
    if (!css->GetArgument(0, 8 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[0]) ||
        !css->GetArgument(0, 9 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[1]) ||
        !css->GetArgument(0, 10 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[2]))
      {
      vtkErrorMacro("Error parsing lower left coordinate from message.");
      return;
      }
    if (!css->GetArgument(0, 11 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerRight[0]) ||
        !css->GetArgument(0, 12 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerRight[1]) ||
        !css->GetArgument(0, 13 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerRight[2]))
      {
      vtkErrorMacro("Error parsing lower right coordinate from message.");
      return;
      }
    if (!css->GetArgument(0, 14 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].UpperLeft[0]) ||
        !css->GetArgument(0, 15 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].UpperLeft[1]) ||
        !css->GetArgument(0, 16 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].UpperLeft[2]))
      {
      vtkErrorMacro("Error parsing upper left coordinate from message.");
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetNumberOfMachines(unsigned int num)
{
  delete this->MachinesInternals;
  this->MachinesInternals = new vtkPVServerOptionsInternals;
  unsigned int idx;
  vtkPVServerOptionsInternals::MachineInformation info;
  for (idx = 0; idx < num; idx++)
    {
    this->MachinesInternals->MachineInformationVector.push_back(info);
    }
}

//----------------------------------------------------------------------------
unsigned int vtkPVServerInformation::GetNumberOfMachines() const
{
  return this->MachinesInternals->MachineInformationVector.size();
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetEnvironment(unsigned int idx,
                                            const char* name)
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].Environment = name;
}

//----------------------------------------------------------------------------
const char* vtkPVServerInformation::GetEnvironment(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].Environment.c_str();
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetLowerLeft(unsigned int idx, double coord[3])
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].LowerLeft[0] = coord[0];
  this->MachinesInternals->MachineInformationVector[idx].LowerLeft[1] = coord[1];
  this->MachinesInternals->MachineInformationVector[idx].LowerLeft[2] = coord[2];
}

//----------------------------------------------------------------------------
double* vtkPVServerInformation::GetLowerLeft(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].LowerLeft;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetLowerRight(unsigned int idx, double coord[3])
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].LowerRight[0] = coord[0];
  this->MachinesInternals->MachineInformationVector[idx].LowerRight[1] = coord[1];
  this->MachinesInternals->MachineInformationVector[idx].LowerRight[2] = coord[2];
}

//----------------------------------------------------------------------------
double* vtkPVServerInformation::GetLowerRight(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].LowerRight;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetUpperLeft(unsigned int idx, double coord[3])
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].UpperLeft[0] = coord[0];
  this->MachinesInternals->MachineInformationVector[idx].UpperLeft[1] = coord[1];
  this->MachinesInternals->MachineInformationVector[idx].UpperLeft[2] = coord[2];
}

//----------------------------------------------------------------------------
double* vtkPVServerInformation::GetUpperLeft(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].UpperLeft;
}
