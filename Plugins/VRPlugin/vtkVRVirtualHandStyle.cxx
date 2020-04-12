/*=========================================================================

   Program: ParaView
   Module:    vtkVRVirtualHandStyle.cxx

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

#include "vtkVRVirtualHandStyle.h"

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

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRVirtualHandStyle)

  // -----------------------------------------------------------------------------
  vtkVRVirtualHandStyle::vtkVRVirtualHandStyle()
  : Superclass()
{
  this->AddButtonRole("Grab world");

  this->CurrentButton = false;
  this->PrevButton = false;

  this->EventPress = false;
  this->EventRelease = false;

  this->InverseTrackerMatrix->Identity();
  this->CachedModelMatrix->Identity();
  this->CurrentTrackerMatrix->Identity();
}

// -----------------------------------------------------------------------------
vtkVRVirtualHandStyle::~vtkVRVirtualHandStyle()
{
}

// ----------------------------------------------------------------------------
void vtkVRVirtualHandStyle::HandleButton(const vtkVREventData& data)
{
  std::string role = this->GetButtonRole(data.name);
  if (role == "Grab world")
  {

    this->CurrentButton = data.data.button.state;

    if (this->CurrentButton == true && this->PrevButton == false)
    {
      this->EventPress = true;
    }
    if (this->CurrentButton == false && this->PrevButton == true)
    {
      this->EventRelease = true;
    }

    this->PrevButton = this->CurrentButton;
  }
}

// ----------------------------------------------------------------------------
void vtkVRVirtualHandStyle::HandleTracker(const vtkVREventData& data)
{
  std::string role = this->GetTrackerRole(data.name);
  if (role == "Tracker")
  {
    this->CurrentTrackerMatrix->DeepCopy(data.data.tracker.matrix);

    if (this->EventPress)
    {
      double matrix_vals[16];
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Get(matrix_vals, 16);
      this->CachedModelMatrix->DeepCopy(matrix_vals);

      this->InverseTrackerMatrix->DeepCopy(this->CurrentTrackerMatrix.GetPointer());
      this->InverseTrackerMatrix->Invert();

      this->EventPress = false;
    }

    if (CurrentButton || EventRelease) // if grabbing or if we just let go
    {
      // Goal is:
      // result = tracker * tracker_inverse_at_start * original_model

      vtkNew<vtkMatrix4x4> tempMatrix;

      // TODO: could reduce to 1 matrix multiplication (pre-calc inv*model)
      vtkMatrix4x4::Multiply4x4(this->CurrentTrackerMatrix.GetPointer(),
        this->InverseTrackerMatrix.GetPointer(), tempMatrix.GetPointer());
      vtkMatrix4x4::Multiply4x4(tempMatrix.GetPointer(), this->CachedModelMatrix.GetPointer(),
        this->NewModelMatrix.GetPointer());

      // Set the new matrix for the proxy.
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Set(&this->NewModelMatrix->Element[0][0], 16);
      this->ControlledProxy->UpdateVTKObjects();

      this->EventRelease = false;
    }
  }
}

// ----------------------------------------------------------------------------
void vtkVRVirtualHandStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CurrentButton: " << this->CurrentButton << endl;
  os << indent << "PrevButton: " << this->PrevButton << endl;
  os << indent << "EventPress: " << this->EventPress << endl;
  os << indent << "EventRelease: " << this->EventPress << endl;

  os << indent << "CurrentTrackerMatrix:" << endl;
  this->CurrentTrackerMatrix->PrintSelf(os, indent.GetNextIndent());

  os << indent << "InverseTrackerMatrix:" << endl;
  this->InverseTrackerMatrix->PrintSelf(os, indent.GetNextIndent());

  os << indent << "CachedModelMatrix:" << endl;
  this->CachedModelMatrix->PrintSelf(os, indent.GetNextIndent());

  os << indent << "NewModelMatrix:" << endl;
  this->NewModelMatrix->PrintSelf(os, indent.GetNextIndent());
}
