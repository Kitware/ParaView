/*=========================================================================

   Program: ParaView
   Module:    pqColorMapModel.cxx

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

/// \file pqColorMapModel.cxx
/// \date 3/9/2007

#include "pqColorMapModel.h"

#include "pqChartValue.h"
#include "pqChartPixelScale.h"

#include <QColor>
#include <QList>
#include <QPainter>
#include <QRect>
#include <QSize>

#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626
#endif

//=============================================================================
class pqColorMapModelItem
{
public:
  pqColorMapModelItem();
  pqColorMapModelItem(const pqChartValue &value, const QColor &color);
  pqColorMapModelItem(const pqChartValue &value, const QColor &color,
      const pqChartValue &opacity);
  ~pqColorMapModelItem() {}

  pqChartValue Value;
  QColor Color;
  pqChartValue Opacity;
};


class pqColorMapModelInternal : public QList<pqColorMapModelItem *> {};


//=============================================================================
// Given two angular orientations, returns the smallest angle between the two.
inline double pqColorMapModelAngleDiff(double a1, double a2)
{
  double adiff = a1 - a2;
  if (adiff < 0.0) adiff = -adiff;
  while (adiff >= M_PI) adiff -= M_PI;
  return adiff;
}

// For the case when interpolating from a saturated color to an unsaturated
// color, find a hue for the unsaturated color that makes sense.
inline double pqColorMapModelAdjustHue(double satM, double satS, double satH,
                                       double unsatM)
{
  if (satM >= unsatM - 0.1)
    {
    // The best we can do is hold hue constant.
    return satH;
    }
  else
    {
    // This equation is designed to make the perceptual change of the
    // interpolation to be close to constant.
    double hueSpin = (  satS*sqrt(unsatM*unsatM - satM*satM)
                      / (satM*sin(satS)) );
    // Spin hue away from 0 except in purple hues.
    if (satH > -0.3*M_PI)
      {
      return satH + hueSpin;
      }
    else
      {
      return satH - hueSpin;
      }
    }
}



//----------------------------------------------------------------------------
pqColorMapModelItem::pqColorMapModelItem()
  : Value(), Color()
{
}

pqColorMapModelItem::pqColorMapModelItem(const pqChartValue &value,
    const QColor &color)
  : Value(value), Color(color), Opacity((double)1.0)
{
}

pqColorMapModelItem::pqColorMapModelItem(const pqChartValue &value,
    const QColor &color, const pqChartValue &opacity)
  : Value(value), Color(color), Opacity(opacity)
{
}


//----------------------------------------------------------------------------
pqColorMapModel::pqColorMapModel(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqColorMapModelInternal();
  this->Space = pqColorMapModel::HsvSpace;
  this->NanColor = QColor(127, 0, 0);
  this->InModify = false;
}

pqColorMapModel::pqColorMapModel(const pqColorMapModel &other)
  : QObject(0)
{
  this->Internal = new pqColorMapModelInternal();
  this->Space = other.Space;
  this->NanColor = other.NanColor;
  this->InModify = false;

  // Copy the list of points.
  QList<pqColorMapModelItem *>::ConstIterator iter = other.Internal->begin();
  for( ; iter != other.Internal->end(); ++iter)
    {
    this->Internal->append(new pqColorMapModelItem(
        (*iter)->Value, (*iter)->Color, (*iter)->Opacity));
    }
}

pqColorMapModel::~pqColorMapModel()
{
  this->InModify = true;
  this->removeAllPoints();
  delete this->Internal;
}

void pqColorMapModel::setColorSpace(pqColorMapModel::ColorSpace space)
{
  if(this->Space != space)
    {
    this->Space = space;
    if(!this->InModify)
      {
      emit this->colorSpaceChanged();
      }
    }
}

int pqColorMapModel::getColorSpaceAsInt() const
{
  switch(this->Space)
    {
    case pqColorMapModel::RgbSpace:
      return 0;
    case pqColorMapModel::WrappedHsvSpace:
      return 2;
    case pqColorMapModel::LabSpace:
      return 3;
    case pqColorMapModel::DivergingSpace:
      return 4;
    case pqColorMapModel::HsvSpace:
    default:
      return 1;
    }
}

void pqColorMapModel::setColorSpaceFromInt(int space)
{
  switch(space)
    {
    case 0:
      {
      this->setColorSpace(pqColorMapModel::RgbSpace);
      break;
      }
    case 1:
      {
      this->setColorSpace(pqColorMapModel::HsvSpace);
      break;
      }
    case 2:
      {
      this->setColorSpace(pqColorMapModel::WrappedHsvSpace);
      break;
      }
    case 3:
      {
      this->setColorSpace(pqColorMapModel::LabSpace);
      break;
      }
    case 4:
      {
      this->setColorSpace(pqColorMapModel::DivergingSpace);
      break;
      }
    }
}

int pqColorMapModel::getNumberOfPoints() const
{
  return this->Internal->size();
}

void pqColorMapModel::addPoint(const pqChartValue &value, const QColor &color)
{
  this->addPoint(value, color, pqChartValue((double)1.0));
}

void pqColorMapModel::addPoint(const pqChartValue &value, const QColor &color,
    const pqChartValue &opacity)
{
  // The list of points should be in ascending value order. Add the
  // new point according to its value.
  QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if(value == (*iter)->Value)
      {
      return;
      }
    else if(value < (*iter)->Value)
      {
      break;
      }
    }

  pqColorMapModelItem *item = new pqColorMapModelItem(value, color, opacity);
  if(iter == this->Internal->end())
    {
    // Add the point to the end of the list if it is greater than all
    // the current points.
    this->Internal->append(item);
    }
  else
    {
    this->Internal->insert(iter, item);
    }

  if(!this->InModify)
    {
    emit this->pointAdded(this->Internal->indexOf(item));
    }
}

void pqColorMapModel::removePoint(int index)
{
  if(index >= 0 && index < this->Internal->size())
    {
    if(!this->InModify)
      {
      emit this->removingPoint(index);
      }

    pqColorMapModelItem *item = this->Internal->takeAt(index);
    delete item;
    if(!this->InModify)
      {
      emit this->pointRemoved(index);
      }
    }
}

void pqColorMapModel::removeAllPoints()
{
  if(this->Internal->size() > 0)
    {
    QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++ iter)
      {
      delete (*iter);
      }

    this->Internal->clear();
    if(!this->InModify)
      {
      emit this->pointsReset();
      }
    }
}

void pqColorMapModel::startModifyingData()
{
  this->InModify = true;
}

void pqColorMapModel::finishModifyingData()
{
  if(this->InModify)
    {
    this->InModify = false;
    emit this->pointsReset();
    }
}

void pqColorMapModel::getPointValue(int index, pqChartValue &value) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    value = (*this->Internal)[index]->Value;
    }
}

void pqColorMapModel::setPointValue(int index, const pqChartValue &value)
{
  if(index >= 0 && index < this->Internal->size() &&
      (*this->Internal)[index]->Value != value)
    {
    (*this->Internal)[index]->Value = value;
    if(!this->InModify)
      {
      emit this->valueChanged(index, value);
      }
    }
}

void pqColorMapModel::getPointColor(int index, QColor &color) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    color = (*this->Internal)[index]->Color;
    }
}

void pqColorMapModel::setPointColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->size() &&
      (*this->Internal)[index]->Color != color)
    {
    (*this->Internal)[index]->Color = color;
    if(!this->InModify)
      {
      emit this->colorChanged(index, color);
      }
    }
}

void pqColorMapModel::getPointOpacity(int index, pqChartValue &opacity) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    opacity = (*this->Internal)[index]->Opacity;
    }
}

void pqColorMapModel::setPointOpacity(int index, const pqChartValue &opacity)
{
  if(index >= 0 && index < this->Internal->size() &&
      (*this->Internal)[index]->Opacity != opacity)
    {
    (*this->Internal)[index]->Opacity = opacity;
    if(!this->InModify)
      {
      emit this->opacityChanged(index, opacity);
      }
    }
}

bool pqColorMapModel::isRangeNormalized() const
{
  if(this->Internal->size() > 1)
    {
    return this->Internal->first()->Value == (float)0.0 &&
        this->Internal->last()->Value == (float)1.0;
    }

  return false;
}

void pqColorMapModel::getValueRange(pqChartValue &min, pqChartValue &max) const
{
  if(this->Internal->size() > 0)
    {
    min = this->Internal->first()->Value;
    max = this->Internal->last()->Value;
    }
}

void pqColorMapModel::getNanColor(QColor &color) const
{
  color = this->NanColor;
}

void pqColorMapModel::setNanColor(const QColor &color)
{
  if (this->NanColor != color)
    {
    this->NanColor = color;
    if (!this->InModify)
      {
      emit nanColorChanged(this->NanColor);
      }
    }
}

void pqColorMapModel::setValueRange(const pqChartValue &min,
    const pqChartValue &max)
{
  if(this->Internal->size() == 0)
    {
    return;
    }

  // Scale the current points to fit the given range.
  if(this->Internal->size() == 1)
    {
    this->Internal->first()->Value = min;
    }
  else
    {
    pqChartValue newMin, newRange;
    pqChartValue oldMin = this->Internal->first()->Value;
    pqChartValue oldRange = this->Internal->last()->Value - oldMin;
    if(max < min)
      {
      newMin = max;
      newRange = min - max;
      }
    else
      {
      newMin = min;
      newRange = max - min;
      }

    QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      (*iter)->Value = (((*iter)->Value - oldMin) * newRange) / oldRange;
      (*iter)->Value += newMin;
      }
    }

  if(!this->InModify)
    {
    emit this->pointsReset();
    }
}

QPixmap pqColorMapModel::generateGradient(const QSize &size) const
{
  if(this->Internal->size() < 2 || size.width() <= 0 || size.height() <= 0)
    {
    return QPixmap();
    }

  // Create a pixmap and painter.
  QPixmap gradient(size);
  QPainter painter(&gradient);

  // Set up the pixel-value map for the image size.
  pqChartPixelScale pixelMap;
  pixelMap.setPixelRange(1, size.width() - 1);
  pixelMap.setValueRange(this->Internal->first()->Value,
      this->Internal->last()->Value);

  // Draw the first color.
  int i = 0;
  QColor next, previous;
  QList<pqColorMapModelItem *>::Iterator iter = this->Internal->begin();
  previous = (*iter)->Color;
  int imageHeight = gradient.height();
  painter.setPen(previous);
  painter.drawLine(0, 0, 0, imageHeight);

  // Loop through the points to draw the gradient(s).
  int px = 1;
  int p1 = pixelMap.getPixel((*iter)->Value);
  for(++i, ++iter; iter != this->Internal->end(); ++i, ++iter)
    {
    // Draw the colors between the previous and next color.
    next = (*iter)->Color;
    int p2 = pixelMap.getPixel((*iter)->Value);
    int w = p2 - p1;
    if(w > 0)
      {
      int x1 = px - 1;
      int x2 = x1 + w;
      for( ; px <= x2; px++)
        {
        // Use RGB, HSV, or CIE-L*ab space depending on the user option.
        if(px == x2)
          {
          painter.setPen(next);
          }
        else if(this->Space == pqColorMapModel::RgbSpace)
          {
          // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
          int r=0, g=0, b=0;
          r = ((px - x1)*(next.red() - previous.red()))/w + previous.red();
          g = ((px - x1)*(next.green() - previous.green()))/w +
              previous.green();
          b = ((px - x1)*(next.blue() - previous.blue()))/w + previous.blue();
          painter.setPen(QColor(r, g, b));
          }
        else if(this->Space == pqColorMapModel::HsvSpace ||
            this->Space == pqColorMapModel::WrappedHsvSpace)
          {
          // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
          int s=0, v=0;
          int h = next.hue();
          int h1 = previous.hue();
          if(this->Space == pqColorMapModel::WrappedHsvSpace &&
              (h - h1 > 180 || h1 - h > 180))
            {
            if(h1 > h)
              {
              h1 -= 360;
              }
            else
              {
              h -= 360;
              }
            }

          h = ((px - x1)*(h - h1))/w + h1;
          if(h < 0)
            {
            h += 360;
            }

          s = ((px - x1)*(next.saturation() - previous.saturation()))/w +
              previous.saturation();
          v = ((px - x1)*(next.value() - previous.value()))/w + previous.value();
          painter.setPen(QColor::fromHsv(h, s, v));
          }
        else if (this->Space == pqColorMapModel::LabSpace)
          {
          double L_next, a_next, b_next, L_previous, a_previous, b_previous;
          pqColorMapModel::RGBToLab(next.redF(), next.greenF(), next.blueF(),
                                    &L_next, &a_next, &b_next);
          pqColorMapModel::RGBToLab(
                           previous.redF(), previous.greenF(), previous.blueF(),
                           &L_previous, &a_previous, &b_previous);
          double L = ((px - x1)*(L_next - L_previous))/w + L_previous;
          double a = ((px - x1)*(a_next - a_previous))/w + a_previous;
          double b = ((px - x1)*(b_next - b_previous))/w + b_previous;
          double red, green, blue;
          pqColorMapModel::LabToRGB(L, a, b, &red, &green, &blue);
          QColor color;
          color.setRgbF(red, green, blue);
          painter.setPen(color);
          }
        else if(this->Space == pqColorMapModel::DivergingSpace)
          {
          double M_next, s_next, h_next, M_previous, s_previous, h_previous;
          pqColorMapModel::RGBToMsh(next.redF(), next.greenF(), next.blueF(),
                                    &M_next, &s_next, &h_next);
          pqColorMapModel::RGBToMsh(
                           previous.redF(), previous.greenF(), previous.blueF(),
                           &M_previous, &s_previous, &h_previous);

          double interp = (double)(px - x1)/w;

          // If the endpoints are distinct saturated colors, then place white in
          // between them.
          if (   (s_next > 0.05) && (s_previous > 0.05)
              && (pqColorMapModelAngleDiff(h_next, h_previous) > 0.33*M_PI) )
            {
            // Insert the white midpoint by setting one end to white and
            // adjusting the interpolation value.
            if (interp < 0.5)
              {
              M_next = 95.0;  s_next = 0.0;  h_next = 0.0;
              interp = 2.0*interp;
              }
            else
              {
              M_previous = 95.0;  s_previous = 0.0; h_previous = 0.0;
              interp = 2.0*interp - 1.0;
              }
            }

          // If one color has no saturation, thin its hue is invalid.  In this
          // case, we want to set it to something logical so that the
          // interpolation of hue makes sense.
          if ((s_previous < 0.05) && (s_next > 0.05))
            {
            h_previous = pqColorMapModelAdjustHue(M_next, s_next, h_next,
                                                  M_previous);
            }
          else if ((s_next < 0.01) && (s_previous > 0.01))
            {
            h_next = pqColorMapModelAdjustHue(
                                             M_previous, s_previous, h_previous,
                                             M_next);
            }

          double M = (1-interp)*M_previous + interp*M_next;
          double s = (1-interp)*s_previous + interp*s_next;
          double h = (1-interp)*h_previous + interp*h_next;

          double red, green, blue;
          pqColorMapModel::MshToRGB(M, s, h, &red, &green, &blue);
          QColor color;
          color.setRgbF(red, green, blue);
          painter.setPen(color);
          }

        painter.drawLine(px, 0, px, imageHeight);
        }
      }

    previous = next;
    p1 = p2;
    }

  // Make sure the last pixel is drawn.
  if(px < pixelMap.getMaxPixel())
    {
    painter.drawLine(px, 0, px, imageHeight);
    }

  // Finally, add a border to the gradient.
  QRect border(0, 0, size.width() - 1, size.height() - 1);
  painter.setPen(QColor(100, 100, 100));
  painter.drawRect(border);

  return gradient;
}

pqColorMapModel &pqColorMapModel::operator=(const pqColorMapModel &other)
{
  this->Space = other.Space;

  // Remove the current points and copy the new points.
  bool oldModify = this->InModify;
  this->InModify = false;
  this->removeAllPoints();
  QList<pqColorMapModelItem *>::ConstIterator iter = other.Internal->begin();
  for( ; iter != other.Internal->end(); ++iter)
    {
    this->Internal->append(new pqColorMapModelItem(
        (*iter)->Value, (*iter)->Color, (*iter)->Opacity));
    }

  other.getNanColor(this->NanColor);

  this->InModify = oldModify;
  if(!this->InModify)
    {
    emit this->pointsReset();
    }

  return *this;
}


//-----------------------------------------------------------------------------
void pqColorMapModel::RGBToLab(double red, double green, double blue,
                               double *L, double *a, double *b)
{
  double var_R = red;
  double var_G = green;
  double var_B = blue;

  if ( var_R > 0.04045 ) var_R = pow(( var_R + 0.055 ) / 1.055, 2.4);
  else                   var_R = var_R / 12.92;
  if ( var_G > 0.04045 ) var_G = pow(( var_G + 0.055 ) / 1.055, 2.4);
  else                   var_G = var_G / 12.92;
  if ( var_B > 0.04045 ) var_B = pow(( var_B + 0.055 ) / 1.055, 2.4);
  else                   var_B = var_B / 12.92;

  var_R = var_R * 100;
  var_G = var_G * 100;
  var_B = var_B * 100;

  double x = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
  double y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
  double z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

  const double ref_X =  95.047;
  const double ref_Y = 100.000;
  const double ref_Z = 108.883;
  double var_X = x / ref_X;  //ref_X =  95.047  Observer= 2 deg, Illuminant= D65
  double var_Y = y / ref_Y;  //ref_Y = 100.000
  double var_Z = z / ref_Z;  //ref_Z = 108.883

  if ( var_X > 0.008856 ) var_X = pow(var_X, 1.0/3.0);
  else                    var_X = ( 7.787 * var_X ) + ( 16.0 / 116.0 );
  if ( var_Y > 0.008856 ) var_Y = pow(var_Y, 1.0/3.0);
  else                    var_Y = ( 7.787 * var_Y ) + ( 16.0 / 116.0 );
  if ( var_Z > 0.008856 ) var_Z = pow(var_Z, 1.0/3.0);
  else                    var_Z = ( 7.787 * var_Z ) + ( 16.0 / 116.0 );

  *L = ( 116 * var_Y ) - 16;
  *a = 500 * ( var_X - var_Y );
  *b = 200 * ( var_Y - var_Z );
}

void pqColorMapModel::LabToRGB(double L, double a, double b,
                               double *red, double *green, double *blue)
{
  double var_Y = ( L + 16 ) / 116;
  double var_X = a / 500 + var_Y;
  double var_Z = var_Y - b / 200;
    
  if ( pow(var_Y,3) > 0.008856 ) var_Y = pow(var_Y,3);
  else var_Y = ( var_Y - 16.0 / 116.0 ) / 7.787;
                                                            
  if ( pow(var_X,3) > 0.008856 ) var_X = pow(var_X,3);
  else var_X = ( var_X - 16.0 / 116.0 ) / 7.787;

  if ( pow(var_Z,3) > 0.008856 ) var_Z = pow(var_Z,3);
  else var_Z = ( var_Z - 16.0 / 116.0 ) / 7.787;
  const double ref_X =  95.047;
  const double ref_Y = 100.000;
  const double ref_Z = 108.883;
  double x = ref_X * var_X;   //ref_X =  95.047  Observer= 2 deg Illuminant= D65
  double y = ref_Y * var_Y;   //ref_Y = 100.000
  double z = ref_Z * var_Z;   //ref_Z = 108.883

  var_X = x / 100;        //X = From 0 to ref_X
  var_Y = y / 100;        //Y = From 0 to ref_Y
  var_Z = z / 100;        //Z = From 0 to ref_Z
 
  double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
  double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
  double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;
 
  if ( var_R > 0.0031308 ) var_R = 1.055 * ( pow(var_R, ( 1 / 2.4 )) ) - 0.055;
  else var_R = 12.92 * var_R;
  if ( var_G > 0.0031308 ) var_G = 1.055 * ( pow(var_G ,( 1 / 2.4 )) ) - 0.055;
  else  var_G = 12.92 * var_G;
  if ( var_B > 0.0031308 ) var_B = 1.055 * ( pow(var_B, ( 1 / 2.4 )) ) - 0.055;
  else var_B = 12.92 * var_B;

  *red   = var_R;
  *green = var_G;
  *blue  = var_B;
  
  //clip colors. ideally we would do something different for colors
  //out of gamut, but not really sure what to do atm.
  if (*red<0)   *red=0;
  if (*green<0) *green=0;
  if (*blue<0)  *blue=0;
  if (*red>1)   *red=1;
  if (*green>1) *green=1;
  if (*blue>1)  *blue=1;
}

//-----------------------------------------------------------------------------
void pqColorMapModel::RGBToMsh(double red, double green, double blue,
                               double *M, double *s, double *h)
{
  double L, a, b;
  pqColorMapModel::RGBToLab(red, green, blue, &L, &a, &b);

  *M = sqrt(L*L + a*a + b*b);
  *s = (*M > 0.001) ? acos(L/(*M)) : 0.0;
  *h = (*s > 0.001) ? atan2(b, a) : 0.0;
}

void pqColorMapModel::MshToRGB(double M, double s, double h,
                               double *red, double *green, double *blue)
{
  double L, a, b;

  L = M*cos(s);
  a = M*sin(s)*cos(h);
  b = M*sin(s)*sin(h);

  pqColorMapModel::LabToRGB(L, a, b, red, green, blue);
}
