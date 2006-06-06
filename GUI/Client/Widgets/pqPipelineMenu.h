
/// \file pqPipelineMenu.h
/// \date 6/5/2006

#ifndef _pqPipelineMenu_h
#define _pqPipelineMenu_h


#include "pqWidgetsExport.h"
#include <QObject>

class pqPipelineMenuInternal;
class pqSourceInfoGroupMap;
class pqSourceInfoModel;
class QAction;
class QMenu;
class QMenuBar;
class QStringList;
class vtkPVXMLElement;
class vtkSMProxy;


class PQWIDGETS_EXPORT pqPipelineMenu : public QObject
{
  Q_OBJECT

public:
  enum ActionName
    {
    InvalidAction = -1,
    AddSourceAction = 0,
    AddFilterAction,
    AddBundleAction,
    LastAction = AddBundleAction
    };

public:
  pqPipelineMenu(QObject *parent=0);
  virtual ~pqPipelineMenu();

  void loadSourceInfo(vtkPVXMLElement *root);
  void loadFilterInfo(vtkPVXMLElement *root);
  void loadBundleInfo(vtkPVXMLElement *root);

  pqSourceInfoModel *getFilterModel();

  void addActionsToMenuBar(QMenuBar *menubar) const;
  void addActionsToMenu(QMenu *menu) const;
  QAction *getMenuAction(ActionName name) const;

public slots:
  void addSource();
  void addFilter();
  void addBundle();

private:
  void setupConnections(pqSourceInfoModel *model, pqSourceInfoGroupMap *map);
  void getAllowedSources(pqSourceInfoModel *model, vtkSMProxy *input,
      QStringList &list);

private:
  pqPipelineMenuInternal *Internal;
  QAction **MenuList;
};

#endif
