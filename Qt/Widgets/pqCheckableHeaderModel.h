/*=========================================================================

   Program: ParaView
   Module:    pqCheckableHeaderModel.h

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

/// \file pqCheckableHeaderModel.h
/// \date 8/17/2007

#ifndef _pqCheckableHeaderModel_h
#define _pqCheckableHeaderModel_h


#include "QtWidgetsExport.h"
#include <QAbstractItemModel>

class pqCheckableHeaderModelInternal;
class pqCheckableHeaderModelItem;


class QTWIDGETS_EXPORT pqCheckableHeaderModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  pqCheckableHeaderModel(QObject *parent=0);
  virtual ~pqCheckableHeaderModel();

  virtual QVariant headerData(int section, Qt::Orientation orient,
      int role=Qt::DisplayRole) const;
  virtual bool setHeaderData(int section, Qt::Orientation orient,
      const QVariant &value, int role=Qt::EditRole);

  bool isCheckable(int section, Qt::Orientation orient) const;
  void setCheckable(int section, Qt::Orientation orient, bool checkable);

  int getCheckState(int section, Qt::Orientation orient) const;
  bool setCheckState(int section, Qt::Orientation orient, int state);

  void updateCheckState(int section, Qt::Orientation orient);

  int getNumberOfHeaderSections(Qt::Orientation orient) const;
  void clearHeaderSections(Qt::Orientation orient);
  void insertHeaderSections(Qt::Orientation orient, int first, int last);
  void removeHeaderSections(Qt::Orientation orient, int first, int last);

  void beginMultiStateChange();
  void endMultipleStateChange();

public slots:
  void setIndexCheckState(Qt::Orientation orient, int first, int last);

private:
  pqCheckableHeaderModelItem *getItem(int section,
      Qt::Orientation orient) const;

private:
  pqCheckableHeaderModelInternal *Internal;
};

#endif
