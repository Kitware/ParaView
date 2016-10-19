/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#include "vtkVRInteractorStyleFactory.h"

#include "vtkObjectFactory.h"
#include "vtkVRControlSliceOrientationStyle.h"
#include "vtkVRControlSlicePositionStyle.h"
#include "vtkVRGrabWorldStyle.h"
#include "vtkVRSpaceNavigatorGrabWorldStyle.h"
#include "vtkVRTrackStyle.h"
#include "vtkVRVirtualHandStyle.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRInteractorStyleFactory)
  vtkVRInteractorStyleFactory* vtkVRInteractorStyleFactory::Instance = NULL;

//-----------------------------------------------------------------------------
vtkVRInteractorStyleFactory::vtkVRInteractorStyleFactory()
{
  // Add TrackStyle
  this->InteractorStyleClassNames.push_back("vtkVRTrackStyle");
  this->InteractorStyleDescriptions.push_back("Track");

  // Add GrabWorldStyle
  this->InteractorStyleClassNames.push_back("vtkVRGrabWorldStyle");
  this->InteractorStyleDescriptions.push_back("Grab");

  // Add ControlSlicePositionStyle
  this->InteractorStyleClassNames.push_back("vtkVRControlSlicePositionStyle");
  this->InteractorStyleDescriptions.push_back("Slice Position");

  // Add ControlSliceOrientationStyle
  this->InteractorStyleClassNames.push_back("vtkVRControlSliceOrientationStyle");
  this->InteractorStyleDescriptions.push_back("Slice Orientation");

  // Add SpaceNavigatorGrabWorldStye
  this->InteractorStyleClassNames.push_back("vtkVRSpaceNavigatorGrabWorldStyle");
  this->InteractorStyleDescriptions.push_back("Space Navigator Grab");

  // Add Virtual Hand - DJZ
  this->InteractorStyleClassNames.push_back("vtkVRVirtualHandStyle");
  this->InteractorStyleDescriptions.push_back("Virtual Hand");
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyleFactory::~vtkVRInteractorStyleFactory()
{
}

//-----------------------------------------------------------------------------
void vtkVRInteractorStyleFactory::SetInstance(vtkVRInteractorStyleFactory* ins)
{
  if (vtkVRInteractorStyleFactory::Instance)
  {
    vtkVRInteractorStyleFactory::Instance->UnRegister(NULL);
  }

  if (ins)
  {
    ins->Register(NULL);
  }

  vtkVRInteractorStyleFactory::Instance = ins;
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyleFactory* vtkVRInteractorStyleFactory::GetInstance()
{
  return vtkVRInteractorStyleFactory::Instance;
}

//-----------------------------------------------------------------------------
std::vector<std::string> vtkVRInteractorStyleFactory::GetInteractorStyleClassNames()
{
  return this->InteractorStyleClassNames;
}

//-----------------------------------------------------------------------------
std::vector<std::string> vtkVRInteractorStyleFactory::GetInteractorStyleDescriptions()
{
  return this->InteractorStyleDescriptions;
}

//-----------------------------------------------------------------------------
std::string vtkVRInteractorStyleFactory::GetDescriptionFromClassName(const std::string& className)
{
  for (size_t i = 0; i < this->InteractorStyleClassNames.size(); ++i)
  {
    if (this->InteractorStyleClassNames[i] == className)
    {
      return this->InteractorStyleDescriptions[i];
    }
  }
  return std::string("Unknown");
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyle* vtkVRInteractorStyleFactory::NewInteractorStyleFromClassName(
  const std::string& name)
{
  if (name == "vtkVRTrackStyle")
  {
    return vtkVRTrackStyle::New();
  }
  else if (name == "vtkVRGrabWorldStyle")
  {
    return vtkVRGrabWorldStyle::New();
  }
  else if (name == "vtkVRControlSlicePositionStyle")
  {
    return vtkVRControlSlicePositionStyle::New();
  }
  else if (name == "vtkVRControlSliceOrientationStyle")
  {
    return vtkVRControlSliceOrientationStyle::New();
  }
  else if (name == "vtkVRSpaceNavigatorGrabWorldStyle")
  {
    return vtkVRSpaceNavigatorGrabWorldStyle::New();
  }
  else if (name == "vtkVRVirtualHandStyle")
  {
    return vtkVRVirtualHandStyle::New();
  }

  return NULL;
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyle* vtkVRInteractorStyleFactory::NewInteractorStyleFromDescription(
  const std::string& desc)
{
  for (size_t i = 0; i < this->InteractorStyleDescriptions.size(); ++i)
  {
    if (this->InteractorStyleDescriptions[i] == desc)
    {
      return this->NewInteractorStyleFromClassName(this->InteractorStyleClassNames[i]);
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkVRInteractorStyleFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->InteractorStyleClassNames.size() != this->InteractorStyleDescriptions.size())
  {
    os << indent << "Internal state invalid!\n";
    return;
  }
  os << indent << "Known interactor styles:" << endl;
  vtkIndent iindent = indent.GetNextIndent();
  for (size_t i = 0; i < this->InteractorStyleClassNames.size(); ++i)
  {
    os << iindent << "\"" << this->InteractorStyleDescriptions[i] << "\" ("
       << this->InteractorStyleClassNames[i] << ")\n";
  }
}
