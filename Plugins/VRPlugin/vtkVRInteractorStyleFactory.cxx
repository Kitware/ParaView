/*=========================================================================

   Program: ParaView
   Module:  vtkVRInteractorStyleFactory.cxx


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

// Includes all the interaction styles:
#include "vtkVRControlSliceOrientationStyle.h"
#include "vtkVRControlSlicePositionStyle.h"
#include "vtkVRGrabPointStyle.h"
#include "vtkVRGrabTransformStyle.h"
#include "vtkVRGrabWorldStyle.h"
#include "vtkVRSkeletonStyle.h"
#include "vtkVRSpaceNavigatorGrabWorldStyle.h"
#include "vtkVRStylusStyle.h"
#include "vtkVRStylusStyle.h"
#include "vtkVRTrackStyle.h"
#include "vtkVRVirtualHandStyle.h"
#include "vtkVRVirtualHandStyle.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRInteractorStyleFactory);
vtkVRInteractorStyleFactory* vtkVRInteractorStyleFactory::Instance = nullptr;

//-----------------------------------------------------------------------------
// Constructor() method
//   This is where all the InteractorStyle classes are connected to the VR Plugin.
//   This should be the ONLY place where interactor styles are mentioned by name.
//   TODO: figure out how to store an array of New() methods so the
//     NewInteractorStyleFromClassName() method doesn't have to explicitly
//     refer to named classes.
vtkVRInteractorStyleFactory::vtkVRInteractorStyleFactory()
{
#if 0 /* TODO: (WRS) We need to figure out how to store an array of New methods */
  this->InteractorStyleNewMethods.push_back((vtkVRInteractorStyle *)(vtkVRTrackStyle::New));
#endif
  // Add TrackStyle
  this->InteractorStyleClassNames.push_back("vtkVRTrackStyle");
  this->InteractorStyleDescriptions.push_back("Track");

  // Add GrabWorldStyle
  this->InteractorStyleClassNames.push_back("vtkVRGrabWorldStyle");
  this->InteractorStyleDescriptions.push_back("Grab");

  // Add GrabTransformStyle
  this->InteractorStyleClassNames.push_back("vtkVRGrabTransformStyle");
  this->InteractorStyleDescriptions.push_back("Grab Transform");

  // Add GrabPointStyle
  this->InteractorStyleClassNames.push_back("vtkVRGrabPointStyle");
  this->InteractorStyleDescriptions.push_back("Grab Point");

  // Add ControlSlicePositionStyle
  this->InteractorStyleClassNames.push_back("vtkVRControlSlicePositionStyle");
  this->InteractorStyleDescriptions.push_back("Slice Position");

  // Add ControlSliceOrientationStyle
  this->InteractorStyleClassNames.push_back("vtkVRControlSliceOrientationStyle");
  this->InteractorStyleDescriptions.push_back("Slice Orientation");

  // Add SpaceNavigatorGrabWorldStyle
  this->InteractorStyleClassNames.push_back("vtkVRSpaceNavigatorGrabWorldStyle");
  this->InteractorStyleDescriptions.push_back("Space Navigator Grab");

  // Add SpaceNavigatorGrabWorldStyle
  this->InteractorStyleClassNames.push_back("vtkVRStylusStyle");
  this->InteractorStyleDescriptions.push_back("zSpace Stylus");

  // Add SpaceNavigatorGrabWorldStyle
  this->InteractorStyleClassNames.push_back("vtkVRVirtualHandStyle");
  this->InteractorStyleDescriptions.push_back("Virtual Hand");

#if 0 /* WRS-TODO: wait until I implement these */
  // Add Virtual Hand - DJZ
  this->InteractorStyleClassNames.push_back("vtkVRVirtualHandStyle");
  this->InteractorStyleDescriptions.push_back("Virtual Hand");

  // Add Stylus style
  this->InteractorStyleClassNames.push_back("vtkVRStylusStyle");
  this->InteractorStyleDescriptions.push_back("Stylus");
#endif

#if 0 /* For the end-user, there is no need to see the Skeleton style code */
  // Add SkeletonStyle
  this->InteractorStyleClassNames.push_back("vtkVRSkeletonStyle");
  this->InteractorStyleDescriptions.push_back("Skeleton");
#endif
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
    vtkVRInteractorStyleFactory::Instance->UnRegister(nullptr);
  }

  if (ins)
  {
    ins->Register(nullptr);
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
  for (size_t count = 0; count < this->InteractorStyleClassNames.size(); ++count)
  {
    if (this->InteractorStyleClassNames[count] == className)
    {
      return this->InteractorStyleDescriptions[count];
    }
  }
  return std::string("Unknown");
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyle* vtkVRInteractorStyleFactory::NewInteractorStyleFromClassName(
  const std::string& name)
{
  // WRS-TODO: this is probably where we cause ParaView to crash if there are missing styles
  if (name == "vtkVRSkeletonStyle")
  {
    return vtkVRSkeletonStyle::New();
  }
  else if (name == "vtkVRTrackStyle")
  {
    return vtkVRTrackStyle::New();
  }
  else if (name == "vtkVRGrabWorldStyle")
  {
    return vtkVRGrabWorldStyle::New();
  }
  else if (name == "vtkVRGrabTransformStyle")
  {
    return vtkVRGrabTransformStyle::New();
  }
  else if (name == "vtkVRGrabPointStyle")
  {
    return vtkVRGrabPointStyle::New();
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
  else if (name == "vtkVRStylusStyle")
  {
    return vtkVRStylusStyle::New();
  }
  else if (name == "vtkVRVirtualHandStyle")
  {
    return vtkVRVirtualHandStyle::New();
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyle* vtkVRInteractorStyleFactory::NewInteractorStyleFromDescription(
  const std::string& desc)
{
  for (size_t count = 0; count < this->InteractorStyleDescriptions.size(); ++count)
  {
    if (this->InteractorStyleDescriptions[count] == desc)
    {
      return this->NewInteractorStyleFromClassName(this->InteractorStyleClassNames[count]);
    }
  }
  return nullptr;
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
  for (size_t count = 0; count < this->InteractorStyleClassNames.size(); ++count)
  {
    os << iindent << "\"" << this->InteractorStyleDescriptions[count] << "\" ("
       << this->InteractorStyleClassNames[count] << ")\n";
  }
}
