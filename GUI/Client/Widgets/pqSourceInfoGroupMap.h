
/// \file pqSourceInfoGroupMap.h
/// \date 5/31/2006

#ifndef _pqSourceInfoGroupMap_h
#define _pqSourceInfoGroupMap_h


#include "pqWidgetsExport.h"
#include <QObject>

class pqSourceInfoGroupMapItem;
class pqSourceInfoModel;
class QString;
class QStringList;
class vtkPVXMLElement;


class PQWIDGETS_EXPORT pqSourceInfoGroupMap : public QObject
{
  Q_OBJECT

public:
  pqSourceInfoGroupMap(QObject *parent=0);
  ~pqSourceInfoGroupMap();

  void loadSourceInfo(vtkPVXMLElement *root);
  void saveSourceInfo(vtkPVXMLElement *root);

  void addGroup(const QString &group);
  void removeGroup(const QString &group);

  void addSource(const QString &name, const QString &group);
  void removeSource(const QString &name, const QString &group);

  /// \brief
  ///   Adds all the source groups from the map to the model.
  /// \param model The model to initialize using the map.
  void initializeModel(pqSourceInfoModel *model) const;

signals:
  void clearingData();

  void groupAdded(const QString &group);
  void removingGroup(const QString &group);

  void sourceAdded(const QString &name, const QString &group);
  void removingSource(const QString &name, const QString &group);

private:
  pqSourceInfoGroupMapItem *getNextItem(pqSourceInfoGroupMapItem *item) const;
  void getGroupPath(pqSourceInfoGroupMapItem *item, QString &group) const;
  pqSourceInfoGroupMapItem *getGroupItemFor(const QString &group) const;

  pqSourceInfoGroupMapItem *getChildItem(pqSourceInfoGroupMapItem *item,
      const QString &name) const;
  bool isNameInItem(const QString &name, pqSourceInfoGroupMapItem *item) const;

private:
  pqSourceInfoGroupMapItem *Root;
};

#endif
