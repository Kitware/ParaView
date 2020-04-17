/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqListPropertyWidget_h
#define pqListPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QVariant>

class QTableWidget;

/**
* pqListPropertyWidget is a pqPropertyWidget that is used to show an editable
* list of elements. This is suitable for int/idtype/double/string vector
* properties with multiple elements, useful for allowing the user to change
* a range of values without adding or removing entries from it.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqListPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QList<QVariant> value READ value WRITE setValue NOTIFY valueChanged);

public:
  explicit pqListPropertyWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqListPropertyWidget() override;

  /**
  * Methods used to set/get the value for the widget.
  */
  QList<QVariant> value() const;
  void setValue(const QList<QVariant>& value);

Q_SIGNALS:
  void valueChanged();

private:
  Q_DISABLE_COPY(pqListPropertyWidget)
  QTableWidget* TableWidget;
};

#endif
