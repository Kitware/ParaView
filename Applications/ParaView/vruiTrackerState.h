#ifndef vruiTrackerState_h
#define vruiTrackerState_h

#include "vtkObjectBase.h"

class vruiTrackerState : public vtkObjectBase
{
public:
  static vruiTrackerState *New();
  vtkTypeMacro(vruiTrackerState,vtkObjectBase);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Ditto.
  // Initial value is (0,0,0).
  vtkGetVector3Macro(Position,double);
  vtkSetVector3Macro(Position,double);

  // Description:
  // Unit quaternion representing the orientation.
  // Initial value is (0,0,0,1).
  vtkGetVector4Macro(UnitQuaternion,double);
  vtkSetVector4Macro(UnitQuaternion,double);

  // Description:
  // Linear velocity in units/s.
  // Initial value is (0,0,0).
  vtkGetVector3Macro(LinearVelocity,double);
  vtkSetVector3Macro(LinearVelocity,double);

  // Description:
  // Angular velocity in units/s.
  // Initial value is (0,0,0)
  vtkGetVector3Macro(AngularVelocity,double);
  vtkSetVector3Macro(AngularVelocity,double);

protected:
  vruiTrackerState();
  ~vruiTrackerState();

  double Position[3];
  double UnitQuaternion[4];

  double LinearVelocity[3];
  double AngularVelocity[3];

private:
  vruiTrackerState(const vruiTrackerState&); // Not implemented.
  void operator=(const vruiTrackerState&); // Not implemented.
};


#endif // #ifndef vruiTrackerState_h
