/*=========================================================================

   Program: ParaView
   Module:  vtkSMVRVirtualHandStyleProxy.h

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
#ifndef vtkSMVRVirtualHandStyleProxy_h
#define vtkSMVRVirtualHandStyleProxy_h

#include "vtkInteractionStylesModule.h" // for export macro
#include "vtkNew.h"
#include "vtkSMVRTrackStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKINTERACTIONSTYLES_EXPORT vtkSMVRVirtualHandStyleProxy : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRVirtualHandStyleProxy* New();
  vtkTypeMacro(vtkSMVRVirtualHandStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRVirtualHandStyleProxy();
  ~vtkSMVRVirtualHandStyleProxy(); // WRS-TODO: no "override" here?  Others have it.

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool CurrentButton;
  bool PrevButton;

  bool EventPress;
  bool EventRelease;

  vtkNew<vtkMatrix4x4> CurrentTrackerMatrix;
  vtkNew<vtkMatrix4x4> InverseTrackerMatrix;
  vtkNew<vtkMatrix4x4> CachedModelMatrix;
  vtkNew<vtkMatrix4x4> NewModelMatrix;

private:
  vtkSMVRVirtualHandStyleProxy(const vtkSMVRVirtualHandStyleProxy&) = delete;
  void operator=(const vtkSMVRVirtualHandStyleProxy&) = delete;
};

#endif // vtkSMVRVirtualHandStyleProxy_h
