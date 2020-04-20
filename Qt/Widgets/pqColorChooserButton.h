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

#include "pqWidgetsModule.h"

#include <QColor>
#include <QToolButton>
#include <QVariant>

/**
* pqColorChooserButton is a QToolButton subclass suitable for showing a
* a button that allows the use to select/change color. It renders a color
* swatch next to the button text matching the chosen color.
*/
class PQWIDGETS_EXPORT pqColorChooserButton : public QToolButton
{
  Q_OBJECT
  Q_PROPERTY(QColor chosenColor READ chosenColor WRITE setChosenColor);
  Q_PROPERTY(QVariantList chosenColorRgbF READ chosenColorRgbF WRITE setChosenColorRgbF);
  Q_PROPERTY(QVariantList chosenColorRgbaF READ chosenColorRgbaF WRITE setChosenColorRgbaF);
  Q_PROPERTY(bool showAlphaChannel READ showAlphaChannel WRITE setShowAlphaChannel);

public:
  /**
  * constructor requires a QComboBox
  */
  pqColorChooserButton(QWidget* p);

  /**
  * get the color
  */
  QColor chosenColor() const;

  /**
  * Returns the chosen color as a QVariantList with exatctly 3 QVariants with
  * values in the range [0, 1] for each of the 3 color components.
  */
  QVariantList chosenColorRgbF() const;

  /**
  * Returns the chosen color as a QVariantList with exatctly 4 QVariants with
  * values in the range [0, 1] for each of the 4 color components.
  */
  QVariantList chosenColorRgbaF() const;

  /**
  * Set/Get the ratio of icon radius to button height
  */
  void setIconRadiusHeightRatio(double val) { this->IconRadiusHeightRatio = val; }
  double iconRadiusHeightRatio() const { return this->IconRadiusHeightRatio; }

  /**
  * When true, the widget will allow users to choose the alpha channel.
  */
  bool showAlphaChannel() const { return this->ShowAlphaChannel; }
  void setShowAlphaChannel(bool val) { this->ShowAlphaChannel = val; }

Q_SIGNALS:
  /**
  * signal color changed. This is fired in setChosenColor() only
  * when the color is indeed different.
  */
  void chosenColorChanged(const QColor&);

  /**
  * signal color selected. Unlike chosenColorChanged() this is fired
  * even if the color hasn't changed.
  */
  void validColorChosen(const QColor&);

public Q_SLOTS:
  /**
  * set the color
  */
  virtual void setChosenColor(const QColor&);

  /**
  * set the color as a QVariantList with exactly 3 QVariants with
  * values in the range [0, 1] for each of the 3 color components.
  */
  void setChosenColorRgbF(const QVariantList&);

  /**
  * set the color as a QVariantList with exactly 4 QVariants with
  * values in the range [0, 1] for each of the 4 color components.
  */
  void setChosenColorRgbaF(const QVariantList&);

  /**
  * show a dialog to choose the color
  */
  virtual void chooseColor();

protected:
  /**
  * overridden to resize the color icon.
  */
  void resizeEvent(QResizeEvent* rEvent) override;

  /**
  * renders an icon for the color.
  */
  QIcon renderColorSwatch(const QColor&);

  /**
  * RGBA values representing the chosen color
  */
  double Color[4];

  /**
  * the ratio of icon radius to button height
  */
  double IconRadiusHeightRatio;

  bool ShowAlphaChannel;
};

#endif
