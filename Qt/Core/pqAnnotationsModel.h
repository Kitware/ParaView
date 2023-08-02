// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnnotationsModel_h
#define pqAnnotationsModel_h

#include "pqCoreModule.h"

#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_12_0
#include "vtkSmartPointer.h"

#include <QAbstractTableModel>
#include <QColor>
#include <QIcon>

#include <vector>

class QModelIndex;

class vtkSMStringListDomain;

//-----------------------------------------------------------------------------
// QAbstractTableModel subclass for keeping track of the annotations and their properties (color,
// visibilities)
class PQCORE_EXPORT pqAnnotationsModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;

public:
  pqAnnotationsModel(QObject* parentObject = nullptr);
  ~pqAnnotationsModel() override;

  enum ColumnRoles
  {
    VISIBILITY = 0,
    COLOR,
    OPACITY,
    VALUE,
    LABEL,
    NUMBER_OF_COLUMNS,
    COLOR_DATA = NUMBER_OF_COLUMNS,
    OPACITY_DATA
  };

  ///@{
  /**
   * Reimplements QAbstractTableModel
   */
  Qt::ItemFlags flags(const QModelIndex& idx) const override;
  int rowCount(const QModelIndex& prnt = QModelIndex()) const override;
  int columnCount(const QModelIndex& /*parent*/) const override;
  bool setData(const QModelIndex& idx, const QVariant& value, int role = Qt::EditRole) override;
  QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override;
  bool setHeaderData(
    int section, Qt::Orientation orientation, const QVariant& value, int role) override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  Qt::DropActions supportedDropActions() const override;
  QStringList mimeTypes() const override;
  QMimeData* mimeData(const QModelIndexList& indexes) const override;
  bool dropMimeData(const QMimeData* mime_data, Qt::DropAction action, int row, int column,
    const QModelIndex& parentIdx) override;
  ///@}

  /**
   * Return the number of columns.
   */
  int columnCount() const { return NUMBER_OF_COLUMNS; }

  void setVisibilityDomain(vtkSMStringListDomain* domain);

  ///@{
  /**
   * Add/remove annotations.
   */
  QModelIndex addAnnotation(const QModelIndex& after = QModelIndex());
  QModelIndex removeAnnotations(const QModelIndexList& toRemove = QModelIndexList());
  void removeAllAnnotations();
  ///@}

  ///@{
  /**
   * Set/Get the value-annotation pairs.
   * Emit dataChanged signal, unless quiet is true.
   */
  void setAnnotations(
    const std::vector<std::pair<QString, QString>>& newAnnotations, bool quiet = false);
  std::vector<std::pair<QString, QString>> annotations() const;
  ///@}

  ///@{
  /**
   * Set/Get the visibilities.
   */
  void setVisibilities(const std::vector<std::pair<QString, int>>& newVisibilities);
  std::vector<std::pair<QString, int>> visibilities() const;
  ///@}

  ///@{
  /**
   * Set/Get the colors.
   */
  void setIndexedColors(const std::vector<QColor>& newColors);
  std::vector<QColor> indexedColors() const;
  ///@}

  bool hasColors() const;

  ///@{
  /**
   * Set/Get IndexedOpacities.
   */
  void setIndexedOpacities(const std::vector<double>& newOpacities);
  std::vector<double> indexedOpacities() const;
  ///@}

  ///@{
  /**
   * Set/Get the global opacity value. Default is 1.0.
   * GlobalOpacity corresponds to a cached value only used to draw the
   * global opacity swatch. The opacity value of each items is modified using
   * the setHeaderData method. Note that setHeaderData can also modify the
   * GlobalOpacity value.
   */
  void setGlobalOpacity(double opacity) { this->GlobalOpacity = opacity; };
  double globalOpacity() const { return this->GlobalOpacity; }
  ///@}

  ///@{
  /**
   * Set the opacity for the given rows.
   */
  void setSelectedOpacity(QList<int> rows, double opacity);
  ///@}

  ///@{
  /**
   * Set/Get SupportsReorder. Default is false.
   */
  void setSupportsReorder(bool reorder);
  bool supportsReorder() const;
  ///@}

  /**
   * Reorders the list of annotations, following the indexes given by newOrder.
   */
  void reorder(std::vector<int> newOrder);

protected:
  PARAVIEW_DEPRECATED_IN_5_12_0("Unused protected member variable.")
  QIcon MissingColorIcon;
  double GlobalOpacity = 1.0;
  vtkSmartPointer<vtkSMStringListDomain> VisibilityDomain;
  bool SupportsReorder = false;

private:
  Q_DISABLE_COPY(pqAnnotationsModel)

  class pqInternals;
  pqInternals* Internals;
};

#endif
