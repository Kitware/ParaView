#ifndef _pqChartLabel_h
#define _pqChartLabel_h

#include "pqChartExport.h"

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

  /// Sets the text to be displayed by the label
  void setText(const QString& text);
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
