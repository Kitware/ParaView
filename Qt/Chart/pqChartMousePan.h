/*=========================================================================

   Program: ParaView
   Module:    pqChartMousePan.h

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

/// \file pqChartMousePan.h
/// \date 6/25/2007

#ifndef _pqChartMousePan_h
#define _pqChartMousePan_h


#include "QtChartExport.h"
#include "pqChartMouseFunction.h"

class pqChartContentsSpace;
class pqChartMousePanInternal;
class QMouseEvent;


/// \class pqChartMousePan
/// \brief
///   The pqChartMousePan class pans the contents in response to
///   mouse events.
class QTCHART_EXPORT pqChartMousePan : public pqChartMouseFunction
{
public:
  /// \brief
  ///   Creates a mouse pan instance.
  /// \param parent Te parent object.
  pqChartMousePan(QObject *parent=0);
  virtual ~pqChartMousePan();

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

private:
  pqChartMousePanInternal *Internal; ///< Stores the last mouse position.
};

#endif
