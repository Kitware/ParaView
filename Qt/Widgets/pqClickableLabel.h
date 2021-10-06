/*=========================================================================

   Program:   ParaView
   Module:    pqDoubleRangeWidget.h

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
#ifndef pqClickableLabel_h
#define pqClickableLabel_h

#include "pqWidgetsModule.h"

#include <QLabel>

/**
 * @brief A simple clickable label that mimics
 * a push button and emits onClicked event
 */
class PQWIDGETS_EXPORT pqClickableLabel : public QLabel
{
  Q_OBJECT

public:
  /**
   * @brief Default constructor is deleted
   */
  pqClickableLabel() = delete;

  /**
   * @brief Default constructor that sets up the tooltip
   * and the label pixmap or text.
   * If the pixmap is nullptr, it will not be set.
   */
  pqClickableLabel(QWidget* widget, const QString& text, const QString& tooltip,
    const QString& statusTip, QPixmap* pixmap, QWidget* parent);

  /**
   * @brief Defaulted destructor for polymorphism
   */
  ~pqClickableLabel() override = default;

Q_SIGNALS:
  /**
   * @brief Signal emitted when the label
   * is clicked (to mimic a push button)
   * @param[in] w the widget attached to
   * the pqClickableLabel
   */
  void onClicked(QWidget* widget);

protected:
  void mousePressEvent(QMouseEvent* event) override;

  QWidget* Widget = nullptr;
};

#endif
