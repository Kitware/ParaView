#include "vruiTrackerState.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vruiTrackerState);

// ----------------------------------------------------------------------------
vruiTrackerState::vruiTrackerState()
{
  this->Position[0]=0.0;
  this->Position[1]=0.0;
  this->Position[2]=0.0;
  this->UnitQuaternion[0]=0.0;
  this->UnitQuaternion[1]=0.0;
  this->UnitQuaternion[2]=0.0;
  this->UnitQuaternion[3]=1.0;

  this->LinearVelocity[0]=0.0;
  this->LinearVelocity[1]=0.0;
  this->LinearVelocity[2]=0.0;

  this->AngularVelocity[0]=0.0;
  this->AngularVelocity[1]=0.0;
  this->AngularVelocity[2]=0.0;
}

// ----------------------------------------------------------------------------
vruiTrackerState::~vruiTrackerState()
{
}

// ----------------------------------------------------------------------------
void vruiTrackerState::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
