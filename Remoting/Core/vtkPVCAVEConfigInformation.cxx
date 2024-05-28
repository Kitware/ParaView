// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVCAVEConfigInformation.h"

#include "vtkClientServerStream.h"
#include "vtkDisplayConfiguration.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"

#include <vtksys/SystemInformation.hxx>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN

class vtkPVCAVEConfigInformation::vtkInternals
{
public:
  bool IsInCAVE;
  int NumberOfDisplays;
  double EyeSeparation;
  bool ShowBorders;
  bool FullScreen;
  std::vector<int> Geometries;
  std::vector<int> HasCorners;
  std::vector<double> LowerLefts;
  std::vector<double> LowerRights;
  std::vector<double> UpperRights;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCAVEConfigInformation);

//----------------------------------------------------------------------------
vtkPVCAVEConfigInformation::vtkPVCAVEConfigInformation()
  : Internal(new vtkPVCAVEConfigInformation::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVCAVEConfigInformation::~vtkPVCAVEConfigInformation() = default;

//----------------------------------------------------------------------------
void vtkPVCAVEConfigInformation::CopyFromObject(vtkObject* vtkNotUsed(obj))
{
  auto config = vtkRemotingCoreConfiguration::GetInstance();
  auto caveConfig = config->GetDisplayConfiguration();

  config->GetIsInCave();

  int numberOfDisplays = caveConfig->GetNumberOfDisplays();
  this->Internal->IsInCAVE = config->GetIsInCave();
  this->Internal->NumberOfDisplays = numberOfDisplays;
  this->Internal->EyeSeparation = caveConfig->GetEyeSeparation();
  this->Internal->ShowBorders = caveConfig->GetShowBorders();
  this->Internal->FullScreen = caveConfig->GetFullScreen();

  this->Internal->Geometries.resize(0);
  this->Internal->HasCorners.resize(0);
  this->Internal->LowerLefts.resize(0);
  this->Internal->LowerRights.resize(0);
  this->Internal->UpperRights.resize(0);

  for (int i = 0; i < numberOfDisplays; ++i)
  {
    vtkTuple<int, 4> geom = caveConfig->GetGeometry(i);
    this->Internal->Geometries.push_back(geom[0]);
    this->Internal->Geometries.push_back(geom[1]);
    this->Internal->Geometries.push_back(geom[2]);
    this->Internal->Geometries.push_back(geom[3]);

    bool hasCorners = caveConfig->GetHasCorners(i);
    this->Internal->HasCorners.push_back(hasCorners ? 1 : 0);

    vtkTuple<double, 3> lowerLeft = caveConfig->GetLowerLeft(i);
    this->Internal->LowerLefts.push_back(lowerLeft[0]);
    this->Internal->LowerLefts.push_back(lowerLeft[1]);
    this->Internal->LowerLefts.push_back(lowerLeft[2]);

    vtkTuple<double, 3> lowerRight = caveConfig->GetLowerRight(i);
    this->Internal->LowerRights.push_back(lowerRight[0]);
    this->Internal->LowerRights.push_back(lowerRight[1]);
    this->Internal->LowerRights.push_back(lowerRight[2]);

    vtkTuple<double, 3> upperRight = caveConfig->GetUpperRight(i);
    this->Internal->UpperRights.push_back(upperRight[0]);
    this->Internal->UpperRights.push_back(upperRight[1]);
    this->Internal->UpperRights.push_back(upperRight[2]);
  }
}

//----------------------------------------------------------------------------
void vtkPVCAVEConfigInformation::AddInformation(vtkPVInformation* pvinfo)
{
  vtkPVCAVEConfigInformation* info = dynamic_cast<vtkPVCAVEConfigInformation*>(pvinfo);

  if (!info)
  {
    return;
  }

  int numberOfDisplays = info->GetNumberOfDisplays();

  this->Internal->IsInCAVE = info->GetIsInCAVE();
  this->Internal->NumberOfDisplays = numberOfDisplays;
  this->Internal->EyeSeparation = info->GetEyeSeparation();
  this->Internal->ShowBorders = info->GetShowBorders();
  this->Internal->FullScreen = info->GetFullScreen();

  this->Internal->Geometries.resize(0);
  this->Internal->HasCorners.resize(0);
  this->Internal->LowerLefts.resize(0);
  this->Internal->LowerRights.resize(0);
  this->Internal->UpperRights.resize(0);

  for (int i = 0; i < numberOfDisplays; ++i)
  {
    vtkTuple<int, 4> geom = info->GetGeometry(i);
    this->Internal->Geometries.push_back(geom[0]);
    this->Internal->Geometries.push_back(geom[1]);
    this->Internal->Geometries.push_back(geom[2]);
    this->Internal->Geometries.push_back(geom[3]);

    bool hasCorners = info->GetHasCorners(i);
    this->Internal->HasCorners.push_back(hasCorners ? 1 : 0);

    vtkTuple<double, 3> lowerLeft = info->GetLowerLeft(i);
    this->Internal->LowerLefts.push_back(lowerLeft[0]);
    this->Internal->LowerLefts.push_back(lowerLeft[1]);
    this->Internal->LowerLefts.push_back(lowerLeft[2]);

    vtkTuple<double, 3> lowerRight = info->GetLowerRight(i);
    this->Internal->LowerRights.push_back(lowerRight[0]);
    this->Internal->LowerRights.push_back(lowerRight[1]);
    this->Internal->LowerRights.push_back(lowerRight[2]);

    vtkTuple<double, 3> upperRight = info->GetUpperRight(i);
    this->Internal->UpperRights.push_back(upperRight[0]);
    this->Internal->UpperRights.push_back(upperRight[1]);
    this->Internal->UpperRights.push_back(upperRight[2]);
  }
}

//----------------------------------------------------------------------------
void vtkPVCAVEConfigInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();

  *css << vtkClientServerStream::Reply;
  *css << this->Internal->IsInCAVE << this->Internal->NumberOfDisplays
       << this->Internal->EyeSeparation << this->Internal->ShowBorders
       << this->Internal->FullScreen;

  for (int i = 0; i < this->Internal->Geometries.size(); ++i)
  {
    *css << this->Internal->Geometries[i];
  }

  for (int i = 0; i < this->Internal->HasCorners.size(); ++i)
  {
    *css << this->Internal->HasCorners[i];
  }

  for (int i = 0; i < this->Internal->LowerLefts.size(); ++i)
  {
    *css << this->Internal->LowerLefts[i];
  }

  for (int i = 0; i < this->Internal->LowerRights.size(); ++i)
  {
    *css << this->Internal->LowerRights[i];
  }

  for (int i = 0; i < this->Internal->UpperRights.size(); ++i)
  {
    *css << this->Internal->UpperRights[i];
  }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVCAVEConfigInformation::CopyFromStream(const vtkClientServerStream* css)
{
  int idx = 0;
  if (!css->GetArgument(0, idx++, &this->Internal->IsInCAVE))
  {
    vtkErrorMacro("Error parsing IsInCAVE from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->Internal->NumberOfDisplays))
  {
    vtkErrorMacro("Error parsing NumberOfDisplays from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->Internal->EyeSeparation))
  {
    vtkErrorMacro("Error parsing EyeSeparation from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->Internal->ShowBorders))
  {
    vtkErrorMacro("Error parsing ShowBorders from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->Internal->FullScreen))
  {
    vtkErrorMacro("Error parsing FullScreen from message.");
    return;
  }

  // Use the one we just read out of the stream, above
  int numberOfDisplays = this->Internal->NumberOfDisplays;

  // Copy the Geometries from the stream
  this->Internal->Geometries.resize(4 * numberOfDisplays);
  int eltIdx = 0;

  for (int i = 0; i < numberOfDisplays; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      if (!css->GetArgument(0, idx++, &(this->Internal->Geometries[eltIdx++])))
      {
        vtkErrorMacro("Error parsing Geometries from message.");
        return;
      }
    }
  }

  // Copy the HasCorners values from the stream
  this->Internal->HasCorners.resize(numberOfDisplays);

  for (int i = 0; i < numberOfDisplays; ++i)
  {
    if (!css->GetArgument(0, idx++, &(this->Internal->HasCorners[i])))
    {
      vtkErrorMacro("Error parsing HasCorners from message.");
      return;
    }
  }

  // Copy the LowerLefts (lower left corners) from the stream
  this->Internal->LowerLefts.resize(3 * numberOfDisplays);
  eltIdx = 0;

  for (int i = 0; i < numberOfDisplays; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (!css->GetArgument(0, idx++, &(this->Internal->LowerLefts[eltIdx++])))
      {
        vtkErrorMacro("Error parsing LowerLefts from message.");
        return;
      }
    }
  }

  // Copy the LowerRights (lower right corners) from the stream
  this->Internal->LowerRights.resize(3 * numberOfDisplays);
  eltIdx = 0;

  for (int i = 0; i < numberOfDisplays; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (!css->GetArgument(0, idx++, &(this->Internal->LowerRights[eltIdx++])))
      {
        vtkErrorMacro("Error parsing LowerRights from message.");
        return;
      }
    }
  }

  // Copy the UpperRights (lower right corners) from the stream
  this->Internal->UpperRights.resize(3 * numberOfDisplays);
  eltIdx = 0;

  for (int i = 0; i < numberOfDisplays; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (!css->GetArgument(0, idx++, &(this->Internal->UpperRights[eltIdx++])))
      {
        vtkErrorMacro("Error parsing UpperRights from message.");
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPVCAVEConfigInformation::GetIsInCAVE()
{
  return this->Internal->IsInCAVE;
}

//----------------------------------------------------------------------------
double vtkPVCAVEConfigInformation::GetEyeSeparation()
{
  return this->Internal->EyeSeparation;
}

//----------------------------------------------------------------------------
bool vtkPVCAVEConfigInformation::GetShowBorders()
{
  return this->Internal->ShowBorders;
}

//----------------------------------------------------------------------------
bool vtkPVCAVEConfigInformation::GetFullScreen()
{
  return this->Internal->FullScreen;
}

//----------------------------------------------------------------------------
int vtkPVCAVEConfigInformation::GetNumberOfDisplays()
{
  return this->Internal->NumberOfDisplays;
}

//----------------------------------------------------------------------------
vtkTuple<int, 4> vtkPVCAVEConfigInformation::GetGeometry(int index)
{
  vtkTuple<int, 4> ithTuple;
  int idx = index * 4;
  ithTuple[0] = this->Internal->Geometries.at(idx++);
  ithTuple[1] = this->Internal->Geometries.at(idx++);
  ithTuple[2] = this->Internal->Geometries.at(idx++);
  ithTuple[3] = this->Internal->Geometries.at(idx++);
  return ithTuple;
}

//----------------------------------------------------------------------------
bool vtkPVCAVEConfigInformation::GetHasCorners(int index)
{
  return this->Internal->HasCorners.at(index) == 1 ? true : false;
}

//----------------------------------------------------------------------------
vtkTuple<double, 3> vtkPVCAVEConfigInformation::GetLowerLeft(int index)
{
  vtkTuple<double, 3> ithTuple;
  int idx = index * 3;
  ithTuple[0] = this->Internal->LowerLefts.at(idx++);
  ithTuple[1] = this->Internal->LowerLefts.at(idx++);
  ithTuple[2] = this->Internal->LowerLefts.at(idx++);
  return ithTuple;
}

//----------------------------------------------------------------------------
vtkTuple<double, 3> vtkPVCAVEConfigInformation::GetLowerRight(int index)
{
  vtkTuple<double, 3> ithTuple;
  int idx = index * 3;
  ithTuple[0] = this->Internal->LowerRights.at(idx++);
  ithTuple[1] = this->Internal->LowerRights.at(idx++);
  ithTuple[2] = this->Internal->LowerRights.at(idx++);
  return ithTuple;
}

//----------------------------------------------------------------------------
vtkTuple<double, 3> vtkPVCAVEConfigInformation::GetUpperRight(int index)
{
  vtkTuple<double, 3> ithTuple;
  int idx = index * 3;
  ithTuple[0] = this->Internal->UpperRights.at(idx++);
  ithTuple[1] = this->Internal->UpperRights.at(idx++);
  ithTuple[2] = this->Internal->UpperRights.at(idx++);
  return ithTuple;
}

//----------------------------------------------------------------------------
void vtkPVCAVEConfigInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
