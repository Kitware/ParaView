// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/***********************************************************************/
/*                                                                     */
/* Style for the head tracking interface -- vtkSMVRTrackStyleProxy     */
/*                                                                     */
/* NOTES:                                                              */
/*    * The simplest of interface styles -- simply maps head tracking  */
/*        data to the eye location.                                    */
/*                                                                     */
/*    * It is expected that the RenderView EyeTransformMatrix is the   */
/*        property that will be connected to the head tracker.         */
/*                                                                     */
/***********************************************************************/
#include "vtkSMVRTrackStyleProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRTrackStyleProxy);

// ----------------------------------------------------------------------------
// Constructor method
vtkSMVRTrackStyleProxy::vtkSMVRTrackStyleProxy()
  : Superclass()
{
  this->AddTrackerRole("Tracker");
}

// ----------------------------------------------------------------------------
// Destructor method
vtkSMVRTrackStyleProxy::~vtkSMVRTrackStyleProxy() = default;

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkSMVRTrackStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkSMVRTrackStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role == "Tracker")
  {
    if (this->ControlledProxy && this->ControlledPropertyName != nullptr &&
      this->ControlledPropertyName[0] != '\0')
    {
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Set(event.data.tracker.matrix, 16);
      this->ControlledProxy->UpdateVTKObjects();
    }
  }
}
