/*=========================================================================

   Program: ParaView
   Module:    vtkVRUITrackerState.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef vtkVRUITrackerState_h
#define vtkVRUITrackerState_h

#include "vtkObject.h"

class vtkVRUITrackerState : public vtkObject
{
public:
  static vtkVRUITrackerState* New();
  vtkTypeMacro(vtkVRUITrackerState, vtkObjectBase);
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Ditto.
  // Initial value is (0,0,0).
  vtkGetVector3Macro(Position, float);
  vtkSetVector3Macro(Position, float);

  // Description:
  // Unit quaternion representing the orientation.
  // Initial value is (0,0,0,1).
  vtkGetVector4Macro(UnitQuaternion, float);
  vtkSetVector4Macro(UnitQuaternion, float);

  // Description:
  // Linear velocity in units/s.
  // Initial value is (0,0,0).
  vtkGetVector3Macro(LinearVelocity, float);
  vtkSetVector3Macro(LinearVelocity, float);

  // Description:
  // Angular velocity in units/s.
  // Initial value is (0,0,0)
  vtkGetVector3Macro(AngularVelocity, float);
  vtkSetVector3Macro(AngularVelocity, float);

protected:
  vtkVRUITrackerState();
  ~vtkVRUITrackerState() override;

  float Position[3];
  float UnitQuaternion[4];

  float LinearVelocity[3];
  float AngularVelocity[3];

private:
  vtkVRUITrackerState(const vtkVRUITrackerState&) = delete;
  void operator=(const vtkVRUITrackerState&) = delete;
};

#endif // #ifndef vtkVRUITrackerState_h
