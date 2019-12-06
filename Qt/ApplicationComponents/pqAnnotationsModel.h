/*=========================================================================

   Program: ParaView
   Module:  pqAnnotationsModel.h

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
#ifndef pqAnnotationsModel_h
#define pqAnnotationsModel_h

#include "pqApplicationComponentsModule.h"
#include <QAbstractTableModel>

#include <QColor>
#include <QIcon>

#include <vector>

#include "vtkSmartPointer.h"

class QModelIndex;

class vtkSMStringListDomain;

//-----------------------------------------------------------------------------
// QAbstractTableModel subclass for keeping track of the annotations and their properties (color,
// visibilities)
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnnotationsModel : public QAbstractTableModel
{
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

  //@{
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
  //@}

  /**
   * Return the number of columns.
   */
  int columnCount() const { return NUMBER_OF_COLUMNS; }

  void setVisibilityDomain(vtkSMStringListDomain* domain);

  //@{
  /**
   * Add/remove annotations.
   */
  QModelIndex addAnnotation(const QModelIndex& after = QModelIndex());
  QModelIndex removeAnnotations(const QModelIndexList& toRemove = QModelIndexList());
  void removeAllAnnotations();
  //@}

  //@{
  /**
   * Set/Get the value-annotation pairs.
   */
  void setAnnotations(const std::vector<std::pair<QString, QString> >& newAnnotations);
  std::vector<std::pair<QString, QString> > annotations() const;
  //@}

  //@{
  /**
   * Set/Get the visibilities.
   */
  void setVisibilities(const std::vector<std::pair<QString, int> >& newVisibilities);
  std::vector<std::pair<QString, int> > visibilities() const;
  //@}

  //@{
  /**
   * Set/Get the colors.
   */
  void setIndexedColors(const std::vector<QColor>& newColors);
  std::vector<QColor> indexedColors() const;
  //@}

  bool hasColors() const;

  //@{
  /**
   * Set/Get IndexedOpacities.
   */
  void setIndexedOpacities(const std::vector<double>& newOpacities);
  std::vector<double> indexedOpacities() const;
  //@}

  //@{
  /**
   * Set/Get Global opacity.
   */
  void setGlobalOpacity(double opacity);
  double globalOpacity() const { return this->GlobalOpacity; }
  //@}

  void setSelectedOpacity(QList<int> rows, double opacity);

  //@{
  /**
   * Set/Get SupportsReorder.
   */
  void setSupportsReorder(bool reorder);
  bool supportsReorder() const;
  //@}

  /**
   * Reorders the list of annotations, following the indexes given by newOrder.
   */
  void reorder(std::vector<int> newOrder);

protected:
  QIcon MissingColorIcon;
  double GlobalOpacity;
  vtkSmartPointer<vtkSMStringListDomain> VisibilityDomain;
  bool SupportsReorder;

private:
  Q_DISABLE_COPY(pqAnnotationsModel)

  class pqInternals;
  pqInternals* Internals;
};

#endif
