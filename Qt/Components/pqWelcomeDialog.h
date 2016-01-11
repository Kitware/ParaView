#ifndef PQWELCOMEDIALOG_H
#define PQWELCOMEDIALOG_H

#include <QDialog>
#include "pqComponentsModule.h"

namespace Ui {
class pqWelcomeDialog;
}

class PQCOMPONENTS_EXPORT pqWelcomeDialog : public QDialog
{
 Q_OBJECT
 typedef QDialog Superclass;

public:
  explicit pqWelcomeDialog(QWidget *parent = 0);
  ~pqWelcomeDialog();

private:
    Ui::pqWelcomeDialog *ui;
};

#endif // PQWELCOMEDIALOG_H
