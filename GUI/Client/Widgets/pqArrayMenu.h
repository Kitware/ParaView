/*=========================================================================

   Program: ParaView
   Module:    pqArrayMenu.h

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

#ifndef _pqArrayMenu_h
#define _pqArrayMenu_h

#include "pqVariableType.h"
#include "pqWidgetsExport.h"

#include <QWidget>

class vtkSMSourceProxy;

/** Provides a standard user interface for selecting from a collection 
of dataset arrays (both cell and node). */
class PQWIDGETS_EXPORT pqArrayMenu :
  public QWidget
{
  Q_OBJECT

public:
  pqArrayMenu(QWidget *parent = 0);
  ~pqArrayMenu();

  /// Removes all arrays from the collection.
  void clear();
  /// Adds a variable to the collection.
  void add(pqVariableType type, const QString& name);
  /// Adds zero-to-many variables to the collection from the given source.
  void add(vtkSMSourceProxy* source);

  /// Returns the currently-selected variable.
  void getCurrent(pqVariableType& type, QString& name);

signals:
  /// Signal emitted whenever the user picks a different array.
  void arrayChanged();

public:
  class Filter
  {
  public:
    virtual ~Filter() {}
    
    virtual bool Allow() = 0;
    
  protected:
    Filter() {}
    Filter(const Filter&) {};
    Filter& operator=(const Filter&) { return *this; }
  };

private slots:
  void onArrayActivated(int row);

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
