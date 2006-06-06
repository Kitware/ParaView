
/// \file pqAddSourceDialog.h
/// \date 5/26/2006

#ifndef _pqAddSourceDialog_h
#define _pqAddSourceDialog_h


#include "pqWidgetsExport.h"
#include <QDialog>

class pqAddSourceDialogForm;
class pqSourceInfoModel;
class QAbstractItemModel;
class QAbstractListModel;
class QModelIndex;
class QString;
class QStringList;


class PQWIDGETS_EXPORT pqAddSourceDialog : public QDialog
{
  Q_OBJECT

public:
  pqAddSourceDialog(QWidget *parent=0);
  virtual ~pqAddSourceDialog();

  void setSourceList(QAbstractItemModel *sources);
  void setHistoryList(QAbstractListModel *history);

  void getPath(QString &path);
  void setPath(const QString &path);
  void setSource(const QString &name);

public slots:
  void navigateBack();
  void navigateUp();
  void addFolder();
  void addFavorite();

private slots:
  void changeRoot(const QModelIndex &index);
  void changeRoot(int index);
  void updateFromSources(const QModelIndex &current,
      const QModelIndex &previous);
  void updateFromHistory(const QModelIndex &current,
      const QModelIndex &previous);

private:
  void getPath(const QModelIndex &index, QStringList &path);

private:
  pqAddSourceDialogForm *Form;
  QAbstractItemModel *Sources;
  QAbstractListModel *History;
  pqSourceInfoModel *SourceInfo;
};

#endif
