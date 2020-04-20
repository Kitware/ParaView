/*=========================================================================

   Program: ParaView
   Module:    pqSampleScalarWidget.h

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

#ifndef _pqSampleScalarWidget_h
#define _pqSampleScalarWidget_h

#include "pqComponentsModule.h"
#include "pqSMProxy.h"
#include "vtkSetGet.h" // for VTK_LEGACY_REMOVE

#if !defined(VTK_LEGACY_REMOVE)
#include <QList>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>

class QItemSelection;

class vtkSMDoubleVectorProperty;

/**
 * @class pqSampleScalarWidget
 *
 * @deprecated in ParaView 5.8. This class is left over from old properties
 * panel.
 *
 * Provides a standard user interface component for manipulating a list of
scalar samples.  Current uses include: specifying the set of "slices" for
the Cut filter, and specifying the set of contour values for the Contour filter.
*/

class PQCOMPONENTS_EXPORT pqSampleScalarWidget : public QWidget
{
  typedef QWidget Superclass;

  Q_OBJECT
  Q_PROPERTY(QVariantList samples READ samples WRITE setSamples)

public:
  // If preserve_order == true, then the widget will preserve value orders
  // and allow for duplicates, otherwise the values are unique and sorted.
  pqSampleScalarWidget(bool preserve_order, QWidget* Parent = 0);
  ~pqSampleScalarWidget() override;

  /**
  * Sets the server manager objects that will be controlled by the widget
  */
  void setDataSources(pqSMProxy controlled_proxy, vtkSMDoubleVectorProperty* sample_property,
    vtkSMProperty* range_property = 0);

  /**
  * Accept pending changes
  */
  void accept();
  /**
  * Reset pending changes
  */
  void reset();

  // Returns the samples currently selected in the widget
  // (these may differ from the accepted values).
  QList<QVariant> samples();

  // Set the current value of the widget.
  void setSamples(QList<QVariant> samples);
Q_SIGNALS:
  /**
  * Signal emitted whenever the set of samples changes.
  */
  void samplesChanged();

private Q_SLOTS:
  void onSamplesChanged();
  void onSelectionChanged(const QItemSelection&, const QItemSelection&);

  void onDelete();
  void onDeleteAll();
  void onNewValue();
  void onNewRange();
  void onSelectAll();
  void onScientificNotation(bool);

  void onControlledPropertyChanged();
  void onControlledPropertyDomainChanged();

private:
  pqSampleScalarWidget(const pqSampleScalarWidget&);
  pqSampleScalarWidget& operator=(const pqSampleScalarWidget&);

  bool getRange(double& range_min, double& range_max);

  class pqImplementation;
  pqImplementation* const Implementation;

  bool eventFilter(QObject* object, QEvent* e) override;
};

#endif // !defined(VTK_LEGACY_REMOVE)
#endif // !_pqSampleScalarWidget_h
