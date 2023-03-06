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
#ifndef pqSelectionListPropertyWidget_h
#define pqSelectionListPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyGroupWidget.h"

#include <QScopedPointer>
#include <QVariant>

#include "pqSMProxy.h" // For property.

class vtkSMProxy;
class vtkSMPropertyGroup;

/**
 * pqSelectionListPropertyWidget is a custom widget used to associate a label for each selectionNode
 * from an input selection.
 *
 * It also use the pqSelectionInputWidget to know the number of labels we need to define, by copying
 * the active selection.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSelectionListPropertyWidget : public pqPropertyGroupWidget
{
  Q_OBJECT
  typedef pqPropertyGroupWidget Superclass;
  Q_PROPERTY(QList<QVariant> labels READ labels WRITE setLabels NOTIFY labelsChanged);

public:
  pqSelectionListPropertyWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = nullptr);
  ~pqSelectionListPropertyWidget() override;

  /**
   * Methods used to set/get the label values for the widget.
   *
   * The number of labels required depend of the number of SelectionNode in the activeSelection
   * provided by the pqSelectionInputWidget.
   */
  QList<QVariant> labels() const;
  void setLabels(const QList<QVariant>& labels);

Q_SIGNALS:
  /**
   * Signal for the labels proxy changed.
   */
  void labelsChanged();

protected Q_SLOTS:
  /**
   * Clone the active selection handled by the selection manager.
   */
  void populateRowLabels(pqSMProxy appendSelection);

private:
  Q_DISABLE_COPY(pqSelectionListPropertyWidget)

  class pqUi;
  QScopedPointer<pqUi> Ui;
};

#endif
