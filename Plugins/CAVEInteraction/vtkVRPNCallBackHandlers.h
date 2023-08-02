// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkVRPNCallBackHandlers -
// .SECTION Description
// vtkVRPNCallBackHandlers

#ifndef vtkVRPNCallBackHandlers_h
#define vtkVRPNCallBackHandlers_h

#include <iostream>
#include <vector>
#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include <vrpn_Tracker.h>

void VRPN_CALLBACK handleAnalogChange(void* userdata, const vrpn_ANALOGCB analog_value);
void VRPN_CALLBACK handleButtonChange(
  void* userdata, vrpn_BUTTONCB button_value); // WRS-TODO: why no "const" here?
void VRPN_CALLBACK handleTrackerChange(void* userdata, const vrpn_TRACKERCB tracker_value);

#endif // vtkVRPNCallBackHandlers_h
