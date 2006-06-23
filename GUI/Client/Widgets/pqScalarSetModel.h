/*=========================================================================

   Program: ParaView
   Module:    pqScalarSetModel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#ifndef _pqScalarSetModel_h
#define _pqScalarSetModel_h

#include "pqWidgetsExport.h"

#include <QAbstractListModel>

/// Qt model that stores a sorted collection of unique floating-point numbers
class PQWIDGETS_EXPORT pqScalarSetModel :
  public QAbstractListModel
{
  typedef QAbstractListModel base;

  Q_OBJECT

public:
  pqScalarSetModel();
  ~pqScalarSetModel();

  /// Clears the model contents
  void clear();
  /// Inserts a floating-point number into the model
  void insert(double value);
  /// Erases a floating-point number from the model
  void erase(double value);
  /// Erases a zero-based row from the model
  void erase(int row);
  /// Returns the sorted collection of numbers stored in the model
  const QList<double> values();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    
private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqScalarSetModel_h
