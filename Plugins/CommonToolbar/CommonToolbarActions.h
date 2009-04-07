#ifndef _CommonToolbarActions_h
#define _CommonToolbarActions_h

#include "ToolbarActions.h"

class CommonToolbarActions : public ToolbarActions
{
  Q_OBJECT
public:
  CommonToolbarActions(QObject* p);
  ~CommonToolbarActions();

private slots:

  // Description:
  // Updates the enable state of the toolbar actions when the 
  // pipline selection changes.
  void updateEnableState();

};
#endif

