/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeAnimationPlayer.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

class vtkCompositeAnimationPlayer::vtkInternal
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkAnimationPlayer> > VectorOfPlayers;
  VectorOfPlayers Players;
  vtkSmartPointer<vtkAnimationPlayer> ActivePlayer;
};

vtkStandardNewMacro(vtkCompositeAnimationPlayer);
//----------------------------------------------------------------------------
vtkCompositeAnimationPlayer::vtkCompositeAnimationPlayer()
{
  this->Internal = new vtkInternal();
}

//----------------------------------------------------------------------------
vtkCompositeAnimationPlayer::~vtkCompositeAnimationPlayer()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
int vtkCompositeAnimationPlayer::AddPlayer(vtkAnimationPlayer* player)
{
  if (!player)
    {
    return -1;
    }

  int index =0;
  vtkInternal::VectorOfPlayers::iterator iter;
  for (iter = this->Internal->Players.begin(); 
    iter != this->Internal->Players.end(); ++iter, ++index)
    {
    if (iter->GetPointer() == player)
      {
      return index;
      }
    }

  this->Internal->Players.push_back(player);
  return index;
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::RemoveAllPlayers()
{
  this->Internal->Players.clear();
  this->Internal->ActivePlayer = 0;
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::SetActive(int index)
{
  this->Internal->ActivePlayer = 0;
  if (index >= 0 && index < static_cast<int>(this->Internal->Players.size()))
    {
    this->Internal->ActivePlayer = this->Internal->Players[index];
    }
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::StartLoop(double starttime, double endtime, double currenttime)
{
  if (this->Internal->ActivePlayer)
    {
    this->Internal->ActivePlayer->StartLoop(starttime, endtime, currenttime);
    }
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::EndLoop()
{
  if (this->Internal->ActivePlayer)
    {
    this->Internal->ActivePlayer->EndLoop();
    }
}

//----------------------------------------------------------------------------
double vtkCompositeAnimationPlayer::GetNextTime(double currentime)
{
  if (this->Internal->ActivePlayer)
    {
    return this->Internal->ActivePlayer->GetNextTime(currentime);
    }

  return VTK_DOUBLE_MAX;
}

//----------------------------------------------------------------------------
double vtkCompositeAnimationPlayer::GoToNext(double start, double end, 
  double currenttime)
{
  if (this->Internal->ActivePlayer)
    {
    return this->Internal->ActivePlayer->GoToNext(start, end, currenttime);
    }

  return VTK_DOUBLE_MAX;
}

//----------------------------------------------------------------------------
double vtkCompositeAnimationPlayer::GoToPrevious(double start, double end, 
  double currenttime)
{
  if (this->Internal->ActivePlayer)
    {
    return this->Internal->ActivePlayer->GoToPrevious(start, end, currenttime);
    }

  return VTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

