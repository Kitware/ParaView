/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimateWarp.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimateWarp.h"

#include "vtkKWFrame.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVActiveTrackSelector.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVTrackEditor.h"
#include "vtkPVVerticalAnimationInterface.h"
#include "vtkPVWindow.h"

#include "Resources/vtkPVRamp.h"
#include "Resources/vtkPVStep.h"
#include "Resources/vtkPVExponential.h"
#include "Resources/vtkPVSinusoid.h"

vtkStandardNewMacro(vtkPVAnimateWarp);
vtkCxxRevisionMacro(vtkPVAnimateWarp, "1.1");

//----------------------------------------------------------------------------
vtkPVAnimateWarp::vtkPVAnimateWarp()
{
  this->AnimateFrame = vtkKWFrame::New();
  this->AnimateLabel = vtkKWLabel::New();
  this->RampButton = vtkKWPushButton::New();
  this->StepButton = vtkKWPushButton::New();
  this->ExponentialButton = vtkKWPushButton::New();
  this->SinusoidalButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkPVAnimateWarp::~vtkPVAnimateWarp()
{
  this->AnimateFrame->Delete();
  this->AnimateLabel->Delete();
  this->RampButton->Delete();
  this->StepButton->Delete();
  this->ExponentialButton->Delete();
  this->SinusoidalButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVAnimateWarp::CreateProperties()
{
  this->Superclass::CreateProperties();

  this->AnimateFrame->SetParent(this->ParameterFrame->GetFrame());
  this->AnimateFrame->Create();

  this->AnimateLabel->SetParent(this->AnimateFrame);
  this->AnimateLabel->Create();
  this->AnimateLabel->SetWidth(18);
  this->AnimateLabel->SetJustificationToRight();
  this->AnimateLabel->SetText("Animate");

  this->RampButton->SetParent(this->AnimateFrame);
  this->RampButton->Create();
  this->RampButton->SetImageToPixels(image_PVRamp,
                                     image_PVRamp_width, image_PVRamp_height,
                                     image_PVRamp_pixel_size,
                                     image_PVRamp_length);
  ostrstream rampStr;
  rampStr << "AnimateScaleFactor " << vtkPVSimpleAnimationCue::RAMP << ends;
  this->RampButton->SetCommand(this, rampStr.str());
  rampStr.rdbuf()->freeze(0);
  this->RampButton->SetBalloonHelpString(
    "Accept the current parameters, and go to the animation page. Make "
    "ScaleFactor the selected Parameter. If it previously had no keyframes, "
    "add one that uses ramp interpolation.");

  this->StepButton->SetParent(this->AnimateFrame);
  this->StepButton->Create();
  this->StepButton->SetImageToPixels(image_PVStep,
                                     image_PVStep_width, image_PVStep_height,
                                     image_PVStep_pixel_size,
                                     image_PVStep_length);
  ostrstream stepStr;
  stepStr << "AnimateScaleFactor " << vtkPVSimpleAnimationCue::STEP << ends;
  this->StepButton->SetCommand(this, stepStr.str());
  stepStr.rdbuf()->freeze(0);
  this->StepButton->SetBalloonHelpString(
    "Accept the current parameters, and go to the animation page. Make "
    "ScaleFactor the selected Parameter. If it previously had no keyframes, "
    "add one that uses step interpolation.");

  this->ExponentialButton->SetParent(this->AnimateFrame);
  this->ExponentialButton->Create();
  this->ExponentialButton->SetImageToPixels(image_PVExponential,
                                            image_PVExponential_width,
                                            image_PVExponential_height,
                                            image_PVExponential_pixel_size,
                                            image_PVExponential_length);
  ostrstream expStr;
  expStr << "AnimateScaleFactor " << vtkPVSimpleAnimationCue::EXPONENTIAL << ends;
  this->ExponentialButton->SetCommand(this, expStr.str());
  expStr.rdbuf()->freeze(0);
  this->ExponentialButton->SetBalloonHelpString(
    "Accept the current parameters, and go to the animation page. Make "
    "ScaleFactor the selected Parameter. If it previously had no keyframes, "
    "add one that uses exponential interpolation.");

  this->SinusoidalButton->SetParent(this->AnimateFrame);
  this->SinusoidalButton->Create();
  this->SinusoidalButton->SetImageToPixels(image_PVSinusoid,
                                           image_PVSinusoid_width,
                                           image_PVSinusoid_height,
                                           image_PVSinusoid_pixel_size,
                                           image_PVSinusoid_length);
  ostrstream sinStr;
  sinStr << "AnimateScaleFactor " << vtkPVSimpleAnimationCue::SINUSOID << ends;
  this->SinusoidalButton->SetCommand(this, sinStr.str());
  sinStr.rdbuf()->freeze(0);
  this->SinusoidalButton->SetBalloonHelpString(
    "Accept the current parameters, and go to the animation page. Make "
    "ScaleFactor the selected Parameter. If it previously had no keyframes, "
    "add one that uses sinusoidal interpolation.");

  this->Script("pack %s -side left",
               this->AnimateLabel->GetWidgetName());
  this->Script("pack %s %s %s %s -side left -padx 2",
               this->RampButton->GetWidgetName(),
               this->StepButton->GetWidgetName(),
               this->ExponentialButton->GetWidgetName(),
               this->SinusoidalButton->GetWidgetName());
  this->Script("pack %s -side top -fill x -expand t",
               this->AnimateFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVAnimateWarp::AnimateScaleFactor(int modeShape)
{
  this->Accept();

  vtkPVWindow *window = this->GetPVWindow();
  if (!window)
    {
    return;
    }

  window->ShowAnimationPanes();
  vtkPVAnimationManager *animManager = window->GetAnimationManager();
  if (!animManager)
    {
    return;
    }

  vtkPVActiveTrackSelector *trackSel = animManager->GetActiveTrackSelector();
  if (!trackSel)
    {
    return;
    }

  trackSel->SelectSourceCallback(this->GetName());
  trackSel->SelectPropertyCallback(
    trackSel->GetPropertyMenuButton()->GetMenu()->GetIndexOfItem("ScaleFactor"));

  vtkPVAnimationCue *animCue = trackSel->GetCurrentCue();
  if (!animCue || animCue->GetNumberOfKeyFrames() > 0)
    {
    return;
    }

  animManager->GetVAnimationInterface()->GetTrackEditor()->
    AddKeyFrameButtonCallback();
  vtkPVSimpleAnimationCue *simpleAnimCue = animCue;
  simpleAnimCue->ReplaceKeyFrame(modeShape, animCue->GetSelectedKeyFrame());
}

//----------------------------------------------------------------------------
void vtkPVAnimateWarp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
