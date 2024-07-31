// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSearchBox.h"
#include "ui_pqSearchBox.h"

// Server Manager Includes.

// Qt Includes.
#include <QDebug>
#include <QKeyEvent>
#include <QSettings>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <cassert>

//-----------------------------------------------------------------------------
// Internals
class pqSearchBox::pqInternals : public Ui::SearchBox
{
public:
  pqInternals(pqSearchBox* self)
  {
    this->setupUi(self);

    self->connect(
      this->SearchLineEdit, SIGNAL(textChanged(QString)), self, SIGNAL(textChanged(QString)));
    self->connect(
      this->AdvancedButton, SIGNAL(clicked(bool)), self, SLOT(onAdvancedButtonClicked(bool)));
    self->connect(self->settings(), SIGNAL(modified()), self, SLOT(updateFromSettings()));
  };
};

//-----------------------------------------------------------------------------
// Class

//-----------------------------------------------------------------------------
pqSearchBox::pqSearchBox(QWidget* _parent)
  : Superclass(_parent)
  , Internals(new pqSearchBox::pqInternals(this))
  , SettingKey("")
{
  this->setAdvancedSearchEnabled(false);
}

//-----------------------------------------------------------------------------
pqSearchBox::pqSearchBox(bool advancedSearchEnabled, const QString& settingKey, QWidget* _parent)
  : Superclass(_parent)
  , Internals(new pqSearchBox::pqInternals(this))
  , SettingKey("")
{
  this->setAdvancedSearchEnabled(advancedSearchEnabled);
  this->setSettingKey(settingKey);
}

//-----------------------------------------------------------------------------
pqSearchBox::~pqSearchBox() = default;

//-----------------------------------------------------------------------------
void pqSearchBox::keyPressEvent(QKeyEvent* keyEvent)
{
  // Remove the current text in the line edit on escape pressed
  if (keyEvent && keyEvent->key() == Qt::Key_Escape &&
    !this->Internals->SearchLineEdit->text().isEmpty())
  {
    this->Internals->SearchLineEdit->clear();
  }
  else
  {
    this->Superclass::keyPressEvent(keyEvent);
  }
}

//-----------------------------------------------------------------------------
QString pqSearchBox::text() const
{
  return this->Internals->SearchLineEdit->text();
}

//-----------------------------------------------------------------------------
void pqSearchBox::setText(const QString& text)
{
  this->Internals->SearchLineEdit->setText(text);
}

//-----------------------------------------------------------------------------
bool pqSearchBox::isAdvancedSearchActive() const
{
  if (this->isAdvancedSearchEnabled())
  {
    return this->Internals->AdvancedButton->isChecked();
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqSearchBox::setAdvancedSearchActive(bool use)
{
  if (this->isAdvancedSearchEnabled())
  {
    this->Internals->AdvancedButton->setChecked(use);
  }
}

//-----------------------------------------------------------------------------
bool pqSearchBox::isAdvancedSearchEnabled() const
{
  return this->Internals->AdvancedButton->isEnabled();
}

//-----------------------------------------------------------------------------
void pqSearchBox::setAdvancedSearchEnabled(bool enable)
{
  this->Internals->AdvancedButton->setVisible(enable);
  this->Internals->AdvancedButton->setEnabled(enable);
  this->updateFromSettings();
}

//-----------------------------------------------------------------------------
QSettings* pqSearchBox::settings() const
{
  return pqApplicationCore::instance()->settings();
}

//-----------------------------------------------------------------------------
QString pqSearchBox::settingKey() const
{
  return this->SettingKey;
}

//-----------------------------------------------------------------------------
QString pqSearchBox::setSettingKey(const QString& key)
{
  QString oldKey = this->SettingKey;
  if (this->SettingKey == key)
  {
    return oldKey;
  }

  this->SettingKey = key;
  this->updateFromSettings();
  Q_EMIT this->settingKeyChanged(this->SettingKey);
  return oldKey;
}

//-----------------------------------------------------------------------------
void pqSearchBox::onAdvancedButtonClicked(bool clicked)
{
  assert(this->isAdvancedSearchEnabled());
  this->updateSettings();
  Q_EMIT this->advancedSearchActivated(clicked);
}

//-----------------------------------------------------------------------------
void pqSearchBox::updateFromSettings()
{
  QSettings* settings = this->settings();
  if (settings && !this->SettingKey.isEmpty())
  {
    this->Internals->AdvancedButton->setChecked(settings->value(this->SettingKey, false).toBool());
  }
}

//-----------------------------------------------------------------------------
void pqSearchBox::updateSettings()
{
  QSettings* settings = this->settings();
  if (settings && !this->SettingKey.isEmpty())
  {
    settings->setValue(this->SettingKey, this->Internals->AdvancedButton->isChecked());
  }
}

//-----------------------------------------------------------------------------
void pqSearchBox::setPlaceholderText(const QString& text)
{
  this->Internals->SearchLineEdit->setPlaceholderText(text);
}

//-----------------------------------------------------------------------------
QString pqSearchBox::placeholderText() const
{
  return this->Internals->SearchLineEdit->placeholderText();
}
