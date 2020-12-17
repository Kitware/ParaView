/*=========================================================================

   Program: ParaView
   Module:  vtkVRGrabWorldStyle.h

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
#ifndef vtkVRGrabWorldStyle_h
#define vtkVRGrabWorldStyle_h

#include "vtkNew.h"
#include "vtkVRTrackStyle.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class vtkVRGrabWorldStyle : public vtkVRTrackStyle
{
public:
  static vtkVRGrabWorldStyle* New();
  vtkTypeMacro(vtkVRGrabWorldStyle, vtkVRTrackStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkVRGrabWorldStyle();
  ~vtkVRGrabWorldStyle() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableTranslate; /* mirrors the button assigned the "Translate World" role */
  bool EnableRotate;    /* mirrors the button assigned the "Rotate World" role */

  bool IsInitialTransRecorded; /* flag indicating that we're in the middle of a translation
                                  operation */
  bool IsInitialRotRecorded; /* flag indicating that we're in the middle of a rotation operation */
                             /* NOTE: only one of translation or rotation can be active at a time */

  vtkNew<vtkMatrix4x4> InverseInitialTransMatrix;
  vtkNew<vtkMatrix4x4> InverseInitialRotMatrix;

  vtkNew<vtkMatrix4x4> CachedTransMatrix;
  vtkNew<vtkMatrix4x4> CachedRotMatrix;

private:
  vtkVRGrabWorldStyle(const vtkVRGrabWorldStyle&) = delete; // Not implemented
  void operator=(const vtkVRGrabWorldStyle&) = delete;      // Not implemented

  float GetSpeedFactor(vtkCamera* cam, vtkMatrix4x4* mvmatrix); /* WRS: what does this do? */
  vtkCamera* GetCamera();
};

#endif // vtkVRGrabWorldStyle_h
