/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "pqSQTensorGlyph.h"

pqSQTensorGlyph::pqSQTensorGlyph(pqProxy *proxy, QWidget *p)
  : Superclass(proxy, p)
{
  // // List of all children
  // QObjectList list(this->children());
  // foreach(QObject *o, list)
  //   {
  //   std::cout << o->objectName().toStdString() << std::endl;
  //   }

  this->ColorGlyphsWidget = this->findChild<QCheckBox*>("ColorGlyphs");
  if (! this->ColorGlyphsWidget)
    {
    qWarning() << "Failed to locate ColorGlyphs widget.";
    return;
    }

  // Automatically disable "color by" dropdown box if ColorGlyphs is not set
  this->ColorModeWidget = this->findChild<QWidget*>("ColorMode");
  if (this->ColorModeWidget)
    QObject::connect(this->ColorGlyphsWidget, SIGNAL(toggled(bool)),
                     this->ColorModeWidget, SLOT(setEnabled(bool)));

  this->ColorModeLabel = this->findChild<QLabel*>("_labelForColorMode");
  if (this->ColorModeLabel)
    QObject::connect(this->ColorGlyphsWidget, SIGNAL(toggled(bool)),
                     this->ColorModeLabel, SLOT(setEnabled(bool)));

  this->ColorGlyphsWidget->toggle();
  this->ColorGlyphsWidget->toggle();


  this->LimitScalingByEigenvaluesWidget = this->findChild<QCheckBox*>("LimitScalingByEigenvalues");
  if (! this->LimitScalingByEigenvaluesWidget)
    {
    qWarning() << "Failed to locate LimitScalingByEigenvalues widget.";
    return;
    }

  // Automatically disable "MaxScaleFactor" textbox if LimitScaling is not set
  this->MaxScaleFactorWidget = this->findChild<QWidget*>("MaxScaleFactor");
  if (this->MaxScaleFactorWidget)
    QObject::connect(this->LimitScalingByEigenvaluesWidget, SIGNAL(toggled(bool)),
                     this->MaxScaleFactorWidget, SLOT(setEnabled(bool)));

  this->MaxScaleFactorLabel = this->findChild<QLabel*>("_labelForMaxScaleFactor");
  if (this->MaxScaleFactorLabel)
    QObject::connect(this->LimitScalingByEigenvaluesWidget, SIGNAL(toggled(bool)),
                     this->MaxScaleFactorLabel, SLOT(setEnabled(bool)));

  this->LimitScalingByEigenvaluesWidget->toggle();
  this->LimitScalingByEigenvaluesWidget->toggle();

  return;
}
