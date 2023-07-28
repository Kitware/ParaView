// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqExpanderButton.h"
#include "ui_pqExpanderButton.h"

#include <QIcon>
#include <QMouseEvent>

class pqExpanderButton::pqInternals : public Ui::pqExpanderButton
{
public:
  pqInternals()
    : Pressed(false)
    , CheckedPixmap(QIcon(":/QtWidgets/Icons/pqMinus.svg").pixmap(QSize(16, 16)))
    , UncheckedPixmap(QIcon(":/QtWidgets/Icons/pqPlus.svg").pixmap(QSize(16, 16)))
  {
  }

  bool Pressed;
  QPixmap CheckedPixmap;
  QPixmap UncheckedPixmap;
};

//-----------------------------------------------------------------------------
pqExpanderButton::pqExpanderButton(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqExpanderButton::pqInternals())
  , Checked(false)
{
  this->Internals->setupUi(this);
  this->Internals->icon->setPixmap(this->Internals->UncheckedPixmap);

#if defined(Q_WS_WIN) || defined(Q_OS_WIN)
  this->setFrameShadow(QFrame::Sunken);
#endif
}

//-----------------------------------------------------------------------------
pqExpanderButton::~pqExpanderButton()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqExpanderButton::toggle()
{
  this->setChecked(!this->checked());
}

//-----------------------------------------------------------------------------
void pqExpanderButton::setChecked(bool val)
{
  if (this->checked() == val)
  {
    return;
  }

  this->Checked = val;
  if (this->Checked)
  {
    this->Internals->icon->setPixmap(this->Internals->CheckedPixmap);
  }
  else
  {
    this->Internals->icon->setPixmap(this->Internals->UncheckedPixmap);
  }
  Q_EMIT this->toggled(this->Checked);
}

//-----------------------------------------------------------------------------
void pqExpanderButton::setText(const QString& txt)
{
  this->Internals->label->setText(txt);
}

//-----------------------------------------------------------------------------
QString pqExpanderButton::text() const
{
  return this->Internals->label->text();
}

//-----------------------------------------------------------------------------
void pqExpanderButton::mousePressEvent(QMouseEvent* evt)
{
  if (evt->button() == Qt::LeftButton && evt->buttons() == Qt::LeftButton)
  {
    this->Internals->Pressed = true;
  }
}

//-----------------------------------------------------------------------------
void pqExpanderButton::mouseReleaseEvent(QMouseEvent* evt)
{
  if (this->Internals->Pressed && evt->button() == Qt::LeftButton)
  {
    this->Internals->Pressed = false;
    this->toggle();
  }
}
