#include "pqObjectPanel.h"

#include "ui_TableToSparseArrayPanel.h"

class TableToSparseArrayPanel :
  public pqObjectPanel
{
  Q_OBJECT

  typedef pqObjectPanel Superclass;
  
public:
  TableToSparseArrayPanel(pqProxy* proxy, QWidget* p);

private slots:
  virtual void accept();
  virtual void reset();

private:
  Ui::TableToSparseArrayPanel Widgets;
};

