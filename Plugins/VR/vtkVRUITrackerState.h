#ifndef vtkVRUITrackerState_h
#define vtkVRUITrackerState_h

#include "vtkObject.h"

class vtkVRUITrackerState : public vtkObject
{
public:
  static vtkVRUITrackerState *New();
  vtkTypeMacro(vtkVRUITrackerState,vtkObjectBase);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Ditto.
  // Initial value is (0,0,0).
  vtkGetVector3Macro(Position,float);
  vtkSetVector3Macro(Position,float);

  // Description:
  // Unit quaternion representing the orientation.
  // Initial value is (0,0,0,1).
  vtkGetVector4Macro(UnitQuaternion,float);
  vtkSetVector4Macro(UnitQuaternion,float);

  // Description:
  // Linear velocity in units/s.
  // Initial value is (0,0,0).
  vtkGetVector3Macro(LinearVelocity,float);
  vtkSetVector3Macro(LinearVelocity,float);

  // Description:
  // Angular velocity in units/s.
  // Initial value is (0,0,0)
  vtkGetVector3Macro(AngularVelocity,float);
  vtkSetVector3Macro(AngularVelocity,float);

protected:
  vtkVRUITrackerState();
  ~vtkVRUITrackerState();

  float Position[3];
  float UnitQuaternion[4];

  float LinearVelocity[3];
  float AngularVelocity[3];

private:
  vtkVRUITrackerState(const vtkVRUITrackerState&); // Not implemented.
  void operator=(const vtkVRUITrackerState&); // Not implemented.
};


#endif // #ifndef vtkVRUITrackerState_h
