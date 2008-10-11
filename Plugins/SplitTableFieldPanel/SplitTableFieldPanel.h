#include "pqObjectPanel.h"

#include "ui_SplitTableFieldPanel.h"

class SplitTableFieldPanel :
  public pqObjectPanel
{
  Q_OBJECT

  typedef pqObjectPanel Superclass;
  
public:
  SplitTableFieldPanel(pqProxy* proxy, QWidget* p);

private slots:
  virtual void accept();
  virtual void reset();

private:
  Ui::SplitTableFieldPanel Widgets;
};

