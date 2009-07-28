/*=========================================================================

   Program: ParaView
   Module:    pqPointMarker.h

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

/// \file pqPointMarker.h
/// \date 9/18/2006

#ifndef _pqPointMarker_h
#define _pqPointMarker_h


#include "QtChartExport.h"

class pqCrossPointMarkerInternal;
class pqPlusPointMarkerInternal;
class pqSquarePointMarkerInternal;
class pqCirclePointMarkerInternal;
class pqDiamondPointMarkerInternal;
class QPainter;
class QSize;


/// \class pqPointMarker
/// \brief
///   The pqPointMarker class is the base class for all marker classes.
///
/// The \c drawMarker method should be overloaded in the base class
/// to draw a different marker shape. The default implementation uses
/// the Qt painter method to draw a point. The \c drawMarker method
/// should draw the shape as if the point is located at the origin.
/// The painter should be translated to the point before calling the
/// \c drawMarker method to draw the marker in the correct location.
class QTCHART_EXPORT pqPointMarker
{
public:
  pqPointMarker() {}
  virtual ~pqPointMarker() {}

  /// \brief
  ///   Draws a point at the origin.
  ///
  /// The painter should be translated to the point location before
  /// calling this method.
  ///
  /// \param painter The painter used to draw the marker.
  virtual void drawMarker(QPainter &painter);
};


/// \class pqCrossPointMarker
/// \brief
///   The pqCrossPointMarker class is used to draw a cross ("X") marker.
class QTCHART_EXPORT pqCrossPointMarker : public pqPointMarker
{
public:
  /// \brief
  ///   Creates a cross marker instance.
  /// \param size The size of the cross marker.
  pqCrossPointMarker(const QSize &size);
  virtual ~pqCrossPointMarker();

  /// \brief
  ///   Draws a cross at the origin.
  /// \param painter The painter used to draw the marker.
  /// \sa pqPointMarker::drawMarker(QPainter &)
  virtual void drawMarker(QPainter &painter);

private:
  pqCrossPointMarkerInternal *Internal; ///< Stores the marker.
};


/// \class pqPlusPointMarker
/// \brief
///   The pqPlusPointMarker class is used to draw a plus ("+") marker.
class QTCHART_EXPORT pqPlusPointMarker : public pqPointMarker
{
public:
  /// \brief
  ///   Creates a plus marker instance.
  /// \param size The size of the plus marker.
  pqPlusPointMarker(const QSize &size);
  virtual ~pqPlusPointMarker();

  /// \brief
  ///   Draws a plus at the origin.
  /// \param painter The painter used to draw the marker.
  /// \sa pqPointMarker::drawMarker(QPainter &)
  virtual void drawMarker(QPainter &painter);

private:
  pqPlusPointMarkerInternal *Internal; ///< Stores the marker.
};


/// \class pqSquarePointMarker
/// \brief
///   The pqSquarePointMarker class is used to draw a square marker.
class QTCHART_EXPORT pqSquarePointMarker : public pqPointMarker
{
public:
  /// \brief
  ///   Creates a square marker instance.
  /// \param size The size of the square marker.
  pqSquarePointMarker(const QSize &size);
  virtual ~pqSquarePointMarker();

  /// \brief
  ///   Draws a square at the origin.
  /// \param painter The painter used to draw the marker.
  /// \sa pqPointMarker::drawMarker(QPainter &)
  virtual void drawMarker(QPainter &painter);

private:
  pqSquarePointMarkerInternal *Internal; ///< Stores the marker.
};


/// \class pqCirclePointMarker
/// \brief
///   The pqCirclePointMarker class is used to draw a circle marker.
class QTCHART_EXPORT pqCirclePointMarker : public pqPointMarker
{
public:
  /// \brief
  ///   Creates a circle marker instance.
  /// \param size The size of the circle marker.
  pqCirclePointMarker(const QSize &size);
  virtual ~pqCirclePointMarker();

  /// \brief
  ///   Draws a circle at the origin.
  /// \param painter The painter used to draw the marker.
  /// \sa pqPointMarker::drawMarker(QPainter &)
  virtual void drawMarker(QPainter &painter);

private:
  pqCirclePointMarkerInternal *Internal; ///< Stores the marker.
};


/// \class pqDiamondPointMarker
/// \brief
///   The pqDiamondPointMarker class is used to draw a diamond marker.
class QTCHART_EXPORT pqDiamondPointMarker : public pqPointMarker
{
public:
  /// \brief
  ///   Creates a diamond marker instance.
  /// \param size The size of the diamond marker.
  pqDiamondPointMarker(const QSize &size);
  virtual ~pqDiamondPointMarker();

  /// \brief
  ///   Draws a diamond at the origin.
  /// \param painter The painter used to draw the marker.
  /// \sa pqPointMarker::drawMarker(QPainter &)
  virtual void drawMarker(QPainter &painter);

private:
  pqDiamondPointMarkerInternal *Internal; ///< Stores the marker.
};

#endif
