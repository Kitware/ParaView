/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqChartLabel_h
#define _pqChartLabel_h

#include "QtChartExport.h"

#include <QObject>
#include <QRect>  // Needed for bounds member.
#include <QFont>  // Needed for font member.
#include <QColor> // Needed for grid and axis members.

/// Encapsulates a chart label that can be drawn horizontally or vertically, using any combination of text, color, and font
class QTCHART_EXPORT pqChartLabel :
  public QObject
{
  Q_OBJECT

public:
  pqChartLabel(QObject *parent=0);
  pqChartLabel(const QString& Text, QObject *parent=0);

  /// Enumerates the two drawing orientations for the text
  enum OrientationT
  {
    HORIZONTAL,
    VERTICAL
  };

  /// Sets/Gets the text to be displayed by the label
  void setText(const QString& text);
  QString getText(){return this->Text;};
  /// Sets the label color
  void setColor(const QColor& color);
  /// Sets the label font
  void setFont(const QFont& font);
  /// Sets the label orientation (the default is HORIZONTAL)
  void setOrientation(const OrientationT orientation);

  /// Returns the label's preferred size, based on font and orientation
  const QRect getSizeRequest();
  /// Sets the bounds within which the label will be drawn
  void setBounds(const QRect& bounds);
  const QRect getBounds() const;

  /// Renders the label using the given painter and the stored label bounds
  void draw(QPainter &painter, const QRect &area);

signals:
  /// Called when the label needs to be layed out again.
  void layoutNeeded();

  /// Called when the label needs to be repainted.
  void repaintNeeded();

private:
  /// Stores the label text
  QString Text;
  /// Stores the label color
  QColor Color;
  /// Stores the label font
  QFont Font;
  /// Stores the label orientation
  OrientationT Orientation;
  /// Stores the position / size used to render the label
  QRect Bounds;
};

#endif
