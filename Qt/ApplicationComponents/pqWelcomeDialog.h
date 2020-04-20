#ifndef PQWELCOMEDIALOG_H
#define PQWELCOMEDIALOG_H

#include "pqApplicationComponentsModule.h"
#include <QDialog>

namespace Ui
{
class pqWelcomeDialog;
}

/**
* This class provides a welcome dialog screen that you see in many applications.
* The intent is to provide an on-ramp with a lower learning curve than the blank
* ParaView screen.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqWelcomeDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  explicit pqWelcomeDialog(QWidget* parent = 0);
  ~pqWelcomeDialog() override;

public Q_SLOTS:
  virtual void onGettingStartedGuideClicked();

  virtual void onExampleVisualizationsClicked();

protected Q_SLOTS:
  /**
  * React to checkbox events
  */
  void onDoNotShowAgainStateChanged(int);

private:
  Ui::pqWelcomeDialog* ui;
};

#endif // PQWELCOMEDIALOG_H
