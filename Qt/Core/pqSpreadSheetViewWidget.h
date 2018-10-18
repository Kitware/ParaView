/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewWidget.h

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
#ifndef pqSpreadSheetViewWidget_h
#define pqSpreadSheetViewWidget_h

#include "pqCoreModule.h"
#include <QTableView>

class pqSpreadSheetViewModel;

/**
* pqSpreadSheetViewWidget is a  QTableView subclass that is used by
* pqSpreadSheetView as the widget showing the data. Although it's called a
* 'Widget' it's not a QTableWidget but a QTableView subclass. It works with a
* pqSpreadSheetViewModel to show raw data delivered by the
* vtkSMSpreadSheetRepresentationProxy.
*
* pqSpreadSheetViewWidget uses an internal QItemDelegate subclass to handle
* determining of the active viewport as well as showing multi-component
* arrays. Users are advised not to change the item delegate on the view.
*/
class PQCORE_EXPORT pqSpreadSheetViewWidget : public QTableView
{
  Q_OBJECT
  typedef QTableView Superclass;

public:
  pqSpreadSheetViewWidget(QWidget* parent = 0);
  ~pqSpreadSheetViewWidget() override;

  /**
  * Overridden to ensure that the model is a pqSpreadSheetViewModel.
  */
  void setModel(QAbstractItemModel* model) override;

  /**
  * Returns the spread sheetview model for this view.
  */
  pqSpreadSheetViewModel* spreadSheetViewModel() const;

protected slots:
  /**
  * called when a header section is clicked in order to be sorted.
  * It results in that column being sorted if possible.
  */
  void onSortIndicatorChanged(int section, Qt::SortOrder order);

  /**
  * called when header data changes. We ensure that internal columns stay
  * hidden.
  */
  void onHeaderDataChanged();

protected:
  /**
  * Overridden to tell the pqSpreadSheetViewModel about the active viewport.
  */
  void paintEvent(QPaintEvent* event) override;

private:
  Q_DISABLE_COPY(pqSpreadSheetViewWidget)

  class pqDelegate;

  bool SingleColumnMode;
  int OldColumnCount;
};

#endif
