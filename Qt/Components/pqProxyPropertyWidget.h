/*=========================================================================

   Program: ParaView
   Module: pqProxyPropertyWidget.h

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

#ifndef _pqProxyPropertyWidget_h
#define _pqProxyPropertyWidget_h

#include "pqPropertyWidget.h"
#include <QPointer>

class pqSelectionInputWidget;
class pqProxySelectionWidget;

/**
* This is a widget for a vtkSMProxyProperty. It handles a "SelectionInput"
* property and properties with ProxyListDomain.
*/
class PQCOMPONENTS_EXPORT pqProxyPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqProxyPropertyWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = 0);

  /**
  * Overridden to pass the calls to internal widgets.
  */
  virtual void apply();
  virtual void reset();

  /**
  * These methods are called when the pqProxyPropertiesPanel containing the
  * widget is activated/deactivated. Only widgets that have 3D widgets need to
  * override these methods to select/deselect the 3D widgets.
  */
  virtual void select();
  virtual void deselect();

  /**
  * Overridden to hide the properties for proxies in a vtkSMProxyListDomain if
  * requested.
  */
  virtual void updateWidget(bool showing_advanced_properties);

private:
  QPointer<pqSelectionInputWidget> SelectionInputWidget;
  QPointer<pqProxySelectionWidget> ProxySelectionWidget;
};

#endif // _pqProxyPropertyWidget_h
