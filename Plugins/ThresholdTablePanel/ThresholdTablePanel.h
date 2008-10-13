#include "pqNamedObjectPanel.h"

#include "ui_ThresholdTablePanel.h"

class pqDoubleRangeWidget;

class ThresholdTablePanel :
  public pqNamedObjectPanel
{
  Q_OBJECT

  typedef pqNamedObjectPanel Superclass;
  
public:
  ThresholdTablePanel(pqProxy* proxy, QWidget* p);

protected slots:
  void lowerChanged(double);
  void upperChanged(double);
  void variableChanged();

private slots:
  virtual void accept();
  virtual void reset();

protected:
  pqDoubleRangeWidget* Lower;
  pqDoubleRangeWidget* Upper;

private:
  Ui::ThresholdTablePanel Widgets;
};

