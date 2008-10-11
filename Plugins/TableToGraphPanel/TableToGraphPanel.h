#include "pqObjectPanel.h"

#include "ui_TableToGraphPanel.h"

class TableToGraphPanel :
  public pqObjectPanel
{
  Q_OBJECT

  typedef pqObjectPanel Superclass;
  
public:
  TableToGraphPanel(pqProxy* proxy, QWidget* p);

private slots:
  virtual void accept();
  virtual void reset();

private:
  Ui::TableToGraphPanel Widgets;
};

