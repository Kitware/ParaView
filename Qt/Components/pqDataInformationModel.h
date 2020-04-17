/*=========================================================================

   Program: ParaView
   Module:    pqDataInformationModel.h

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
#ifndef pqDataInformationModel_h
#define pqDataInformationModel_h

#include "pqComponentsModule.h"
#include <QAbstractTableModel>

class pqDataInformationModelInternal;
class pqOutputPort;
class pqPipelineSource;
class pqView;

class PQCOMPONENTS_EXPORT pqDataInformationModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  pqDataInformationModel(QObject* _parent = NULL);
  ~pqDataInformationModel() override;

  /**
  * QAbstractTableModel API.
  * Returns the number of rows under the given parent.
  */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * QAbstractTableModel API.
  * Returns the number of columns for the given parent.
  */
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * QAbstractTableModel API.
  * Returns the data stored under the given role for the item referred
  * to by the index.
  */
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
  * QAbstractTableModel API.
  * Returns the data for the given role and section in the header with the
  * specified orientation.
  */
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  /**
  * Given a pqOutputPort, get the index for it, if present in this model,
  * otherwise returns invalid index.
  */
  QModelIndex getIndexFor(pqOutputPort* item) const;

  /**
  * Given a valid index, returns the pqOutputPort item corresponding
  * to it.
  */
  pqOutputPort* getItemFor(const QModelIndex& index) const;

  /**
  * Method needed for copy/past cell editor
  */
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

public Q_SLOTS:
  /**
  * Called when a new source/filter is registered.
  */
  void addSource(pqPipelineSource* source);

  /**
  * Called when a new source/filter is unregistered.
  */
  void removeSource(pqPipelineSource* source);

  /**
  * Called when the active view changes. Need to correctly show the geometry
  * size for the source.
  */
  void setActiveView(pqView* view);

private Q_SLOTS:
  /**
  * Called after the associated algorithm executes.
  */
  void dataUpdated(pqPipelineSource* changedSource);

  /**
  * Called at end of every render since geometry sizes may have changed.
  */
  void refreshGeometrySizes();

private:
  pqDataInformationModelInternal* Internal;

  enum ColumnType
  {
    Name = 0,
    DataType,
    CellCount,
    PointCount,
    MemorySize,
    GeometrySize,
    Bounds,
    TimeSpan,
    Max_Columns
  };
};

#endif
