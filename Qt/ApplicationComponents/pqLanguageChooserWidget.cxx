// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLanguageChooserWidget.h"

#include "pqServer.h"
#include "vtkPVFileInformation.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QObject>
#include <QProcessEnvironment>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqLanguageChooserWidget::pqLanguageChooserWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  this->ComboBox = new QComboBox(this);
  this->ComboBox->setObjectName("LanguageComboBox");
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(this->ComboBox);

  QList<QDir> paths;
  QProcessEnvironment options = QProcessEnvironment::systemEnvironment();
  if (options.contains("PV_TRANSLATIONS_DIR"))
  {
    for (QString path : options.value("PV_TRANSLATIONS_DIR").split(":"))
    {
      paths.append(QDir(path));
    }
  }
  QString translationsPath(vtkPVFileInformation::GetParaViewTranslationsDirectory().c_str());
  /* PV_TRANSLATIONS_DIR `override` translationsPath's qm files,
    thus translationsPath has to be added lastly */
  paths.append(translationsPath);

  /* en is the language used in source string, therefore it does not need a translation file
    and is always available */
  this->ComboBox->addItem(QLocale("en").nativeLanguageName(), QVariant("en"));
  for (QDir directory : paths)
  {
    for (QFileInfo fileInfo : directory.entryInfoList(QDir::Files))
    {
      QString localeString =
        fileInfo.completeBaseName().mid(fileInfo.completeBaseName().indexOf("_") + 1);
      QLocale locale = QLocale(localeString);
      if (localeString.contains("_") && fileInfo.suffix() == "qm" && locale != QLocale::C &&
        this->ComboBox->findData(localeString) == -1)
      {
        this->ComboBox->addItem(locale.nativeLanguageName(), QVariant(localeString));
      }
    }
  }

  if (options.contains("PV_TRANSLATIONS_LOCALE"))
  {
    QString value = options.value("PV_TRANSLATIONS_LOCALE");
    this->ComboBox->addItem(
      QLocale(value).nativeLanguageName() + (this->ComboBox->findData(value) == -1 ? " (?)" : ""),
      QVariant(value));
    this->ComboBox->setCurrentIndex(this->ComboBox->count() - 1);
    this->ComboBox->setEnabled(false);
    QLabel* overridenWarning = new QLabel(
      tr("Setting overriden by environment variable PV_TRANSLATIONS_LOCALE"), parentObject = this);
    vbox->addWidget(overridenWarning);
    return;
  }

  this->ComboBox->model()->sort(0, Qt::AscendingOrder);
  this->connect(this->ComboBox, SIGNAL(currentIndexChanged(int)), SIGNAL(valueChanged()));
  this->addPropertyLink(this, "value", SIGNAL(valueChanged()), smproperty);
}

//-----------------------------------------------------------------------------
pqLanguageChooserWidget::~pqLanguageChooserWidget() = default;

//-----------------------------------------------------------------------------
QString pqLanguageChooserWidget::value() const
{
  return this->ComboBox->itemData(this->ComboBox->currentIndex()).toString();
}

//-----------------------------------------------------------------------------
void pqLanguageChooserWidget::setValue(QString& val)
{
  int index = this->ComboBox->findData(val);
  if (index == -1)
  {
    // add the value being specified to the combo-box.
    index = this->ComboBox->count();
    this->ComboBox->addItem(QLocale(val).nativeLanguageName() + " (?)", val);
  }
  this->ComboBox->setCurrentIndex(index);
  this->ComboBox->model()->sort(0, Qt::AscendingOrder);
}
