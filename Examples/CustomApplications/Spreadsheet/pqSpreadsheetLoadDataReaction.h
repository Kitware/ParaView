#ifndef pqSpreadsheetLoadDataReaction_h
#define pqSpreadsheetLoadDataReaction_h

#include <pqLoadDataReaction.h>

class pqSpreadsheetLoadDataReaction : public pqLoadDataReaction
{
  Q_OBJECT
  typedef pqLoadDataReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqSpreadsheetLoadDataReaction(QAction* parent);

protected:
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqSpreadsheetLoadDataReaction)
};

#endif
