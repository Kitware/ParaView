#ifndef _StatisticsToolbarActions_h
#define _StatisticsToolbarActions_h

#include "ToolbarActions.h"

class StatisticsToolbarActions : public ToolbarActions
{
  Q_OBJECT
public:
  StatisticsToolbarActions(QObject* p);
  ~StatisticsToolbarActions();

private slots:

  // Description:
  // Updates the enable state of the toolbar actions when the 
  // pipline selection changes.
  void updateEnableState();

};
#endif

