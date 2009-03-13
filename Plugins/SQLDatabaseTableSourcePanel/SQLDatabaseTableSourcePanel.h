#include "pqObjectPanel.h"

class QModelIndex;

class SQLDatabaseTableSourcePanel :
  public pqObjectPanel
{
  Q_OBJECT

  typedef pqObjectPanel Superclass;
  
public:
  SQLDatabaseTableSourcePanel(pqProxy* proxy, QWidget* p);
  ~SQLDatabaseTableSourcePanel();

private slots:
  virtual void accept();
  virtual void reset();

  void onDatabaseTypeChanged(const QString &databaseType);
  void slotDatabaseInfo();
  void slotTableInfo(const QModelIndex&);
  
protected:
  void loadDefaultSettings();
  void saveDefaultSettings();

  void loadInitialState();

private:
  class implementation;
  implementation* const Implementation;
};

