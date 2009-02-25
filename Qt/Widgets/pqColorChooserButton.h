/*=========================================================================

   Program: ParaView
   Module:    pqColorChooserButton.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef pq_ColorChooserButton_h
#define pq_ColorChooserButton_h

#include "QtWidgetsExport.h"

#include <QToolButton>
#include <QColor>

/// 
class QTWIDGETS_EXPORT pqColorChooserButton : public QToolButton
{
  Q_OBJECT
  Q_PROPERTY(QColor chosenColor READ chosenColor WRITE setChosenColor)
public:
  /// constructor requires a QComboBox
  pqColorChooserButton(QWidget* p);
  /// get the color
  QColor chosenColor() const;

  /// Set the label to be used when firing beginUndo() signal.
  void setUndoLabel(const QString& lbl)
    { this->UndoLabel = lbl; }
  const QString& undoLabel() const
    { return this->UndoLabel; }
signals:
  /// Signals fired before and after the chosenColorChanged() signal is fired.
  /// This is used in ParaView to set up the creation of undo set.
  void beginUndo(const QString&);
  void endUndo();

  /// signal color changed
  void chosenColorChanged(const QColor&);  
  /// signal color selected
  void validColorChosen(const QColor&);  
public slots:
  /// set the color
  virtual void setChosenColor(const QColor&);

  /// show a dialog to choose the color
  virtual void chooseColor();
protected:
  QColor Color;
  QString UndoLabel;
};

#endif

