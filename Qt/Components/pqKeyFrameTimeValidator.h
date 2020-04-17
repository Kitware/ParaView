/*=========================================================================

   Program: ParaView
   Module:    pqKeyFrameTimeValidator.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef pqKeyFrameTimeValidator_h
#define pqKeyFrameTimeValidator_h

#include "pqComponentsModule.h"
#include <QDoubleValidator>

class pqAnimationScene;
class vtkSMDomain;

class PQCOMPONENTS_EXPORT pqKeyFrameTimeValidator : public QDoubleValidator
{
  Q_OBJECT
  typedef QDoubleValidator Superclass;

public:
  pqKeyFrameTimeValidator(QObject* parent);
  ~pqKeyFrameTimeValidator() override;

  // Set the AnimationScene. The ClockTimeRange from the time
  // keeper is used to determine the scale factor for the
  // range for this validator.
  void setAnimationScene(pqAnimationScene* timekeeper);

  // Set the domain which for this key time. Domain provides
  // the normalized range for this validator.
  void setDomain(vtkSMDomain* domain);

protected Q_SLOTS:
  void onDomainModified();

private:
  class pqInternals;
  pqInternals* Internals;
};

#endif
