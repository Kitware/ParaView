#ifndef _pqServerFileBrowser_h
#define _pqServerFileBrowser_h

#include "pqServerFileBrowserBase.ui.h"

class pqServerFileBrowser :
  public QDialog
{
  Q_OBJECT

public:
  pqServerFileBrowser();
  
public slots:
  void onFileSelected(const QString&);
  
private:
  Ui::pqServerFileBrowserBase ui;
};

#endif // !_pqServerFileBrowser_h

