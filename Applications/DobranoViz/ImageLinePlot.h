#ifndef _ImageLinePlot_h
#define _ImageLinePlot_h

#include "pqPiecewiseLine.h"

#include <QPixmap>

/// Line plot that displays an (optional) image as its tooltip (instead of the default data tooltip)
class ImageLinePlot :
  public pqPiecewiseLine
{
public:
  ImageLinePlot(const QPixmap& image);

  virtual void showTooltip(int index, QHelpEvent& event) const;
  
private:
  QPixmap Image;
};

#endif
