/*=========================================================================

   Program: ParaView
   Module:    pqSampleScalarWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqSMProxy.h"
#include "pqWidgetsExport.h"

#include <QWidget>

namespace Ui { class pqSampleScalarWidget; }

class QItemSelection;
class QModelIndex;

class vtkSMDoubleVectorProperty;

/** Provides a standard user interface component for manipulating a list of
scalar samples.  Current uses include: specifying the set of "slices" for
the Cut filter, and specifying the set of contour values for the Contour filter.
*/

class PQWIDGETS_EXPORT pqSampleScalarWidget :
  public QWidget
{
  typedef QWidget base;

  Q_OBJECT

public:
  pqSampleScalarWidget(QWidget* Parent);
  ~pqSampleScalarWidget();

  /// Sets the server manager objects that will be controlled by the widget
  void setDataSources(
    pqSMProxy controlled_proxy,
    vtkSMDoubleVectorProperty* sample_property,
    vtkSMProperty* range_property = 0);

  /// Accept pending changes
  void accept();
  /// Reset pending changes
  void reset();

signals:
  /// Signal emitted whenever the set of samples changes.
  void samplesChanged();

private slots:
  void onSamplesChanged();
  void onSelectionChanged(const QItemSelection&, const QItemSelection&);
  
  void onDelete();
  void onNewValue();
  void onNewRange();
  void onSelectAll();
  void onScientificNotation(bool);
  
  void onControlledPropertyChanged();
  void onControlledPropertyDomainChanged();
  
private:
  pqSampleScalarWidget(const pqSampleScalarWidget&);
  pqSampleScalarWidget& operator=(const pqSampleScalarWidget&);
  
  class pqAddRangeDialog;
  
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqSampleScalarWidget_h
