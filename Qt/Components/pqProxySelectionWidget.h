/*=========================================================================

   Program:   ParaView
   Module:    pqProxySelectionWidget.h

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
#ifndef pqProxySelectionWidget_h
#define pqProxySelectionWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"
#include <QScopedPointer>

class vtkSMProxy;
class vtkSMProperty;
class pqView;

/**
* pqPropertyWidget that can be used for any proxy with a vtkSMProxyListDomain.
* pqProxyPropertyWidget automatically creates this widget when it encounters a
* property with vtkSMProxyListDomain.
*/
class PQCOMPONENTS_EXPORT pqProxySelectionWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy chosenProxy READ chosenProxy WRITE setChosenProxy)

public:
  /**
  * constructor requires the proxy, property. Note that this will abort if the
  * property does not have a ProxyListDomain.
  */
  pqProxySelectionWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = 0);
  ~pqProxySelectionWidget() override;

  /**
  * get the selected proxy
  */
  vtkSMProxy* chosenProxy() const;
  void setChosenProxy(vtkSMProxy* proxy);

  /**
  * Overridden to forward the call to the internal pqProxyWidget maintained
  * for the chosen proxy.
  */
  void apply() override;
  void reset() override;
  void select() override;
  void deselect() override;
  void updateWidget(bool showing_advanced_properties) override;
  void setPanelVisibility(const char* vis) override;
  void setView(pqView*) override;

Q_SIGNALS:
  /**
  * Signal fired by setChosenProxy() when the proxy changes.
  */
  void chosenProxyChanged();

private Q_SLOTS:
  /**
  * Called when the current index in the combo-box is changed from the UI.
  * This calls setChosenProxy() with the argument as the proxy corresponding
  * to the \c idx from the domain.
  */
  void currentIndexChanged(int);

private:
  class pqInternal;
  const QScopedPointer<pqInternal> Internal;

  Q_DISABLE_COPY(pqProxySelectionWidget)
};

#endif
