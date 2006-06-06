
/// \file pqSourceInfoModel.h
/// \date 5/26/2006

#ifndef _pqSourceInfoModel_h
#define _pqSourceInfoModel_h


#include "pqWidgetsExport.h"
#include <QAbstractItemModel>

class pqSourceInfoModelItem;
class QString;
class QStringList;


class PQWIDGETS_EXPORT pqSourceInfoModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  pqSourceInfoModel(const QStringList &sources, QObject *parent=0);
  virtual ~pqSourceInfoModel();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the number of columns for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of columns for the given index.
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets whether or not the given index has child items.
  /// \param parent The parent index.
  /// \return
  ///   True if the given index has child items.
  virtual bool hasChildren(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets a model index for a given location.
  /// \param row The row number.
  /// \param column The column number.
  /// \param parent The parent index.
  /// \return
  ///   A model index for the given location.
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the parent for a given index.
  /// \param index The model index.
  /// \return
  ///   A model index for the parent of the given index.
  virtual QModelIndex parent(const QModelIndex &index) const;

  /// \brief
  ///   Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return
  ///   The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

  /// \brief
  ///   Gets the flags for a given model index.
  ///
  /// The flags for an item indicate if it is enabled, editable, etc.
  ///
  /// \param index The model index.
  /// \return
  ///   The flags for the given model index.
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  //@}

  bool isSource(const QModelIndex &index) const;

public slots:
  /// \name Modification Methods
  //@{
  void clearGroups();

  void addGroup(const QString &group);
  void removeGroup(const QString &group);

  void addSource(const QString &name, const QString &group);
  void removeSource(const QString &name, const QString &group);
  //@}

private:
  pqSourceInfoModelItem *getItemFor(const QModelIndex &index) const;
  pqSourceInfoModelItem *getGroupItemFor(const QString &group) const;

  pqSourceInfoModelItem *getChildItem(pqSourceInfoModelItem *item,
      const QString &name) const;
  bool isNameInItem(const QString &name, pqSourceInfoModelItem *item) const;

  void addChildItem(pqSourceInfoModelItem *item);
  void removeChildItem(pqSourceInfoModelItem *item);

private:
  pqSourceInfoModelItem *Root;
};

#endif
