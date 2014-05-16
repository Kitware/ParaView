/*=========================================================================

   Program: ParaView
   Module:    pqStandardColorButton.h

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
#ifndef __pqStandardColorButton_h
#define __pqStandardColorButton_h

#include "pqColorChooserButton.h"
#include "pqDeprecatedModule.h"


#include <QPointer>
class QActionGroup;
class vtkSMProxy;

/// DEPRECATED: Use pqColorChooserButtonWithPalettes instead.
class PQDEPRECATED_EXPORT pqStandardColorButton : public pqColorChooserButton
{
  Q_OBJECT
  typedef pqColorChooserButton Superclass;
public:
  pqStandardColorButton(QWidget* parent);
  virtual ~pqStandardColorButton();

  /// Returns the name of the standard color the user choose, otherwise empty.
  QString standardColor();

public slots:
  /// show a dialog to choose the color
  virtual void chooseColor();

  /// set the current standard color if any. This doesn't affect the actual
  /// color, that can be changed using setChosenColor(). This only affect the
  /// check state for the colors in the standard colors menu.
  void setStandardColor(const QString&);

private slots:
  /// Called to rebuild the menu. This is called everytime the popup menu is
  /// going to be shown.
  void updateMenu();

signals:
  /// Fired in conjunction with chosenColorChanged(). The standardColorName is
  /// set to the name of the standard color chosen if any, otherwise is empty.
  void standardColorChanged(const QString& standardColorName);

private slots:
  void actionTriggered(QAction*);

private:
  pqStandardColorButton(const pqStandardColorButton&); // Not implemented.
  void operator=(const pqStandardColorButton&); // Not implemented.


  vtkSMProxy* colorPalette() const;

  QPointer<QActionGroup> ActionGroup;
  QString StandardColor;
};

#endif
