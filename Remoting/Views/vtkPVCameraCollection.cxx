/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraCollection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraCollection.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkVectorOperators.h"

#include <vector>

namespace
{
double GetSimilarity(vtkCamera* ac, vtkCamera* bc)
{
  vtkVector3d a = vtkVector3d(ac->GetFocalPoint()) - vtkVector3d(ac->GetPosition());
  vtkVector3d b = vtkVector3d(bc->GetFocalPoint()) - vtkVector3d(bc->GetPosition());
  a.Normalize();
  b.Normalize();
  return (a - b).Norm();
}
}
class vtkPVCameraCollection::vtkInternals
{
public:
  typedef std::vector<vtkSmartPointer<vtkCamera> > CamerasType;
  CamerasType Cameras;
};

vtkStandardNewMacro(vtkPVCameraCollection);
//----------------------------------------------------------------------------
vtkPVCameraCollection::vtkPVCameraCollection()
{
  this->Internals = new vtkInternals();
  this->LastCameraIndex = -1;
}

//----------------------------------------------------------------------------
vtkPVCameraCollection::~vtkPVCameraCollection()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVCameraCollection::RemoveAllCameras()
{
  if (this->Internals->Cameras.size() > 0)
  {
    this->Internals->Cameras.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPVCameraCollection::AddCamera(vtkCamera* camera)
{
  this->Internals->Cameras.push_back(camera);
  this->Modified();
  return static_cast<int>(this->Internals->Cameras.size()) - 1;
}

//----------------------------------------------------------------------------
vtkCamera* vtkPVCameraCollection::GetCamera(int index)
{
  if (index >= 0 && index < static_cast<int>(this->Internals->Cameras.size()))
  {
    return this->Internals->Cameras[index];
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkPVCameraCollection::FindClosestCamera(vtkCamera* target)
{
  int index = -1;
  double similarity = 0;

  int counter = 0;
  const vtkInternals::CamerasType& cameras = this->Internals->Cameras;
  for (vtkInternals::CamerasType::const_iterator iter = cameras.begin(); iter != cameras.end();
       ++iter, ++counter)
  {
    double cursim = GetSimilarity(target, iter->GetPointer());
    if (cursim < similarity || index == -1)
    {
      similarity = cursim;
      index = counter;
    }
  }
  return index;
}

//----------------------------------------------------------------------------
bool vtkPVCameraCollection::UpdateCamera(int index, vtkCamera* target)
{
  vtkCamera* source = this->GetCamera(index);
  if (source == nullptr || target == nullptr)
  {
    return false;
  }

  // We do not copy ViewUp. See vtkCinemaLayerMapper for the code that rolls the
  // rendered image based on  the current viewup.
  target->SetPosition(source->GetPosition());
  target->SetFocalPoint(source->GetFocalPoint());
  target->SetClippingRange(source->GetClippingRange());
  return true;
}

//----------------------------------------------------------------------------
void vtkPVCameraCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
