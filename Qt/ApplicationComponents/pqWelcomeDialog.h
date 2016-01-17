#ifndef PQWELCOMEDIALOG_H
#define PQWELCOMEDIALOG_H

#include <QDialog>
#include "pqComponentsModule.h"

namespace Ui {
class pqWelcomeDialog;
}

/// This class provides a welcome dialog screen that you see in many applications.
/// The intent is to provide an on-ramp with a lower learning curve than the blank
/// ParaView screen.
class PQCOMPONENTS_EXPORT pqWelcomeDialog : public QDialog
{
 Q_OBJECT
 typedef QDialog Superclass;

public:
  explicit pqWelcomeDialog(QWidget *parent = 0);
  ~pqWelcomeDialog();

protected slots:
  /// Handle ParaView Guide button
  void onParaViewGuideButtonClicked(bool);

  /// Handle tutorials button
  void onParaViewTutorialsButtonClicked(bool);

  /// Handle help button
  void onHelpButtonClicked(bool);

  /// React to checkbox events
  void onDoNotShowAgainStateChanged(int);

private:
    Ui::pqWelcomeDialog *ui;
};

#endif // PQWELCOMEDIALOG_H
