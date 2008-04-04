/*=========================================================================

   Program: ParaView
   Module:    pqColorTableDelegate.h

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

/// \file pqColorTableDelegate.h
/// \date 8/11/2006

#ifndef _pqColorTableDelegate_h
#define _pqColorTableDelegate_h


#include "QtWidgetsExport.h"
#include <QAbstractItemDelegate>


class QTWIDGETS_EXPORT pqColorTableDelegate : public QAbstractItemDelegate
{
public:
  pqColorTableDelegate(QObject *parent=0);
  virtual ~pqColorTableDelegate() {}

  virtual QSize sizeHint(const QStyleOptionViewItem &option,
      const QModelIndex &index) const;

  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
      const QModelIndex &index) const;

  void setColorSize(int size) {this->ColorSize = size > 3 ? size : 4;}
  int getColorSize() const {return this->ColorSize;}

protected:
  // QAbstractItemDelegate disables copy.
  pqColorTableDelegate(const pqColorTableDelegate &);
  pqColorTableDelegate &operator=(const pqColorTableDelegate &);

private:
  int ColorSize;
};

#endif
