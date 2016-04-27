#ifndef PQEXAMPLEVISUALIZATIONSDIALOG_H
#define PQEXAMPLEVISUALIZATIONSDIALOG_H

#include <QDialog>

#include "pqApplicationComponentsModule.h"

namespace Ui {
class pqExampleVisualizationsDialog;
}

class PQAPPLICATIONCOMPONENTS_EXPORT pqExampleVisualizationsDialog : public QDialog
{
 Q_OBJECT
 typedef QDialog Superclass;
public:
  explicit pqExampleVisualizationsDialog(QWidget* parent = 0);
  virtual ~pqExampleVisualizationsDialog();

protected slots:
  virtual void onButtonPressed();

private:
  Ui::pqExampleVisualizationsDialog* ui;

};

#endif // PQEXAMPLEVISUALIZATIONSDIALOG_H
