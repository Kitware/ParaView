/*=========================================================================

   Program: ParaView
   Module:    vtkVRMovePointStyle.h

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
/**
 * @class   vtkVRMovePointStyle
 * @brief   an interaction style to control the position of a point with a stylus
 *
 * vtkVRMovePointStyle is an interaction style that uses the position of the
 * tracker in screen space to modify the position of a 3D point.
 */
#ifndef vtkVRMovePointStyle_h_
#define vtkVRMovePointStyle_h_

#include "vtkVRTrackStyle.h"

struct vtkVREventData;

class vtkVRMovePointStyle : public vtkVRTrackStyle
{
public:
  static vtkVRMovePointStyle* New();
  vtkTypeMacro(vtkVRMovePointStyle, vtkVRTrackStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 3; }

protected:
  vtkVRMovePointStyle();
  ~vtkVRMovePointStyle() override = default;

  void HandleButton(const vtkVREventData& data) override;
  void HandleTracker(const vtkVREventData& data) override;

  bool EnableMovePoint;

private:
  vtkVRMovePointStyle(const vtkVRMovePointStyle&) = delete;
  void operator=(const vtkVRMovePointStyle&) = delete;

  double LastRecordedPosition[3];
  bool PositionRecorded;
};

#endif // vtkVRMovePointStyle.h_
