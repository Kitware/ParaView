/*=========================================================================

   Program: ParaView
   Module:  pqRemoveLightWidget.h

   Copyright (c) 2017 Sandia Corporation, Kitware Inc.
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
#ifndef pqRemoveLightWidget_h
#define pqRemoveLightWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QPointer>

class QPushButton;

/**
 * @class pqRemoveLightWidget
 * @brief Remove a light from the scene.
 *
 * pqRemoveLightWidget is a pqPropertyWidget that sends a signal to remove
 * this
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqRemoveLightWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqRemoveLightWidget(vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqRemoveLightWidget() override;

signals:
  void removeLight(vtkSMProxy* smproxy);

private slots:
  void buttonPressed();

private:
  Q_DISABLE_COPY(pqRemoveLightWidget)
  QPointer<QPushButton> PushButton;
};

#endif
