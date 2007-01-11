/*=========================================================================

   Program: ParaView
   Module:    pqKeyTimeDomain.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/
#ifndef __pqKeyTimeDomain_h
#define __pqKeyTimeDomain_h

#include "pqDoubleSpinBoxDomain.h"
#include <QPointer>

class pqAnimationScene;
// pqKeyTimeDomain is an specialization of pqDoubleSpinBoxDomain which
// scales the bounds given by the domain using the start and end times
// from the Animation Scene.
class PQCOMPONENTS_EXPORT pqKeyTimeDomain : public pqDoubleSpinBoxDomain
{
  Q_OBJECT
  typedef pqDoubleSpinBoxDomain Superclass;
public:
  /// constructor requires a QDoubleSpinBox, 
  /// and the property with the domain to observe
  /// the list of values in the combo box is automatically 
  /// updated when the domain changes
  pqKeyTimeDomain(QDoubleSpinBox* p, vtkSMProperty* prop, int index=-1);
  ~pqKeyTimeDomain();

  void setAnimationScene(pqAnimationScene* scene);
protected:
  virtual void setSingleStep(double step);
  virtual void setRange(double min, double max);

  QPointer<pqAnimationScene> Scene;
};

#endif

