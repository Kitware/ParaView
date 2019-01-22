/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRPNCallBackHandlers.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

void VRPN_CALLBACK handleAnalogChange(void* userdata, const vrpn_ANALOGCB b);
void VRPN_CALLBACK handleButtonChange(void* userdata, vrpn_BUTTONCB b);
void VRPN_CALLBACK handleTrackerChange(void* userdata, const vrpn_TRACKERCB t);

#endif // vtkVRPNCallBackHandlers_h
