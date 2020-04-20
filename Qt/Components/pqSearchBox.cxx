/*=========================================================================

   Program: ParaView
   Module:    pqSearchBox.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
pqSearchBox::~pqSearchBox()
{
}

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
  return this->Internals->SearchLineEdit->setText(text);
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
