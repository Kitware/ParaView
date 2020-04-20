/*=========================================================================

   Program: ParaView
   Module: pqCommandButtonPropertyWidget.h

   Copyright (c) 2005-2012 Kitware Inc.
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

#ifndef _pqCommandButtonPropertyWidget_h
#define _pqCommandButtonPropertyWidget_h

#include "pqApplicationComponentsModule.h"

#include "pqPropertyWidget.h"

/**
* A property widget with a push button for invoking a command on a proxy.
*
* To use this widget for a property add the 'panel_widget="command_button"'
* to the property's XML.
*
* If the property has a "command" attribute set, then the command will be invoked
* immediately. If the property has no command set, then clicking the button will call
* `vtkSMProxy::RecreateVTKObjects()`.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqCommandButtonPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  explicit pqCommandButtonPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = 0);
  ~pqCommandButtonPropertyWidget() override;

protected Q_SLOTS:
  virtual void buttonClicked();

private:
  vtkSMProperty* Property;
};

#endif // _pqCommandButtonPropertyWidget_h
