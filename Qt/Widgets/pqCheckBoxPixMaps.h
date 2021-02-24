/*=========================================================================

   Program: ParaView
   Module:    pqCheckBoxPixMaps.h

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
#ifndef pqCheckBoxPixMaps_h
#define pqCheckBoxPixMaps_h

#include "pqWidgetsModule.h"
#include <QObject>
#include <QPixmap>

class QWidget;

/**
* pqCheckBoxPixMaps is a helper class that can used to create pixmaps for
* checkboxs in various states. This is useful for showing checkboxes in qt-views.
*/
class PQWIDGETS_EXPORT pqCheckBoxPixMaps : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
  * parent cannot be nullptr.
  */
  pqCheckBoxPixMaps(QWidget* parent);

  /**
  * Returns a pixmap for the given state .
  */
  QPixmap getPixmap(Qt::CheckState state, bool active) const;
  QPixmap getPixmap(int state, bool active) const
  {
    return this->getPixmap(static_cast<Qt::CheckState>(state), active);
  }

private:
  Q_DISABLE_COPY(pqCheckBoxPixMaps)

  enum PixmapStateIndex
  {
    Checked = 0,
    PartiallyChecked = 1,
    UnChecked = 2,

    // All active states in lower half
    Checked_Active = 3,
    PartiallyChecked_Active = 4,
    UnChecked_Active = 5,

    PixmapCount = 6
  };
  QPixmap Pixmaps[6];
};

#endif
