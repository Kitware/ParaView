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
#include "vtkProcessModule.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

#include <cstdio>

namespace
{
// redefining from vtkRenderWindow.h to avoid a dependency on rendering.
// we need to fix this at some point.
static int VTK_STEREO_CRYSTAL_EYES = 1;
static int VTK_STEREO_RED_BLUE = 2;
static int VTK_STEREO_INTERLACED = 3;
static int VTK_STEREO_LEFT = 4;
static int VTK_STEREO_RIGHT = 5;
static int VTK_STEREO_DRESDEN = 6;
static int VTK_STEREO_ANAGLYPH = 7;
static int VTK_STEREO_CHECKERBOARD = 8;
static int VTK_STEREO_SPLITVIEWPORT_HORIZONTAL = 9;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerOptions);

//----------------------------------------------------------------------------
vtkPVServerOptions::vtkPVServerOptions()
{
  this->Internals = new vtkPVServerOptionsInternals;

  // initialize host names
  this->ClientHostName = 0;
  this->SetClientHostName(this->GetHostName());

  // This default value for ServerPort is setup in Initialize().
  this->ServerPort = 0;
}

//----------------------------------------------------------------------------
vtkPVServerOptions::~vtkPVServerOptions()
{
  this->SetClientHostName(0);

  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::Initialize()
{
  this->Superclass::Initialize();

  this->AddArgument("--client-host", "-ch", &this->ClientHostName,
    "Tell the data|render server the host name of the client, use with -rc.",
    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVDATA_SERVER | vtkPVOptions::PVSERVER);

  switch (vtkProcessModule::GetProcessType())
  {
    case vtkProcessModule::PROCESS_SERVER:
      this->ServerPort = 11111;
      this->AddArgument("--server-port", "-sp", &this->ServerPort,
        "What port should the combined server use to connect to the client. (default 11111).",
        vtkPVOptions::PVSERVER);
      break;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      this->ServerPort = 11111;
      this->AddArgument("--data-server-port", "-dsp", &this->ServerPort,
        "What port data server use to connect to the client. (default 11111).",
        vtkPVOptions::PVDATA_SERVER);
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      this->ServerPort = 22221;
      this->AddArgument("--render-server-port", "-rsp", &this->ServerPort,
        "What port should the render server use to connect to the client. (default 22221).",
        vtkPVOptions::PVRENDER_SERVER);
      break;

    default:
      vtkErrorMacro("vtkPVServerOptions is only meant for server-processes.");
  }
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
    if (key == "Name")
    {
      info.Name = value;
    }
    else if (key == "Environment")
    {
      info.Environment = value;
    }
    else if (key == "Geometry")
    {
      for (int j = 0; j < 4; j++)
      {
        int matches = sscanf(value.c_str(), "%dx%d+%d+%d", &info.Geometry[2], &info.Geometry[3],
          &info.Geometry[0], &info.Geometry[1]);
        if (matches != 4)
        {
          vtkErrorMacro("Malformed geometry specification: "
            << value.c_str() << " (expected <X>x<Y>+<width>+<height>).");
          info.Geometry[0] = 0;
          info.Geometry[1] = 0;
          info.Geometry[2] = 0;
          info.Geometry[3] = 0;
        }
      }
    }
    else if (key == "FullScreen")
    {
      std::istringstream str(const_cast<char*>(value.c_str()));
      str >> info.FullScreen;
    }
    else if (key == "ShowBorders")
    {
      std::istringstream str(const_cast<char*>(value.c_str()));
      str >> info.ShowBorders;
    }
    else if (key == "StereoType")
    {
      std::map<std::string, int> map;
      map["crystal eyes"] = VTK_STEREO_CRYSTAL_EYES;
      map["red-blue"] = VTK_STEREO_RED_BLUE;
      map["interlaced"] = VTK_STEREO_INTERLACED;
      map["left"] = VTK_STEREO_LEFT;
      map["right"] = VTK_STEREO_RIGHT;
      map["dresden"] = VTK_STEREO_DRESDEN;
      map["anaglyph"] = VTK_STEREO_ANAGLYPH;
      map["checkerboard"] = VTK_STEREO_CHECKERBOARD;
      map["splitviewporthorizontal"] = VTK_STEREO_SPLITVIEWPORT_HORIZONTAL;

      value = vtksys::SystemTools::LowerCase(value);
      if (map.find(value) != map.end())
      {
        info.StereoType = map[value];
      }
      else
      {
        info.StereoType = 0; // no stereo (or invalid stereo mode).
      }
      // Currently, we only support left or right stereo. We cannot simply support
      // other types since that causes multiple renders and those need to match
      // up across all processes, including the client.
      if (info.StereoType != VTK_STEREO_LEFT && info.StereoType != VTK_STEREO_RIGHT &&
        info.StereoType != 0)
      {
        vtkErrorMacro("Only 'Left' or 'Right' can be used as the StereoType. "
                      "For all other modes, please use the command line arguments for the "
                      "ParaView client.");
        info.StereoType = -1;
      }
    }
    else if (key == "LowerLeft")
    {
      caveBounds++;
      std::istringstream str(const_cast<char*>(value.c_str()));
      for (int j = 0; j < 3; j++)
      {
        str >> info.LowerLeft[j];
      }
    }
    else if (key == "LowerRight")
    {
      caveBounds++;
      std::istringstream str(const_cast<char*>(value.c_str()));
      for (int j = 0; j < 3; j++)
      {
        str >> info.LowerRight[j];
      }
    }
    else if (key == "UpperRight")
    {
      caveBounds++;
      std::istringstream str(const_cast<char*>(value.c_str()));
      for (int j = 0; j < 3; j++)
      {
        str >> info.UpperRight[j];
      }
    }
  }
  if (caveBounds && caveBounds != 3)
  {
    vtkErrorMacro("LowerRight LowerLeft and UpperRight must all be present, if one is present");
    return 0;
  }
  if (caveBounds)
  {
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
  if (key == "Value")
  {
    std::istringstream str(const_cast<char*>(value.c_str()));
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
  if (strcmp(name, "Machine") == 0)
  {
    return this->AddMachineInformation(atts);
  }
  // Hande the EyeSeparation tag
  if (strcmp(name, "EyeSeparation") == 0)
  {
    return this->AddEyeSeparationInformation(atts);
  }
  return 0;
}

//----------------------------------------------------------------------------
double vtkPVServerOptions::GetEyeSeparation() const
{
  return static_cast<double>(this->Internals->EyeSeparation);
}

//----------------------------------------------------------------------------
unsigned int vtkPVServerOptions::GetNumberOfMachines() const
{
  return static_cast<unsigned int>(this->Internals->MachineInformationVector.size());
}

//----------------------------------------------------------------------------
const char* vtkPVServerOptions::GetMachineName(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return 0;
  }

  return this->Internals->MachineInformationVector[idx].Name.c_str();
}

//----------------------------------------------------------------------------
const char* vtkPVServerOptions::GetDisplayName(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return 0;
  }

  return this->Internals->MachineInformationVector[idx].Environment.c_str();
}

//----------------------------------------------------------------------------
int* vtkPVServerOptions::GetGeometry(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return 0;
  }

  return this->Internals->MachineInformationVector[idx].Geometry;
}

//----------------------------------------------------------------------------
bool vtkPVServerOptions::GetFullScreen(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return false;
  }

  return this->Internals->MachineInformationVector[idx].FullScreen != 0;
}

//----------------------------------------------------------------------------
int vtkPVServerOptions::GetStereoType(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return -1;
  }

  return this->Internals->MachineInformationVector[idx].StereoType;
}

//----------------------------------------------------------------------------
bool vtkPVServerOptions::GetShowBorders(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return false;
  }

  return this->Internals->MachineInformationVector[idx].ShowBorders != 0;
}

//----------------------------------------------------------------------------
double* vtkPVServerOptions::GetLowerLeft(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return 0;
  }
  return this->Internals->MachineInformationVector[idx].LowerLeft;
}

//----------------------------------------------------------------------------
double* vtkPVServerOptions::GetLowerRight(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return 0;
  }
  return this->Internals->MachineInformationVector[idx].LowerRight;
}

//----------------------------------------------------------------------------
double* vtkPVServerOptions::GetUpperRight(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return 0;
  }
  return this->Internals->MachineInformationVector[idx].UpperRight;
}

//----------------------------------------------------------------------------
bool vtkPVServerOptions::GetCaveBoundsSet(unsigned int idx) const
{
  if (idx >= this->Internals->MachineInformationVector.size())
  {
    return 0;
  }
  return this->Internals->MachineInformationVector[idx].CaveBoundsSet == 1;
}

//----------------------------------------------------------------------------
bool vtkPVServerOptions::GetIsInCave() const
{
  if (this->Superclass::GetIsInCave())
  {
    return true;
  }

  return (this->GetIsInTileDisplay() == false && this->GetNumberOfMachines() > 0);
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Internals->PrintSelf(os, indent);

  os << indent << "ClientHostName: " << (this->ClientHostName ? this->ClientHostName : "(none)")
     << endl;
  os << indent << "ServerPort: " << this->ServerPort << endl;
}
