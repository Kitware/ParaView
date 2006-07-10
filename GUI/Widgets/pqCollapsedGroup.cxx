/*=========================================================================

   Program: ParaView
   Module:    pqCollapsedGroup.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/

// self includes
#include "pqCollapsedGroup.h"

// Qt includes
#include "QtDebug"
#include "QResizeEvent"
#include "QLayout"

#include "QPushButton"

/* XPM */
static const char* pqCollapsedGroup_ShowIcon[] = {
/* columns rows colors chars-per-pixel */
"16 16 25 1",
"  c black",
". c #060F50",
"X c #4556A8",
"o c #5062B2",
"O c #5063B3",
"+ c #5A6FBD",
"@ c #5B6EBC",
"# c #5B6EBD",
"$ c #5B6FBD",
"% c #657BC6",
"& c #657BC7",
"* c #6F87D1",
"= c #6F88D0",
"- c #7087D0",
"; c #7088D1",
": c #738CD3",
"> c #7A94DA",
", c #7A94DB",
"< c #84A0E5",
"1 c #85A1E5",
"2 c #8FADEF",
"3 c #99B9F8",
"4 c #9AB9F8",
"5 c #A0C1FF",
"6 c None",
/* pixels */
"6666666666666666",
"66666.....666666",
"66666.555.666666",
"66666.555.666666",
"66666.444.666666",
"66666.222.666666",
"66666.<<<.666666",
"666...>>>...6666",
"66.:=-;*;=;*.666",
"666.&%%%&&&.6666",
"6666.$$$$$.66666",
"66666.OOO.666666",
"666666.X.6666666",
"6666666.66666666",
"6666666666666666",
"6666666666666666"
};

/* XPM */
static const char* pqCollapsedGroup_HideIcon[] = {
/* columns rows colors chars-per-pixel */
"16 16 42 1",
"  c black",
". c #060F50",
"X c #4252A5",
"o c #4353A6",
"O c #4453A7",
"+ c #4E60B0",
"@ c #4F60B1",
"# c #4F61B1",
"$ c #596DBC",
"% c #5B6EBC",
"& c #5B6FBC",
"* c #657BC6",
"= c #667BC7",
"- c #667DC7",
"; c #7189D1",
": c #7189D2",
"> c #7289D3",
", c #7C96DC",
"< c #7D97DD",
"1 c #86A2E6",
"2 c #87A2E6",
"3 c #87A4E7",
"4 c #88A4E7",
"5 c #88A5E8",
"6 c #89A5E8",
"7 c #89A5E9",
"8 c #89A6EA",
"9 c #8AA7EA",
"0 c #91B1F1",
"q c #92B1F2",
"w c #93B1F2",
"e c #93B2F3",
"r c #94B2F3",
"t c #95B3F4",
"y c #95B4F4",
"u c #9DBEFD",
"i c #9EBFFD",
"p c #9EC0FE",
"a c #9FC0FE",
"s c #A0C0FF",
"d c #A0C1FF",
"f c None",
/* pixels */
"ffffffffffffffff",
"fffffff.ffffffff",
"ffffff.s.fffffff",
"fffff.sss.ffffff",
"ffff.siiii.fffff",
"fff.ytw0000.ffff",
"ff.999999222.fff",
"fff...,,,...ffff",
"fffff.>;;.ffffff",
"fffff.==*.ffffff",
"fffff.&&$.ffffff",
"fffff.#++.ffffff",
"fffff.OXX.ffffff",
"fffff.....ffffff",
"ffffffffffffffff",
"ffffffffffffffff"
};

pqCollapsedGroup::pqCollapsedGroup(QWidget* parent)
  : QGroupBox(parent)
{
  this->initialize();
}

pqCollapsedGroup::pqCollapsedGroup(const QString& title, QWidget* parent)
  : QGroupBox(title, parent)
{
  this->initialize();
}

void pqCollapsedGroup::initialize()
{
  this->installEventFilter(this);
  this->Hidden = false;
  this->OldMargin = 0;

  this->HideButton = new QPushButton(this);
  this->HideButton->setMaximumHeight(15);
  this->HideButton->setMaximumWidth(15);
  this->HideButton->setFocusPolicy(Qt::NoFocus);
  this->HideButton->setFlat(true);

  this->HideIcon = new QIcon(
    QPixmap( pqCollapsedGroup_HideIcon ));
  this->ShowIcon = new QIcon(
    QPixmap( pqCollapsedGroup_ShowIcon ));
  this->HideButton->setIcon(*this->HideIcon);
  this->connect(this->HideButton, SIGNAL(clicked()),
    this, SLOT(buttonPressed()));
}

pqCollapsedGroup::~pqCollapsedGroup()
{
}

void pqCollapsedGroup::buttonPressed()
{
  if ( this->Hidden )
    {
    this->HideButton->setIcon(*this->HideIcon);
    this->Hidden = false;
    foreach(QObject* tmp, this->children())
      {
      QWidget* widget = qobject_cast<QWidget*>(tmp);
      if ( widget && widget != this->HideButton )
        {
        widget->setHidden(false);
        }
      }
    if ( this->layout() )
      {
      this->layout()->setMargin(this->OldMargin);
      }
    }
  else
    {
    this->HideButton->setIcon(*this->ShowIcon);
    this->Hidden = true;
    foreach(QObject* tmp, this->children())
      {
      QWidget* widget = qobject_cast<QWidget*>(tmp);
      if ( widget && widget != this->HideButton )
        {
        widget->setHidden(true);
        }
      }
    if ( this->layout() )
      {
      this->OldMargin = this->layout()->margin();
      this->layout()->setMargin(0);
      }
    }
  this->update();
}

bool pqCollapsedGroup::eventFilter(QObject* watched, QEvent* e)
{
  if ( e->type() == QEvent::Resize )
    {
    QResizeEvent* res = static_cast<QResizeEvent*>(e);
    this->HideButton->move(res->size().width()-this->HideButton->width()-5,2);
    }

  return QGroupBox::eventFilter(watched, e);
}

