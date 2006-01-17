#ifndef _pqChartLabel_h
#define _pqChartLabel_h

#include "pqChartExport.h"

#include <QObject>
#include <QRect>  // Needed for bounds member.
#include <QFont>  // Needed for font member.
#include <QColor> // Needed for grid and axis members.

class QTCHART_EXPORT pqChartLabel :
  public QObject
{
  Q_OBJECT

public:
  pqChartLabel(QObject *parent=0);

  void setBounds(const QRect& bounds);
  void setText(const QString& text);
  void setColor(const QColor& color);
  void setFont(const QFont& font);

  const QRect getSizeRequest();

  void draw(QPainter &painter, const QRect &area);

signals:
  /// Called when the label needs to be layed out again.
  void layoutNeeded();

  /// Called when the label needs to be repainted.
  void repaintNeeded();


private:
  QRect Bounds;
  QString Text;
  QColor Color;
  QFont Font;
};

#endif
