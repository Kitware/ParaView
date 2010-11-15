/*=========================================================================

   Program: ParaView
   Module:    pqCheckableHeaderView.h

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

/// \file pqCheckableHeaderView.h
/// \date 8/17/2007

#ifndef _pqCheckableHeaderView_h
#define _pqCheckableHeaderView_h


#include "QtWidgetsExport.h"
#include <QHeaderView>

class pqCheckableHeaderViewInternal;


class QTWIDGETS_EXPORT pqCheckableHeaderView : public QHeaderView
{
  Q_OBJECT

public:
  pqCheckableHeaderView(Qt::Orientation orient, QWidget *parent=0);
  virtual ~pqCheckableHeaderView();

  /// \brief
  ///   Used to listen for focus in/out events.
  /// \param object The object receiving the event.
  /// \param e Event specific data.
  /// \return
  ///   True if the event should be filtered out.
  virtual bool eventFilter(QObject *object, QEvent *e);

  virtual void mousePressEvent(QMouseEvent *event);

  virtual void setModel(QAbstractItemModel *model);
  virtual void setRootIndex(const QModelIndex &index);

signals:
  void checkStateChanged();

public slots:
  void toggleCheckState(int section);

private slots:
  void initializeIcons();
  void updateHeaderData(Qt::Orientation orient, int first, int last);
  void insertHeaderSection(const QModelIndex &parent, int first, int last);
  void removeHeaderSection(const QModelIndex &parent, int first, int last);

private:
  pqCheckableHeaderViewInternal *Internal;
};

#endif
