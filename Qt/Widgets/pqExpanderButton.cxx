// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqExpanderButton.h"
#include "ui_pqExpanderButton.h"

#include <QIcon>
#include <QMouseEvent>
#include <QStyle>

class pqExpanderButton::pqInternals : public Ui::pqExpanderButton
{
public:
  pqInternals()
    : Pressed(false)
    , MinusPixmap(QIcon(":/QtWidgets/Icons/pqMinus.svg").pixmap(QSize(16, 16)))
    , PlusPixmap(QIcon(":/QtWidgets/Icons/pqPlus.svg").pixmap(QSize(16, 16)))
    , CollapsePixmap(QIcon(":/QtWidgets/Icons/pqCollapse.svg").pixmap(QSize(8, 8)))
    , ExpandPixmap(QIcon(":/QtWidgets/Icons/pqExpand.svg").pixmap(QSize(8, 8)))
  {
  }

  bool Pressed;
  QPixmap MinusPixmap;
  QPixmap PlusPixmap;
  QPixmap CollapsePixmap;
  QPixmap ExpandPixmap;
};

//-----------------------------------------------------------------------------
pqExpanderButton::pqExpanderButton(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqExpanderButton::pqInternals())
  , Checked(false)
  , CurrentLook(H1)
{
  this->Internals->setupUi(this);
  this->updateIcon();

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
  this->updateIcon();
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
void pqExpanderButton::setLook(Look newLook)
{
  if (this->CurrentLook == newLook)
  {
    return;
  }

  this->CurrentLook = newLook;
  this->updateIcon();

  if (this->CurrentLook == H2)
  {
    this->setFrameShape(QFrame::NoFrame);
  }
  else
  {
    this->setFrameShape(QFrame::StyledPanel);
  }
  this->update();
}

//-----------------------------------------------------------------------------
void pqExpanderButton::updateIcon()
{
  if (this->CurrentLook == H2)
  {
    this->Internals->icon->setPixmap(
      this->Checked ? this->Internals->CollapsePixmap : this->Internals->ExpandPixmap);
  }
  else
  {
    this->Internals->icon->setPixmap(
      this->Checked ? this->Internals->MinusPixmap : this->Internals->PlusPixmap);
  }
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
