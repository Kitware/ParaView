// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRPNCallBackHandlers.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqVRPNConnection.h"
#include "pqView.h"
#include "vtkMath.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"

#include <iostream>
#include <vector>

#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include <vrpn_Tracker.h>

// ----------------------------------------------------------------------------
void VRPN_CALLBACK handleAnalogChange(void* userdata, const vrpn_ANALOGCB analog_value)
{
  pqVRPNConnection* self = static_cast<pqVRPNConnection*>(userdata);
  self->newAnalogValue(analog_value);
}

// ----------------------------------------------------------------------------
void VRPN_CALLBACK handleButtonChange(
  void* userdata, vrpn_BUTTONCB button_value) // WRS-TODO: why no "const" here?
{
  pqVRPNConnection* self = static_cast<pqVRPNConnection*>(userdata);
  self->newButtonValue(button_value);
}

// ----------------------------------------------------------------------------
void VRPN_CALLBACK handleTrackerChange(void* userdata, const vrpn_TRACKERCB tracker_value)
{
  pqVRPNConnection* self = static_cast<pqVRPNConnection*>(userdata);
  self->newTrackerValue(tracker_value);
}
