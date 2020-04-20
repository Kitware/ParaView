/*=========================================================================

   Program: ParaView
   Module:  pqProxyEditorPropertyWidget.h

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
#ifndef pqProxyEditorPropertyWidget_h
#define pqProxyEditorPropertyWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"

class pqProxyWidgetDialog;
class QCheckBox;
class QPushButton;

/**
* Property widget that can be used to edit a proxy set as the value of a
* ProxyProperty in a pop-up dialog. e.g. RenderView proxy has a "GridActor3D"
* ProxyProperty which is set to a "GridActor3D" proxy. Using this widget
* allows the users to edit the properties on this GridActor3D, when present.
*
* To use this widget, add 'panel_widget="proxy_editor"' to the property's XML.
*
* Since it's common to have a property on the proxy being edited in the popup
* dialog which controls its visibility (or enabled state), we add support for
* putting that property as a checkbox in this pqProxyEditorPropertyWidget
* itself. For that, one can use `ProxyEditorPropertyWidget` hints.
*
* Example XML:
* @code
*   <ProxyProperty name="AxesGrid" ...  >
*     <ProxyListDomain name="proxy_list">
*       <Proxy group="annotations" name="GridAxes3DActor" />
*     </ProxyListDomain>
*     <Hints>
*       <ProxyEditorPropertyWidget property="Visibility" />
*     </Hints>
*   </ProxyProperty>
* @endcode
*/
class PQCOMPONENTS_EXPORT pqProxyEditorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy proxyToEdit READ proxyToEdit WRITE setProxyToEdit)

public:
  pqProxyEditorPropertyWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqProxyEditorPropertyWidget() override;

  pqSMProxy proxyToEdit() const;

public Q_SLOTS:
  void setProxyToEdit(pqSMProxy);

protected Q_SLOTS:
  void buttonClicked();

Q_SIGNALS:
  void dummySignal();

private:
  Q_DISABLE_COPY(pqProxyEditorPropertyWidget)

  vtkWeakPointer<vtkSMProxy> ProxyToEdit;
  QPointer<QPushButton> Button;
  QPointer<QCheckBox> Checkbox;
  QPointer<pqProxyWidgetDialog> Editor;
  QString PropertyName;
};

#endif
