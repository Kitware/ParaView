#include "ExampleDockPanel.h"
#include "ui_ExampleDockPanel.h"

void ExampleDockPanel::constructor()
{
  this->setWindowTitle("Example Dock Panel");
  QWidget* t_widget = new QWidget(this);
  Ui::ExampleDockPanel ui;
  ui.setupUi(t_widget);
  this->setWidget(t_widget);
}
