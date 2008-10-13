#include "pqObjectPanel.h"

#include "ui_GraphLayoutFilterPanel.h"

class GraphLayoutFilterPanel :
  public pqObjectPanel
{
  Q_OBJECT

  typedef pqObjectPanel Superclass;
  
public:
  GraphLayoutFilterPanel(pqProxy* proxy, QWidget* p);

private slots:
  virtual void accept();
  virtual void reset();

private:
  Ui::GraphLayoutFilterPanel Widgets;
};

