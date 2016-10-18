/*=========================================================================

  Program:   ParaView
  Module:    vtkPVKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVKeyFrame.h"

#include "vtkObjectFactory.h"

#include <vector>
//----------------------------------------------------------------------------
class vtkPVKeyFrameInternals
{
public:
  typedef std::vector<double> VectorOfDoubles;
  VectorOfDoubles KeyValues;
};
//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPVKeyFrame);
//----------------------------------------------------------------------------
vtkPVKeyFrame::vtkPVKeyFrame()
{
  this->KeyTime = -1.0;
  this->Internals = new vtkPVKeyFrameInternals;
}

//----------------------------------------------------------------------------
vtkPVKeyFrame::~vtkPVKeyFrame()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrame::UpdateValue(double vtkNotUsed(currenttime), vtkPVAnimationCue* vtkNotUsed(cue),
  vtkPVKeyFrame* vtkNotUsed(next))
{
}

//----------------------------------------------------------------------------
void vtkPVKeyFrame::RemoveAllKeyValues()
{
  this->Internals->KeyValues.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVKeyFrame::SetKeyValue(unsigned int index, double value)
{
  if (index >= this->GetNumberOfKeyValues())
  {
    this->SetNumberOfKeyValues(index + 1);
  }
  this->Internals->KeyValues[index] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkPVKeyFrame::GetKeyValue(unsigned int index)
{
  if (index >= this->GetNumberOfKeyValues())
  {
    return 0.0;
  }
  return this->Internals->KeyValues[index];
}

//----------------------------------------------------------------------------
void vtkPVKeyFrame::SetNumberOfKeyValues(unsigned int num)
{
  this->Internals->KeyValues.resize(num);
}

//----------------------------------------------------------------------------
unsigned int vtkPVKeyFrame::GetNumberOfKeyValues()
{
  return static_cast<unsigned int>(this->Internals->KeyValues.size());
}

//----------------------------------------------------------------------------
void vtkPVKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KeyTime: " << this->KeyTime << endl;
}
