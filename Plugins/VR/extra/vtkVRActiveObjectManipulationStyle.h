/*=========================================================================

   Program: ParaView
   Module:    vtkVRActiveObjectManipulationStyle.h

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
#ifndef vtkVRActiveObjectManipulationStyle_h_
#define vtkVRActiveObjectManipulationStyle_h_

#include "vtkVRInteractorStyle.h"

class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
struct vtkVREventData;

class vtkVRActiveObjectManipulationStyle : public vtkVRInteractorStyle
{
  Q_OBJECT
  typedef vtkVRInteractorStyle Superclass;

public:
  vtkVRActiveObjectManipulationStyle(QObject* parent);
  ~vtkVRActiveObjectManipulationStyle();
  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);
  virtual vtkPVXMLElement* saveConfiguration() const;
  virtual bool handleEvent(const vtkVREventData& data);
  virtual bool update();

protected:
  void HandleAnalog(const vtkVREventData& data);
  void HandleSpaceNavigatorAnalog(const vtkVREventData& data);
  std::string Analog;

protected:
};

#endif // vtkVRActiveObjectManipulationStyle.h_
