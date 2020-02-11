/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef vtkVRControlSliceOrientationStyle_h
#define vtkVRControlSliceOrientationStyle_h

#include "vtkNew.h"
#include "vtkVRInteractorStyle.h"

class vtkMatrix4x4;

class vtkVRControlSliceOrientationStyle : public vtkVRInteractorStyle
{
public:
  static vtkVRControlSliceOrientationStyle* New();
  vtkTypeMacro(vtkVRControlSliceOrientationStyle, vtkVRInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 3; }

  /// called to update all the remote vtkObjects and perhaps even to render.
  /// Typically processing intensive operations go here. The method should not
  /// be called from within the handler and is reserved to be called from an
  /// external interaction style manager.
  bool Update() override;

protected:
  vtkVRControlSliceOrientationStyle();
  ~vtkVRControlSliceOrientationStyle() override;

  void HandleButton(const vtkVREventData& data) override;
  void HandleTracker(const vtkVREventData& data) override;

  bool Enabled;
  bool InitialOrientationRecorded;

  double InitialQuat[4];
  double InitialTrackerQuat[4];
  double UpdatedQuat[4];
  double Normal[4];

  vtkNew<vtkMatrix4x4> InitialInvertedPose;

private:
  vtkVRControlSliceOrientationStyle(const vtkVRControlSliceOrientationStyle&) = delete;
  void operator=(const vtkVRControlSliceOrientationStyle&) = delete;
};

#endif
