

#ifndef _pqCompoundProxyWizard_h
#define _pqCompoundProxyWizard_h

#include <QtGui/QDialog>
#include "ui_pqCompoundProxyWizard.h"
#include "QtWidgetsExport.h"

class pqServer;

class QTWIDGETS_EXPORT pqCompoundProxyWizard : public QDialog, 
                                               public Ui::pqCompoundProxyWizard
{
  Q_OBJECT
public:
  pqCompoundProxyWizard(pqServer* s, QWidget* p = 0, Qt::WFlags f = 0);
  ~pqCompoundProxyWizard();

public slots:
  void onLoad();
  void onLoad(const QStringList& files);
  void onRemove();
  void addToList(const QString& filename, const QString& proxy);

signals:
  void newCompoundProxy(const QString& filename, const QString& proxy);

private:
  pqServer* Server;

};

#endif // _pqCompoundProxyWizard_h

