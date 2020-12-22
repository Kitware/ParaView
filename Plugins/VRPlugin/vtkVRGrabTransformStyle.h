/*=========================================================================

   Program: ParaView
   Module:  vtkVRGrabTransfromStyle.h

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
#ifndef vtkVRGrabTransfromStyle_h
#define vtkVRGrabTransfromStyle_h

#include "vtkNew.h"
#include "vtkVRTrackStyle.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class vtkVRGrabTransformStyle : public vtkVRTrackStyle
{
public:
  static vtkVRGrabTransformStyle* New();
  vtkTypeMacro(vtkVRGrabTransformStyle, vtkVRTrackStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkVRGrabTransformStyle();
  ~vtkVRGrabTransformStyle() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableNavigate;    /* mirrors the button assigned the "Navigate World" role */
  bool IsInitialRecorded; /* flag indicating that we're in the middle of a navigation operation */

  vtkNew<vtkMatrix4x4> SavedModelViewMatrix;
  vtkNew<vtkMatrix4x4> SavedInverseWandMatrix;

private:
  vtkVRGrabTransformStyle(const vtkVRGrabTransformStyle&) = delete;
  void operator=(const vtkVRGrabTransformStyle&) = delete;

  float GetSpeedFactor(vtkCamera* cam, vtkMatrix4x4* mvmatrix); /* WRS-TODO: what does this do? */
  vtkCamera* GetCamera();                                       // WRS-TODO: how is this used?
};

#endif // vtkVRGrabTransfromStyle_h
