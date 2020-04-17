/*=========================================================================

Program: ParaView
Module:    pqItemViewSearchWidget.h

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

#ifndef pqItemViewSearchWidget_h
#define pqItemViewSearchWidget_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QModelIndex>

/**
* This is the search widget for QAbstractItemView type of widgets.
* When Ctrl-F is invoked on the view widget, this widget will show
* up at the top of the view widget, and it will search through the items
* as user typing in the text field, and highlight the view item
* containing the input text
*/
class PQCOMPONENTS_EXPORT pqItemViewSearchWidget : public QDialog
{
  Q_OBJECT
  Q_ENUMS(ItemSearchType)
  typedef QDialog Superclass;

public:
  pqItemViewSearchWidget(QWidget* parent = 0);
  ~pqItemViewSearchWidget() override;
  enum ItemSearchType
  {
    Current,
    Next,
    Previous
  };

public Q_SLOTS:
  virtual void showSearchWidget();

protected Q_SLOTS:

  /**
  * Given the user entered text, update the GUI.
  */
  virtual void updateSearch(QString);
  virtual void updateSearch();
  /**
  * Find the next/previous item
  */
  virtual void findNext();
  virtual void findPrevious();

protected:
  virtual void setBaseWidget(QWidget* baseWidget);
  /**
  * Overridden to capture key presses.
  */
  bool eventFilter(QObject* obj, QEvent* event) override;
  void keyPressEvent(QKeyEvent* e) override;
  /**
  * Recursive to search all QModelIndices in the model.
  */
  virtual bool searchModel(const QAbstractItemModel* M, const QModelIndex& Top, const QString& S,
    ItemSearchType searchType = Current) const;
  /**
  * Overwrite to focus the lineEdit box
  */
  void showEvent(QShowEvent*) override;
  /**
  * match the input string with the index's text
  */
  virtual bool matchString(
    const QAbstractItemModel* M, const QModelIndex& curIdx, const QString& searchString) const;

private:
  Q_DISABLE_COPY(pqItemViewSearchWidget)

  class PIMPL;
  PIMPL* Private;
};
#endif
