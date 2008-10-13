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

  // Description:
  // Given a graph+selection, this extracts the selected graph
  // out and creates a new graph source from the subgraph.
  // (i.e., vtkExtractSelectedGraph).
  void createGraphSourceFromGraphSelection();
};
#endif

