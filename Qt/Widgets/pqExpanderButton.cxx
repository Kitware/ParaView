/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
