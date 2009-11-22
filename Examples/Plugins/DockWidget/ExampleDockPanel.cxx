#include "ExampleDockPanel.h"
#include "ui_ExampleDockPanel.h"

void ExampleDockPanel::constructor()
{
  this->setWindowTitle("Example Dock Panel");
  QWidget* widget = new QWidget(this);
  Ui::ExampleDockPanel ui;
  ui.setupUi(widget);
  this->setWidget(widget);
}
