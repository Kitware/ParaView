#include "MyToolBar.h"

#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QStyle>

MyToolBar::MyToolBar(const QString& title, QWidget* parentW)
  : Superclass(title, parentW)
{
  this->constructor();
}

MyToolBar::MyToolBar(QWidget* parentW)
  : Superclass(parentW)
{
  this->setWindowTitle("My Toolbar (Examples)");
  this->constructor();
}

MyToolBar::~MyToolBar()
{
}

void MyToolBar::constructor()
{
  this->addWidget(new QLabel("Custom Toolbar", this));
  this->addAction(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation), "My Action",
    []() { QMessageBox::information(nullptr, "MyAction", "MyAction was invoked\n"); });
}
