/*=========================================================================

   Program: ParaView
   Module:    pqChartTitle.h

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

/// \file pqChartTitle.h
/// \date 11/17/2006

#ifndef _pqChartTitle_h
#define _pqChartTitle_h


#include "QtChartExport.h"
#include <QWidget>
#include <QString> // Needed for return type

class QPainter;


/// \class pqChartTitle
/// \brief
///   The pqChartTitle class is used to draw a chart title.
///
/// The text for the title can be drawn horizontally or vertically.
/// This allows the title to be used on a vertical axis.
class QTCHART_EXPORT pqChartTitle : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart title instance.
  /// \param orient The orientation of the title.
  /// \param parent The parent widget.
  pqChartTitle(Qt::Orientation orient=Qt::Horizontal, QWidget *parent=0);
  virtual ~pqChartTitle() {}

  /// \brief
  ///   Gets the orientation of the chart title.
  /// \return
  ///   The orientation of the chart title.
  Qt::Orientation getOrientation() const {return this->Orient;}

  /// \brief
  ///   Sets the orientation of the chart title.
  /// \param orient The orientation of the title.
  void setOrientation(Qt::Orientation orient);

  /// \brief
  ///   Gets the chart title text.
  /// \return
  ///   The chart title text.
  QString getText() const {return this->Text;}

  /// \brief
  ///   Sets the chart title text.
  /// \param text The text to display.
  void setText(const QString &text);

  /// \brief
  ///   Gets the text alignment flags for the title.
  /// \return
  ///   The text alignment flags for the title.
  int getTextAlignment() const {return this->Align;}

  /// \brief
  ///   Sets the text alignment flags for the title.
  /// \param flags The text alignment flags to use.
  void setTextAlignment(int flags) {this->Align = flags;}

  /// \brief
  ///   Gets the preferred size of the chart title.
  /// \return
  ///   The preferred size of the chart title.
  virtual QSize sizeHint() const {return this->Bounds;}

  /// \brief
  ///   Draws the title using the given painter.
  /// \param painter The painter to use.
  void drawTitle(QPainter &painter);

signals:
  /// Emitted when the title orientation has changed.
  void orientationChanged();

protected:
  /// \brief
  ///   Updates the layout when the font changes.
  /// \param e Event specific information.
  /// \return
  ///   True if the event was handled.
  virtual bool event(QEvent *e);

  /// \brief
  ///   Draws the chart title.
  /// \param e Event specific information.
  virtual void paintEvent(QPaintEvent *e);

private:
  /// Calculates the preferred size of the chart title.
  void calculateSize();

private:
  QString Text;           ///< Stores the display text.
  QSize Bounds;           ///< Stores the preferred size.
  Qt::Orientation Orient; ///< Stores the title orientation.
  int Align;              ///< Stores the text alignment flags.
};

#endif
