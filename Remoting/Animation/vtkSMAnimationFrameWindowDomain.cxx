/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationFrameWindowDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationFrameWindowDomain.h"

#include "vtkCompositeAnimationPlayer.h"
#include "vtkObjectFactory.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMAnimationFrameWindowDomain);
//----------------------------------------------------------------------------
vtkSMAnimationFrameWindowDomain::vtkSMAnimationFrameWindowDomain() = default;

//----------------------------------------------------------------------------
vtkSMAnimationFrameWindowDomain::~vtkSMAnimationFrameWindowDomain() = default;

//----------------------------------------------------------------------------
void vtkSMAnimationFrameWindowDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* sceneProperty = this->GetRequiredProperty("AnimationScene");
  vtkSMProperty* frameRateProperty = this->GetRequiredProperty("FrameRate");
  if (!sceneProperty)
  {
    vtkErrorMacro("Missing required 'AnimationScene' property.");
    return;
  }
  if (!frameRateProperty)
  {
    vtkErrorMacro("Missing required 'FrameRate' property.");
    return;
  }

  std::vector<vtkEntry> values;
  if (vtkSMProxy* scene = vtkSMUncheckedPropertyHelper(sceneProperty).GetAsProxy())
  {
    int playMode = vtkSMUncheckedPropertyHelper(scene, "PlayMode").GetAsInt();
    switch (playMode)
    {
      case vtkCompositeAnimationPlayer::SEQUENCE:
      {
        int numFrames = vtkSMUncheckedPropertyHelper(scene, "NumberOfFrames").GetAsInt();
        values.push_back(vtkEntry(0, numFrames - 1));
      }
      break;
      case vtkCompositeAnimationPlayer::REAL_TIME:
      {
        int frameRate = vtkSMUncheckedPropertyHelper(frameRateProperty).GetAsInt();
        double duration = vtkSMUncheckedPropertyHelper(scene, "Duration").GetAsInt();
        // (see #17031)
        int numFrames = 1 + (frameRate * duration);
        values.push_back(vtkEntry(0, numFrames - 1));
      }
      break;
      case vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS:
      {
        vtkSMProxy* timeKeeper = vtkSMUncheckedPropertyHelper(scene, "TimeKeeper").GetAsProxy();
        int numTS =
          vtkSMUncheckedPropertyHelper(timeKeeper, "TimestepValues").GetNumberOfElements();
        values.push_back(vtkEntry(0, numTS - 1));
      }
      break;
    }
  }
  else
  {
    // no scene, nothing much to do.
    values.push_back(vtkEntry(0, 0));
  }
  this->SetEntries(values);
}

//----------------------------------------------------------------------------
void vtkSMAnimationFrameWindowDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
