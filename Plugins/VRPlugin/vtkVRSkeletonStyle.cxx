/*=========================================================================

   Program: ParaView
   Module:  vtkVRSkeletonStyle.cxx

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
#include "vtkVRSkeletonStyle.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "pqView.h"

#include <algorithm>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRSkeletonStyle);

// ----------------------------------------------------------------------------
// Constructor method
vtkVRSkeletonStyle::vtkVRSkeletonStyle()
  : Superclass()
{
  this->AddButtonRole("Rotate Tracker");
  this->AddButtonRole("Report Self");
  this->AddAnalogRole("X");
  this->AddTrackerRole("Tracker");
  this->EnableReport = false;

  // leftover stuff:
  this->IsInitialTransRecorded = false;
  this->IsInitialRotRecorded = false;
  this->CachedTransMatrix->Identity();
  this->CachedRotMatrix->Identity();
}

// ----------------------------------------------------------------------------
// Destructor method
vtkVRSkeletonStyle::~vtkVRSkeletonStyle()
{
}

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkVRSkeletonStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableReport: " << this->EnableReport << endl;
  os << indent << "IsInitialTransRecorded: " << this->IsInitialTransRecorded << endl;
  os << indent << "IsInitialRotRecorded: " << this->IsInitialRotRecorded << endl;
  os << indent << "InverseInitialTransMatrix:" << endl;
  this->InverseInitialTransMatrix->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkVRSkeletonStyle::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);

  if (event.data.button.state == 1)
  {
    cout << "Button " << event.data.button.button << "is pressed\n";
  }
  if (role == "Report Self")
  {
    if (event.data.button.state == 1)
    {
      cout << "Reporting on myself\n";
      this->PrintSelf(cout, vtkIndent(0));
    }
  }
  if (role == "Report Tracker")
  {
    this->EnableReport = event.data.button.state;
  }
}

// ----------------------------------------------------------------------------
// HandleAnalog() method
void vtkVRSkeletonStyle::HandleAnalog(const vtkVREvent& event)
{
  std::string role = this->GetAnalogRole(event.name);

  if (role == "X")
  {
    cout << "Got a value for 'X' of " << event.data.analog.num_channels << " : "
         << event.data.analog.channel[0] << " " << event.data.analog.channel[1] << "\n";
  }
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkVRSkeletonStyle::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role == "Tracker")
  {
    if (this->EnableReport)
    {
      // do something interesting here
      cout << "Do a tracker report\n";
    }
  }
}
