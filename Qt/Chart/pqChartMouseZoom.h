/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseZoom.h

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

/// \file pqChartMouseZoom.h
/// \date 6/25/2007

#ifndef _pqChartMouseZoom_h
#define _pqChartMouseZoom_h


#include "QtChartExport.h"
#include "pqChartMouseFunction.h"

class pqChartContentsSpace;
class pqChartMouseBox;
class pqChartMouseZoomInternal;
class QCursor;
class QMouseEvent;


/// \class pqChartMouseZoom
/// \brief
///   The pqChartMouseZoom class zooms the contents in response to
///   mouse events.
class QTCHART_EXPORT pqChartMouseZoom : public pqChartMouseFunction
{
public:
  enum ZoomFlags
    {
    ZoomBoth,  ///< Zoom in both directions.
    ZoomXOnly, ///< Zoom only in the x-direction.
    ZoomYOnly  ///< Zoom only in the y-direction.
    };

public:
  /// \brief
  ///   Creates a new mouse zoom object.
  /// \param parent The parent object.
  pqChartMouseZoom(QObject *parent=0);
  virtual ~pqChartMouseZoom();

  /// \name pqChartMouseFunction Methods
  //@{
  virtual void setMouseOwner(bool owns);

  virtual bool mousePressEvent(QMouseEvent *e, pqChartContentsSpace *contents);
  virtual bool mouseMoveEvent(QMouseEvent *e, pqChartContentsSpace *contents);
  virtual bool mouseReleaseEvent(QMouseEvent *e,
      pqChartContentsSpace *contents);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e,
      pqChartContentsSpace *contents);
  //@}

  /// \brief
  ///   Gets the zoom flags used during interaction.
  /// \return
  ///   The zoom flags used during interaction.
  ZoomFlags getFlags() const {return this->Flags;}

protected:
  /// \brief
  ///   Sets the zoom flags to use during interaction.
  /// \param flags The zoom flags to use.
  void setFlags(ZoomFlags flags) {this->Flags = flags;}

private:
  pqChartMouseZoomInternal *Internal; ///< Stores the last position.
  ZoomFlags Flags;                    ///< Stores the zoom flags.
};


/// \class pqChartMouseZoomX
/// \brief
///   The pqChartMouseZoomX class zooms the contents in the x-direction.
class QTCHART_EXPORT pqChartMouseZoomX : public pqChartMouseZoom
{
public:
  /// \brief
  ///   Creates a new mouse zoom-x object.
  /// \param parent The parent object.
  pqChartMouseZoomX(QObject *parent=0);
  virtual ~pqChartMouseZoomX() {}
};


/// \class pqChartMouseZoomY
/// \brief
///   The pqChartMouseZoomY class zooms the contents in the y-direction.
class QTCHART_EXPORT pqChartMouseZoomY : public pqChartMouseZoom
{
public:
  /// \brief
  ///   Creates a new mouse zoom-y object.
  /// \param parent The parent object.
  pqChartMouseZoomY(QObject *parent=0);
  virtual ~pqChartMouseZoomY() {}
};


/// \class pqChartMouseZoomBox
/// \brief
///   The pqChartMouseZoomBox class zooms the contents to a rectangle.
class QTCHART_EXPORT pqChartMouseZoomBox : public pqChartMouseFunction
{
public:
  /// \brief
  ///   Creates a new mouse zoom box object.
  /// \param parent The parent object.
  pqChartMouseZoomBox(QObject *parent=0);
  virtual ~pqChartMouseZoomBox();

  /// \name pqChartMouseFunction Methods
  //@{
  virtual void setMouseOwner(bool owns);

  virtual void setMouseBox(pqChartMouseBox *box) {this->MouseBox = box;}

  virtual bool mousePressEvent(QMouseEvent *e, pqChartContentsSpace *contents);
  virtual bool mouseMoveEvent(QMouseEvent *e, pqChartContentsSpace *contents);
  virtual bool mouseReleaseEvent(QMouseEvent *e,
      pqChartContentsSpace *contents);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e,
      pqChartContentsSpace *contents);
  //@}

private:
  pqChartMouseBox *MouseBox; ///< Stores the mouse box.
  QCursor *ZoomCursor;       ///< Stores the zoom cursor.
};

#endif
