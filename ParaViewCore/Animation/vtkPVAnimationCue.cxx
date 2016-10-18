/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationCue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationCue.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVCueManipulator.h"

//----------------------------------------------------------------------------
vtkPVAnimationCue::vtkPVAnimationCue()
{
  this->AnimatedElement = 0;
  this->Manipulator = 0;
  this->Enabled = true;
  this->UseAnimationTime = false;
  this->ObserverID = 0;
}

//----------------------------------------------------------------------------
vtkPVAnimationCue::~vtkPVAnimationCue()
{
  this->SetManipulator(0);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::SetManipulator(vtkPVCueManipulator* manipulator)
{
  if (manipulator == this->Manipulator)
  {
    return;
  }

  if (this->Manipulator && this->ObserverID != 0)
  {
    this->Manipulator->RemoveObserver(this->ObserverID);
  }
  vtkSetObjectBodyMacro(Manipulator, vtkPVCueManipulator, manipulator);

  if (this->Manipulator)
  {
    // Listen to the manipulator's ModifiedEvent. The manipilator fires this
    // event when the manipulator changes, its keyframes change or the values of
    // those key frames change. We simply propagate that event out so
    // applications can only listen to vtkPVAnimationCue for modification
    // of the entire track.
    this->ObserverID =
      this->Manipulator->AddObserver(vtkCommand::ModifiedEvent, this, &vtkPVAnimationCue::Modified);
  }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::StartCueInternal()
{
  this->Superclass::StartCueInternal();

  if (this->Manipulator)
  {
    // let the manipulator know that the cue has been restarted.
    this->Manipulator->Initialize(this);
  }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::EndCueInternal()
{
  this->Superclass::EndCueInternal();
  if (this->Manipulator)
  {
    // let the manipulator know that the cue has ended.
    this->Manipulator->Finalize(this);
  }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::TickInternal(double currenttime, double deltatime, double clocktime)
{
  double ctime = 0.0;
  if (this->StartTime != this->EndTime)
  {
    ctime = (currenttime - this->StartTime) / (this->EndTime - this->StartTime);
  }

  this->AnimationTime = currenttime;
  this->DeltaTime = deltatime;
  this->ClockTime = clocktime;

  if (this->UseAnimationTime)
  {
    this->BeginUpdateAnimationValues();
    this->SetAnimationValue(this->AnimatedElement, clocktime);
    this->EndUpdateAnimationValues();
  }
  else if (this->Manipulator)
  {
    this->Manipulator->UpdateValue(ctime, this);
  }

  this->Superclass::TickInternal(currenttime, deltatime, clocktime);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::Initialize()
{
  if (this->Enabled)
  {
    this->Superclass::Initialize();
  }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::Tick(double currenttime, double deltatime, double clocktime)
{
  if (this->Enabled)
  {
    this->Superclass::Tick(currenttime, deltatime, clocktime);
  }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::Finalize()
{
  if (this->Enabled)
  {
    this->Superclass::Finalize();
  }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimatedElement: " << this->AnimatedElement << endl;
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "Manipulator: " << this->Manipulator << endl;
}
