/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqXRCustomViewpointsController.h"

#include "pqCustomViewpointsToolbar.h"
#include "vtkPVXRInterfaceHelper.h"

//----------------------------------------------------------------------------
pqXRCustomViewpointsController::pqXRCustomViewpointsController(
  vtkPVXRInterfaceHelper* helper, QObject* parent)
  : Superclass(parent)
  , Helper(helper)
{
}

//----------------------------------------------------------------------------
QStringList pqXRCustomViewpointsController::getCustomViewpointToolTips()
{
  return this->Helper->GetCustomViewpointToolTips();
}

//----------------------------------------------------------------------------
void pqXRCustomViewpointsController::configureCustomViewpoints()
{
  // Config button clear all poses in XR
  this->Helper->ClearCameraPoses();
}

//----------------------------------------------------------------------------
void pqXRCustomViewpointsController::setToCurrentViewpoint(int index)
{
  this->Helper->SaveCameraPose(index);
}

//----------------------------------------------------------------------------
void pqXRCustomViewpointsController::applyCustomViewpoint(int index)
{
  this->Helper->LoadCameraPose(index);
}

//----------------------------------------------------------------------------
void pqXRCustomViewpointsController::deleteCustomViewpoint(int index)
{
  // N/A
  Q_UNUSED(index);
}

//----------------------------------------------------------------------------
void pqXRCustomViewpointsController::addCurrentViewpointToCustomViewpoints()
{
  this->Helper->SaveCameraPose(this->Helper->GetNextPoseIndex());
}
