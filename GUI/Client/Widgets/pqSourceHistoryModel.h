
/// \file pqSourceHistoryModel.h
/// \date 5/26/2006

#ifndef _pqSourceHistoryModel_h
#define _pqSourceHistoryModel_h


#include "pqWidgetsExport.h"
#include <QAbstractListModel>

class pqSourceHistoryModelInternal;
class QStringList;


class PQWIDGETS_EXPORT pqSourceHistoryModel : public QAbstractListModel
{
public:
  pqSourceHistoryModel(QObject *parent=0);
  virtual ~pqSourceHistoryModel();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

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

  /// \name Index Mapping Methods
  //@{
  QString getFilterName(const QModelIndex &index) const;
  QModelIndex getIndexFor(const QString &filter) const;
  //@}

  /// \name History Methods
  //@{
  int getHistoryLimit() const {return this->Limit;}
  void setHistoryLimit(int limit);
  void getHistoryList(QStringList &list) const;
  void setHistoryList(const QStringList &list);
  void addRecentFilter(const QString &filter);
  //@}

private:
  pqSourceHistoryModelInternal *Internal;
  int Limit;
};

#endif
