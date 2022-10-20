/*=========================================================================

   Program: ParaView
   Module:  vtkSMVRGrabWorldStyleProxy.h

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
#ifndef vtkSMVRGrabWorldStyleProxy_h
#define vtkSMVRGrabWorldStyleProxy_h

#include "vtkInteractionStylesModule.h" // for export macro
#include "vtkNew.h"
#include "vtkSMVRTrackStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKINTERACTIONSTYLES_EXPORT vtkSMVRGrabWorldStyleProxy : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRGrabWorldStyleProxy* New();
  vtkTypeMacro(vtkSMVRGrabWorldStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRGrabWorldStyleProxy();
  ~vtkSMVRGrabWorldStyleProxy() override;

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
  vtkSMVRGrabWorldStyleProxy(const vtkSMVRGrabWorldStyleProxy&) = delete; // Not implemented
  void operator=(const vtkSMVRGrabWorldStyleProxy&) = delete;             // Not implemented

  float GetSpeedFactor(vtkCamera* cam, vtkMatrix4x4* mvmatrix); /* WRS: what does this do? */
  vtkCamera* GetCamera();
};

#endif // vtkSMVRGrabWorldStyleProxy_h
