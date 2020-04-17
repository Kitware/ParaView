/*=========================================================================

   Program: ParaView
   Module: pqIndexSelectionWidget

   Copyright (c) 2015 Sandia Corporation, Kitware Inc.
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

#ifndef pqIndexSelectionWidget_h
#define pqIndexSelectionWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QMap>
#include <QString>

class QFormLayout;
class QGroupBox;
class QVBoxLayout;

/**
* pqIndexSelectionWidget displays a list of labels and slider widgets,
* intended to be used for selecting an index into a zero-based enumeration.
*/
class PQCOMPONENTS_EXPORT pqIndexSelectionWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqIndexSelectionWidget(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = 0);
  ~pqIndexSelectionWidget() override;

public Q_SLOTS:
  void setHeaderLabel(const QString& str);
  void setPushPropertyName(const QByteArray& pName);

Q_SIGNALS:
  void widgetModified();

protected Q_SLOTS:
  void currentChanged(const QString& current);
  void currentChanged(int current);
  void currentChanged(); // pqLineEdit::textChangedAndEditingFinished() handler

  /**
  * Start a timer that calls updatePropertyImpl. This slot is triggered when
  * a widget is modified.
  */
  void updateProperty();
  /**
  * Update the Qt property with current widget state. Emits widgetModified.
  */
  void updatePropertyImpl();

protected:
  bool eventFilter(QObject* obj, QEvent* e) override;
  /**
  * Update the widget state from the PropertyLink Qt property.
  */
  void propertyChanged();

private:
  Q_DISABLE_COPY(pqIndexSelectionWidget)

  void buildWidget(vtkSMProperty* infoProp);
  void addRow(const QString& key, int current, int size);

  /**
  * The names of the Qt properties used to sync with the vtkSMProperty.
  */
  QByteArray PushPropertyName;

  bool PropertyUpdatePending;     // Only update the property once per 250 ms.
  bool IgnorePushPropertyUpdates; // don't react to our own updates.

  QGroupBox* GroupBox;
  QVBoxLayout* VBox;
  QFormLayout* Form;

  class pqInternals;
  pqInternals* Internals;
};

#endif // pqIndexSelectionWidget_h
