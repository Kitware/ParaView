/*=========================================================================

   Program:   ParaView
   Module:    pqLanguageChooserWidget.h

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

========================================================================*/
#ifndef pqLanguageChooserWidget_h
#define pqLanguageChooserWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QPointer>

class QComboBox;

/**
 * pqLanguageChooserWidget is a property widget that shows a combo-box with
 * values equal to the currently available translation languages.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLanguageChooserWidget : public pqPropertyWidget
{
  Q_OBJECT;
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QString value READ value WRITE setValue);

public:
  pqLanguageChooserWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqLanguageChooserWidget() override;

  /**
   * get the current selected locale.
   */
  QString value() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * set the default value for the widget among the current items. If the value
   * is not in the items, add it, make it default and re-sort the list alphabetically.
   */
  void setValue(QString& value);

Q_SIGNALS:
  void valueChanged();

private:
  Q_DISABLE_COPY(pqLanguageChooserWidget)
  QPointer<QComboBox> ComboBox;
};

#endif
