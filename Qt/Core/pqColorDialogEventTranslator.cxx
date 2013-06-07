/*=========================================================================

   Program: ParaView
   Module:    pqColorDialogEventTranslator.cxx

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
#include "pqColorDialogEventTranslator.h"

#include "pqCoreTestUtility.h"
#include "pqColorDialogEventPlayer.h"
#include "pqColorDialog.h"

#include <QEvent>
#include <QtDebug>
#include <QMenu>


//-----------------------------------------------------------------------------
pqColorDialogEventTranslator::pqColorDialogEventTranslator(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqColorDialogEventTranslator::~pqColorDialogEventTranslator()
{
}

//-----------------------------------------------------------------------------
bool pqColorDialogEventTranslator::translateEvent(
  QObject* object, QEvent* tr_event, bool& /*error*/)
{
  // Capture events from pqColorDialog and all its children.

  pqColorDialog* color_dialog = 0;
  while (object && !color_dialog)
    {
    color_dialog = qobject_cast<pqColorDialog*>(object);
    object = object->parent();
    }

  if (!color_dialog)
    {
    return false;
    }

  if (tr_event->type() == QEvent::FocusIn)
    {
    QObject::disconnect(color_dialog, 0, this, 0);
    QObject::connect(color_dialog, SIGNAL(currentColorChanged(const QColor&)),
      this, SLOT(onColorChosen(const QColor&)));
    QObject::connect(color_dialog, SIGNAL(accepted()), this, SLOT(onAccepted()));
    }

  return true;
}


//-----------------------------------------------------------------------------
void pqColorDialogEventTranslator::onColorChosen(const QColor& color)
{
  pqColorDialog* color_dialog = qobject_cast<pqColorDialog*>(this->sender());

  QString colorvalue = QString("%1,%2,%3").arg(
    color.red()).arg(color.green()).arg(color.blue());

  emit this->recordEvent(color_dialog, pqColorDialogEventPlayer::EVENT_NAME(),
                         colorvalue);
}

//-----------------------------------------------------------------------------
void pqColorDialogEventTranslator::onAccepted()
{
  pqColorDialog* color_dialog = qobject_cast<pqColorDialog*>(this->sender());

  emit recordEvent(color_dialog, "accepted", "");
}
