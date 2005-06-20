/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraAnimationCue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraAnimationCue.h"

#include "vtkObjectFactory.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVCameraKeyFrame.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyStatusManager.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkPVCameraAnimationCue);
vtkCxxRevisionMacro(vtkPVCameraAnimationCue, "1.3");
//------------------------------------------------------------------------------
vtkPVCameraAnimationCue::vtkPVCameraAnimationCue()
{
  this->SetKeyFrameManipulatorProxyXMLName("CameraManipulator");
}

//------------------------------------------------------------------------------
vtkPVCameraAnimationCue::~vtkPVCameraAnimationCue()
{
  
}

//------------------------------------------------------------------------------
int vtkPVCameraAnimationCue::IsKeyFrameTypeSupported(int type)
{
  return (type == vtkPVSimpleAnimationCue::CAMERA)? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkPVCameraAnimationCue::ReplaceKeyFrame(vtkPVKeyFrame*, vtkPVKeyFrame*)
{
  vtkErrorMacro("Camera does not support different types of key frames!");
}

//------------------------------------------------------------------------------
void vtkPVCameraAnimationCue::SetAnimatedProxy(vtkSMProxy* proxy)
{
  this->Superclass::SetAnimatedProxy(proxy);
  if (!this->PropertyStatusManager)
    {
    this->PropertyStatusManager = vtkSMPropertyStatusManager::New();
    }
  this->PropertyStatusManager->UnregisterAllProperties();
  proxy->UpdateInformation();
  const char* names[] = {"CameraPositionInfo", "CameraFocalPointInfo",
        "CameraViewUpInfo", "CameraViewAngleInfo",  0 };
  for (int i=0; names[i]; i++)
    {
    this->PropertyStatusManager->RegisterProperty(
      vtkSMVectorProperty::SafeDownCast(proxy->GetProperty(names[i])));
    }
  this->PropertyStatusManager->InitializeStatus();
        
}

//------------------------------------------------------------------------------
void vtkPVCameraAnimationCue::StartRecording()
{
  if (!this->PropertyStatusManager)
    {
    return;
    }
  vtkSMProxy* proxy = this->CueProxy->GetAnimatedProxy();
  proxy->UpdateInformation();
  this->Superclass::StartRecording();
}

//------------------------------------------------------------------------------
void vtkPVCameraAnimationCue::RecordState(double ntime, double offset, int of)
{
  this->Superclass::RecordState(ntime, offset, of);
}

//------------------------------------------------------------------------------
void vtkPVCameraAnimationCue::RecordState(double ntime, double offset)
{
  if (!this->InRecording)
    {
    vtkErrorMacro("Not in recording mode.");
    return;
    }

  if (this->Virtual || !this->PropertyStatusManager)
    {
    return;
    }
  vtkSMProxy* cameraProxy = this->CueProxy->GetAnimatedProxy();
  cameraProxy->UpdateInformation();

  const char* names[] = {"CameraPositionInfo", "CameraFocalPointInfo",
        "CameraViewUpInfo", "CameraViewAngleInfo",  0};

  const char* kfnames[] = {"Position", "FocalPoint",
        "ViewUp", "ViewAngle",  0}; 
  int modified = 0;
  for (int i=0; names[i]; i++)
    {
    if (this->PropertyStatusManager->HasPropertyChanged(
        vtkSMVectorProperty::SafeDownCast(
          cameraProxy->GetProperty(names[i]))))
      {
      modified = 1;
      break;
      }
    }
  // animated property has changed.
  // add a keyframe at ntime.
  int old_numOfKeyFrames = this->GetNumberOfKeyFrames();
  if (!this->PreviousStepKeyFrameAdded)
    {
    int id = this->AddNewKeyFrame(ntime);
    if (id == -1)
      {
      vtkErrorMacro("Failed to add new key frame");
      return;
      }
    vtkPVCameraKeyFrame* kf = vtkPVCameraKeyFrame::SafeDownCast(
      this->GetKeyFrame(id));
    
    for (int i=0; names[i]; i++)
      {
      // loop over all properties and init the keyframe accordingly.
      vtkSMVectorProperty* cameraVp =
        vtkSMVectorProperty::SafeDownCast(
          cameraProxy->GetProperty(names[i]));
      
      vtkSMDoubleVectorProperty* internalDvp =
        vtkSMDoubleVectorProperty::SafeDownCast(
          this->PropertyStatusManager->GetInternalProperty(cameraVp));
      kf->SetProperty(kfnames[i], internalDvp);
      }
    if (old_numOfKeyFrames == 0)
      {
      //Pilot keyframe also needs to be initilaized.
      vtkPVCameraKeyFrame* kf_pilot = vtkPVCameraKeyFrame::SafeDownCast(
        this->GetKeyFrame(0));
      for (int i=0; names[i]; i++)
        {
        // loop over all properties and init the keyframe accordingly.
        vtkSMVectorProperty* cameraVp =
          vtkSMVectorProperty::SafeDownCast(
            cameraProxy->GetProperty(names[i]));

        vtkSMDoubleVectorProperty* internalDvp =
          vtkSMDoubleVectorProperty::SafeDownCast(
            this->PropertyStatusManager->GetInternalProperty(cameraVp));
        kf_pilot->SetProperty(kfnames[i], internalDvp);
        }
      }
    }

  int id2 = this->AddNewKeyFrame(ntime + offset);
  if (id2 == -1)
    {
    vtkErrorMacro("Failed to add new key frame");
    return;
    }
  this->PreviousStepKeyFrameAdded = 1;
  if (this->PropertyStatusManager)
    {
    this->PropertyStatusManager->InitializeStatus();
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void vtkPVCameraAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
