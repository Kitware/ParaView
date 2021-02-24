/*=========================================================================

   Program: ParaView
   Module:    pqColorButtonEventTranslator.cxx

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
#include "pqColorButtonEventTranslator.h"

#include "pqColorButtonEventPlayer.h"
#include "pqColorChooserButton.h"
#include "pqTestUtility.h"

#include <QEvent>
#include <QMenu>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqColorButtonEventTranslator::pqColorButtonEventTranslator(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqColorButtonEventTranslator::~pqColorButtonEventTranslator() = default;

//-----------------------------------------------------------------------------
bool pqColorButtonEventTranslator::translateEvent(
  QObject* object, QEvent* tr_event, bool& /*error*/)
{
  // Capture events from pqColorChooserButton and all its children.
  if (qobject_cast<QMenu*>(object))
  {
    // we don't want to capture events from the menu on the color chooser button.
    return false;
  }

  pqColorChooserButton* color_button = nullptr;
  while (object && !color_button)
  {
    color_button = qobject_cast<pqColorChooserButton*>(object);
    object = object->parent();
  }

  if (!color_button)
  {
    return false;
  }

  if (tr_event->type() == QEvent::FocusIn)
  {
    QObject::disconnect(color_button, nullptr, this, nullptr);
    QObject::connect(color_button, SIGNAL(validColorChosen(const QColor&)), this,
      SLOT(onColorChosen(const QColor&)));
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqColorButtonEventTranslator::onColorChosen(const QColor& color)
{
  pqColorChooserButton* color_button = qobject_cast<pqColorChooserButton*>(this->sender());

  QString colorvalue = QString("%1,%2,%3").arg(color.red()).arg(color.green()).arg(color.blue());

  Q_EMIT this->recordEvent(color_button, pqColorButtonEventPlayer::EVENT_NAME(), colorvalue);
}
