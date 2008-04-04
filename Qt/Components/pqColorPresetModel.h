/*=========================================================================

   Program: ParaView
   Module:    pqColorPresetModel.h

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

/// \file pqColorPresetModel.h
/// \date 3/12/2007

#ifndef _pqColorPresetModel_h
#define _pqColorPresetModel_h


#include "pqComponentsExport.h"
#include <QAbstractItemModel>

class pqColorPresetModelInternal;
class pqColorMapModel;
class QString;


class PQCOMPONENTS_EXPORT pqColorPresetModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  pqColorPresetModel(QObject *parent=0);
  virtual ~pqColorPresetModel();

  /// \name QAbstractItemModel Methods
  //@{
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &index) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value,
      int role=Qt::EditRole);
  virtual QVariant headerData(int section, Qt::Orientation orient,
      int role=Qt::DisplayRole) const;
  //@}

  void addBuiltinColorMap(const pqColorMapModel &colorMap,
      const QString &name);
  void addColorMap(const pqColorMapModel &colorMap, const QString &name);
  void normalizeColorMap(int index);
  void removeColorMap(int index);
  const pqColorMapModel *getColorMap(int index) const;
  bool isModified() const {return this->Modified;}
  void setModified(bool modified) {this->Modified = modified;}

private:
  pqColorPresetModelInternal *Internal;
  bool Modified;
};

#endif
