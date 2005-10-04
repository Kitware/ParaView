#ifndef _pqServerFileBrowser_h
#define _pqServerFileBrowser_h

#include "pqServerFileBrowser.ui.h"

class pqServer;

class pqServerFileBrowser :
  public QDialog
{
  Q_OBJECT

public:
  pqServerFileBrowser(pqServer& Server, QWidget* Parent = 0, const char* const Name = 0);

signals:
  void fileSelected(const QString&);
  
private:
  ~pqServerFileBrowser() {};
  
  Ui::pqServerFileBrowser ui;
 
private slots:
  void onFileSelected(const QString&);
  void accept();
  void reject();
};

#endif // !_pqServerFileBrowser_h

