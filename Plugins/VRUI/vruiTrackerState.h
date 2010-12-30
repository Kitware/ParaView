#ifndef vruiTrackerState_h
#define vruiTrackerState_h

#include "vtkObject.h"

class vruiTrackerState : public vtkObject
{
public:
  static vruiTrackerState *New();
  vtkTypeMacro(vruiTrackerState,vtkObjectBase);
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
  vruiTrackerState();
  ~vruiTrackerState();

  float Position[3];
  float UnitQuaternion[4];

  float LinearVelocity[3];
  float AngularVelocity[3];

private:
  vruiTrackerState(const vruiTrackerState&); // Not implemented.
  void operator=(const vruiTrackerState&); // Not implemented.
};


#endif // #ifndef vruiTrackerState_h
