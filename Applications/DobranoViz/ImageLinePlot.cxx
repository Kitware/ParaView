#include "ImageLinePlot.h"

#include <pqImageTip.h>

#include <QHelpEvent>

//////////////////////////////////////////////////////////////////////////////////////
// ImageLinePlot

ImageLinePlot::ImageLinePlot(const QPixmap& image) :
  pqPiecewiseLine(0),
  Image(image)
{
}

void ImageLinePlot::showTooltip(int index, QHelpEvent& event) const
{
  if(this->Image.isNull())
    {
    pqPiecewiseLine::showTooltip(index, event);
    return;
    }
  
  pqImageTip::showTip(this->Image, event.globalPos());
}
