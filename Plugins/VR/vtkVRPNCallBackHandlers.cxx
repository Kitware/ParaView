/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRPNCallBackHandlers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRPNCallBackHandlers.h"

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include "vtkMath.h"
#include "pqActiveObjects.h"
#include "pqView.h"
#include <pqDataRepresentation.h>
#include "vtkSMRenderViewProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkVRPNConnection.h"
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vector>
#include <iostream>

// ----------------------------------------------------------------------------
void VRPN_CALLBACK handleAnalogChange(void* userdata, const vrpn_ANALOGCB b)
{
  vtkVRPNConnection *self = static_cast<vtkVRPNConnection*> ( userdata );
  self->NewAnalogValue( b );
}

// ----------------------------------------------------------------------------
void VRPN_CALLBACK handleButtonChange(void* userdata, vrpn_BUTTONCB b)
{
  vtkVRPNConnection *self = static_cast<vtkVRPNConnection*> ( userdata );
  self->NewButtonValue( b );
}

// ----------------------------------------------------------------------------
void VRPN_CALLBACK handleTrackerChange(void *userdata, const vrpn_TRACKERCB t)
{
  vtkVRPNConnection *self = static_cast<vtkVRPNConnection*> ( userdata );
  self->NewTrackerValue( t );
}
