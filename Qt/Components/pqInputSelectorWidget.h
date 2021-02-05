/*=========================================================================

   Program: ParaView
   Module:  pqInputSelectorWidget.h

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
#ifndef pqInputSelectorWidget_h
#define pqInputSelectorWidget_h

#include "pqComponentsModule.h" // for exports
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"       // for pqSMProxy.
#include "pqTimer.h"         // for pqTimer
#include "vtkSmartPointer.h" // for vtkSmartPointer

class QComboBox;
class vtkSMProxy;

/**
 * @class pqInputSelectorWidget
 * @brief widget for input property to choose a pipeline input.
 *
 * Generally, pqProxyWidget does not show widget for input properties. This is
 * by design since changing input to filters is generally not a thing we let the
 * user treat at same level as changing other properties. In some cases,
 * however, we may want to show a combo-box to let user pick the input from the
 * available pipeline proxies. pqInputSelectorWidget is designed for such
 * exceptional cases. To use this widget for a vtkSMInputProperty, set
 * `panel_widget` attribute to `input_selector`.
 *
*/
class PQCOMPONENTS_EXPORT pqInputSelectorWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy selectedInput READ selectedInput WRITE setSelectedInput)

public:
  pqInputSelectorWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqInputSelectorWidget() override;

  /**
   * Chosen port. This returns `vtkSmartPointer<vtkSMOutputPort>`
   * for the chosen port. if any.
   */
  pqSMProxy selectedInput() const;

public Q_SLOTS:
  void setSelectedInput(pqSMProxy);

Q_SIGNALS:
  void selectedInputChanged();

private Q_SLOTS:
  void updateComboBox();

private:
  Q_DISABLE_COPY(pqInputSelectorWidget);
  QComboBox* ComboBox;
  vtkWeakPointer<vtkSMProxy> ChosenPort;
  pqTimer UpdateComboBoxTimer;
};

#endif
