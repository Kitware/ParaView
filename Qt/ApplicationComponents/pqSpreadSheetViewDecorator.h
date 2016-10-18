/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewDecorator.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqSpreadSheetViewDecorator_h
#define pqSpreadSheetViewDecorator_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class pqSpreadSheetView;
class pqOutputPort;
class pqDataRepresentation;

/**
* pqSpreadSheetViewDecorator adds decoration to a spread-sheet view. This
* includes widgets that allows changing the currently shown source/field etc.
* To use the decorator, simply instantiate a new decorator for every new
* instance of pqSpreadSheetView.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqSpreadSheetViewDecorator : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
  Q_PROPERTY(bool allowChangeOfSource READ allowChangeOfSource WRITE setAllowChangeOfSource)
public:
  pqSpreadSheetViewDecorator(pqSpreadSheetView* view);
  ~pqSpreadSheetViewDecorator();

  /**
  * Returns whether the user should allowed to interactive change the source.
  * being shown in the view. `true` by default.
  */
  bool allowChangeOfSource() const;

  /**
  * Set whether the user should be allowed to change the source interactively.
  */
  void setAllowChangeOfSource(bool val);

protected slots:
  void currentIndexChanged(pqOutputPort*);
  void showing(pqDataRepresentation*);
  void displayPrecisionChanged(int);
  void showToggleColumnPopupMenu();
  void resetColumnVisibility();
  void updateColumnVisibility();
  void toggleCellConnectivity();

protected:
  pqSpreadSheetView* Spreadsheet;

private:
  Q_DISABLE_COPY(pqSpreadSheetViewDecorator)

  class pqInternal;
  pqInternal* Internal;
};

#endif
