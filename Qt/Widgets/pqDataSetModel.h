

#ifndef _pqDataSetModel_h
#define _pqDataSetModel_h

#include "QtWidgetsExport.h"
#include <QAbstractTableModel>
class vtkDataSet;

/// provide a QAbstractTableModel for a vtkDataSet's cell scalars
/// \ todo fix this class to watch for changes in the pipeline and update the view accordingly
class QTWIDGETS_EXPORT pqDataSetModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  pqDataSetModel(QObject* p);
  ~pqDataSetModel();

  /// return the number of rows (number of cells in dataset)
  int rowCount(const QModelIndex& p = QModelIndex()) const;
  /// return number of columns (number of cell data arrays)
  int columnCount(const QModelIndex& p = QModelIndex()) const;
  /// return data for a position in the table
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  /// return the column headers (names of cell data arrays)
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  /// set the vtkDataSet to use
  void setDataSet(vtkDataSet* ds);
  /// get the vtkDataSet in use
  vtkDataSet* dataSet() const;

private:
  vtkDataSet* DataSet;

};

#endif //_pqDataSetModel_h

