/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerOptions.h"
#include "vtkObjectFactory.h"
#include "vtkPVServerOptionsInternals.h"
#include "vtksys/ios/sstream"

#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerOptions);

//----------------------------------------------------------------------------
vtkPVServerOptions::vtkPVServerOptions()
{
  this->Internals = new vtkPVServerOptionsInternals;
}

//----------------------------------------------------------------------------
vtkPVServerOptions::~vtkPVServerOptions()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::Initialize()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
int vtkPVServerOptions::AddMachineInformation(const char** atts)
{
  vtkPVServerOptionsInternals::MachineInformation info;
  int caveBounds = 0;
  for (int i = 0; atts[i] && atts[i + 1]; i += 2)
    {
    std::string key = atts[i];
    std::string value = atts[i + 1];
    if(key == "Name")
      {
      info.Name = value;
      }
    else if(key == "Environment")
      {
      info.Environment = value;
      }
    else if(key == "LowerLeft")
      {
      caveBounds++;
      vtksys_ios::istringstream str(const_cast<char *>(value.c_str()));
      for(int j =0; j < 3; j++)
        {
        str >> info.LowerLeft[j];
        }
      }
    else if(key == "LowerRight")
      {
      caveBounds++;
      vtksys_ios::istringstream str(const_cast<char *>(value.c_str()));
      for(int j =0; j < 3; j++)
        {
        str >> info.LowerRight[j];
        }
      }
    else if(key == "UpperRight")
      {
      caveBounds++;
      vtksys_ios::istringstream str(const_cast<char *>(value.c_str()));
      for(int j =0; j < 3; j++)
        {
        str >> info.UpperRight[j];
        }
      }
    }
  if(caveBounds && caveBounds != 3)
    {
    vtkErrorMacro("LowerRight LowerLeft and UpperRight must all be present, if one is present");
    return 0;
    }
  if(caveBounds)
    {
    // if there are cave bounds then set the render module to CaveRenderModule
    this->SetRenderModuleName("CaveRenderModule");
    info.CaveBoundsSet = 1;
    }
  this->Internals->MachineInformationVector.push_back(info);
  return 1;
}

// ----------------------------------------------------------------------------
int vtkPVServerOptions::AddEyeSeparationInformation(const char** atts)
{
  std::string key = atts[0];
    std::string value = atts[1];
    if(key == "Value")
      {
      vtksys_ios::istringstream str(const_cast<char *>(value.c_str()));
      str >> this->Internals->EyeSeparation;
      }
    else
      {
        vtkErrorMacro("<EyeSeparation Value=\"...\"/> needs to be specified");
        return 0;
      }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVServerOptions::ParseExtraXMLTag(const char* name, const char** atts)
{
  // handle the Machine tag
  if(strcmp(name, "Machine") == 0)
    {
    return this->AddMachineInformation(atts);
    }
  // Hande the EyeSeparation tag
  if(strcmp(name, "EyeSeparation") == 0)
    {
    return this->AddEyeSeparationInformation(atts);
    }
  return 0;
}

//----------------------------------------------------------------------------
double vtkPVServerOptions::GetEyeSeparation()
{
  return static_cast<double> ( this->Internals->EyeSeparation );
}

//----------------------------------------------------------------------------
unsigned int vtkPVServerOptions::GetNumberOfMachines()
{
  return static_cast<unsigned int>(
    this->Internals->MachineInformationVector.size());
}

//----------------------------------------------------------------------------
const char* vtkPVServerOptions::GetMachineName(unsigned int idx)
{
  if (idx >= this->Internals->MachineInformationVector.size())
    {
    return 0;
    }

  return this->Internals->MachineInformationVector[idx].Name.c_str();
}

//----------------------------------------------------------------------------
const char* vtkPVServerOptions::GetDisplayName(unsigned int idx)
{
  if (idx >= this->Internals->MachineInformationVector.size())
    {
    return 0;
    }

  return this->Internals->MachineInformationVector[idx].Environment.c_str();
}

//----------------------------------------------------------------------------
double* vtkPVServerOptions::GetLowerLeft(unsigned int idx)
{
  if (idx >= this->Internals->MachineInformationVector.size())
    {
    return 0;
    }
  return this->Internals->MachineInformationVector[idx].LowerLeft;
}

//----------------------------------------------------------------------------
double* vtkPVServerOptions::GetLowerRight(unsigned int idx)
{
  if (idx >= this->Internals->MachineInformationVector.size())
    {
    return 0;
    }
  return this->Internals->MachineInformationVector[idx].LowerRight;
}

//----------------------------------------------------------------------------
double* vtkPVServerOptions::GetUpperRight(unsigned int idx)
{
  if (idx >= this->Internals->MachineInformationVector.size())
    {
    return 0;
    }
  return this->Internals->MachineInformationVector[idx].UpperRight;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Internals->PrintSelf(os, indent);
}

