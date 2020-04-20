/*=========================================================================

   Program: ParaView
   Module:    pqSILModel.h

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
#ifndef pqSILModel_h
#define pqSILModel_h

#include <QAbstractItemModel>
#include <QSet>
#include <QVector>
#include <set>

#include "pqComponentsModule.h"
#include "vtkObject.h"
#include "vtkSmartPointer.h"

class vtkGraph;
class vtkSMSILDomain;
class vtkSMSILModel;
/**
 * @class pqSILModel
 * @brief QAbstractItemModel for legacy SIL (vtkGraph-based SIL)
 *
 * pqSILModel is QAbstractItemModel implementation for legacy SIL.
 *
 * @section pqSILModelLegayWarning Legacy Warning
 *
 * While not deprecated, this class exists to support readers that use legacy
 * representation for SIL which used a `vtkGraph` to represent the SIL. It is
 * recommended that newer code uses vtkSubsetInclusionLattice (or subclass) to
 * represent the SIL. In that case, you should use
 * `pqSubsetInclusionLatticeTreeModel` instead.
 */
class PQCOMPONENTS_EXPORT pqSILModel : public QAbstractItemModel
{
  Q_OBJECT
  typedef QAbstractItemModel Superclass;

public:
  pqSILModel(QObject* parent = 0);
  ~pqSILModel() override;

  /**
  * \name QAbstractItemModel Methods
  */
  //@{
  /**
  * \brief
  *   Gets the number of rows for a given index.
  * \param parent The parent index.
  * \return
  *   The number of rows for the given index.
  */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets the number of columns for a given index.
  * \param parent The parent index.
  * \return
  *   The number of columns for the given index.
  */
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets whether or not the given index has child items.
  * \param parent The parent index.
  * \return
  *   True if the given index has child items.
  */
  bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets a model index for a given location.
  * \param row The row number.
  * \param column The column number.
  * \param parent The parent index.
  * \return
  *   A model index for the given location.
  */
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

  /**
  * \brief
  *   Gets the parent for a given index.
  * \param index The model index.
  * \return
  *   A model index for the parent of the given index.
  */
  QModelIndex parent(const QModelIndex& index) const override;

  /**
  * \brief
  *   Gets the data for a given model index.
  * \param index The model index.
  * \param role The role to get data for.
  * \return
  *   The data for the given model index.
  */
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
  * \brief
  *   Gets the flags for a given model index.
  *
  * The flags for an item indicate if it is enabled, editable, etc.
  *
  * \param index The model index.
  * \return
  *   The flags for the given model index.
  */
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  /**
  * \brief
  *  Sets the role data for the item at index to value. Returns
  *  true if successful; otherwise returns false.
  */
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  //@}

  /**
  * Returns the QModelIndex for the hierarchy with the given name, if present.
  * If the hierarchy is not present an index referring to an empty tree will
  * be returned.
  */
  QModelIndex hierarchyIndex(const QString& hierarchyName) const;

  QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override
  {
    if (role == Qt::DisplayRole)
    {
      return "Selections";
    }
    return QVariant();
  }

  /**
  * API to get/set status of a given hierarchy.
  */
  QList<QVariant> status(const QString& hierarchyName) const;
  void setStatus(const QString& hierarchyName, const QList<QVariant>& values);

  void setSILDomain(vtkSMSILDomain* domain);

  void domainModified();

  /**
  * Returns the model index for a vertex.
  */
  QModelIndex makeIndex(vtkIdType vertexid) const;

  /**
  * Returns the vertex id for a vertex with the given name in the SIL. Returns
  * -1 if no such vertex could be found.
  */
  vtkIdType findVertex(const char* name) const;

Q_SIGNALS:
  void checkStatusChanged();

public Q_SLOTS:
  /**
  * Used to reset the model based on the sil.
  */
  void update();

protected:
  /**
  * Called every time vtkSMSILModel tells us that the check state has changed.
  * We fire the dataChanged() event so that the view updates.
  */
  void checkStateUpdated(vtkObject* caller, unsigned long eventid, void* calldata);

  /**
  * Returns if the given vertex id refers to a leaf node.
  */
  bool isLeaf(vtkIdType vertexid) const;

  /**
  * Returns the parent vertex id for the given vertex. It's an error to call
  * this method for the root vertex id i.e. 0.
  */
  vtkIdType parent(vtkIdType vertexid) const;

  /**
  * Returns the number of children for the given vertex.
  */
  int childrenCount(vtkIdType vertexid) const;

  /**
  * Used to initialize the HierarchyVertexIds list with the leaf node ids for
  * each of the hierarchies.
  */
  void collectLeaves(vtkIdType vertexid, std::set<vtkIdType>& list);

  vtkSMSILModel* SILModel;

  /**
  * Cache used by makeIndex() to avoid iterating over the edges each time.
  */
  QMap<vtkIdType, QModelIndex>* ModelIndexCache;

  QMap<QString, QModelIndex> Hierarchies;

  /**
  * This map keeps a list of vertex ids that refer to the leaves in the
  * hierarchy.
  */
  QMap<QString, std::set<vtkIdType> > HierarchyVertexIds;
  vtkSmartPointer<vtkSMSILDomain> SILDomain;
  unsigned long SILDomainObserverId;

private:
  Q_DISABLE_COPY(pqSILModel)
};

#endif
