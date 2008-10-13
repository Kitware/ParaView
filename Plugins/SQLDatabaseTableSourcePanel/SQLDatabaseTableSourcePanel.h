#include "pqObjectPanel.h"

#include "ui_SQLDatabaseTableSourcePanel.h"

class SQLDatabaseTableSourcePanel :
  public pqObjectPanel
{
  Q_OBJECT

  typedef pqObjectPanel Superclass;
  
public:
  SQLDatabaseTableSourcePanel(pqProxy* proxy, QWidget* p);

private slots:
  virtual void accept();
  virtual void reset();

  void onDatabaseTypeChanged(const QString &databaseType);
  
protected:
  void loadDefaultSettings();
  void saveDefaultSettings();

  void loadInitialState();

private:
  Ui::SQLDatabaseTableSourcePanel Widgets;
};

