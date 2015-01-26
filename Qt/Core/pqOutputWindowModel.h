/*=========================================================================

   Program: ParaView
   Module:    pqOutputWindow.h

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

#ifndef _pqOutputWindowModel_h
#define _pqOutputWindowModel_h

#include "pqCoreModule.h"
#include <QAbstractTableModel>

class QTableView;

struct MessageT
{
  MessageT(int type, int count, const QString& location, 
           const QString& message) :
    Type(type), Count(count), Location(location), Message(message)
  {
  }
  int Type;         // pqOutputWindow::MessageType
  int Count;
  QString Location;
  QString Message;
};

/// This is a model for the pqOutputWindow table that shows collated and
/// abbreviated messages.
class PQCORE_EXPORT pqOutputWindowModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  pqOutputWindowModel(QObject *parent, const QList<MessageT>& messages);
  ~pqOutputWindowModel();
  int rowCount(const QModelIndex &parent = QModelIndex()) const ;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  virtual Qt::ItemFlags flags(const QModelIndex & index) const;
  virtual bool setData(const QModelIndex & index, const QVariant & value, 
                       int role = Qt::EditRole);
  
  void setView(QTableView* view);

  /// Appends the last message to the table
  void appendLastRow();
  /// clears the table
  void clear();
  /// Shows in the table only messages that match the 'show' array.
  /// 'show' tells us if a pqOutputArray::MessageType element should be shown 
  /// or not
  void ShowMessages(bool* show);
  void expandRow(int r);
  void contractRow(int r);

private:
  const QList<MessageT>& Messages; 
  QList<int> Rows;  // element is index in Messages,
                    // when an element is expanded, the index is duplicated
  QTableView* View;
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif // !_pqOutputWindowModel_h
