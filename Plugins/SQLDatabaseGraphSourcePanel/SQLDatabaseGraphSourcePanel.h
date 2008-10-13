#include "pqObjectPanel.h"

#include "ui_SQLDatabaseGraphSourcePanel.h"

class SQLDatabaseGraphSourcePanel :
  public pqObjectPanel
{
  Q_OBJECT

  typedef pqObjectPanel Superclass;
  
public:
  SQLDatabaseGraphSourcePanel(pqProxy* proxy, QWidget* p);

private slots:
  virtual void accept();
  virtual void reset();

  void onDatabaseTypeChanged(const QString &databaseType);
  
protected:
  void loadDefaultSettings();
  void saveDefaultSettings();

  void loadInitialState();

private:
  Ui::SQLDatabaseGraphSourcePanel Widgets;
};

