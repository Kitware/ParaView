/*=========================================================================

   Program: ParaView
   Module:    pqPipelineDisplay.h

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

/// \file pqPipelineDisplay.h
/// \date 4/24/2006

#ifndef _pqPipelineDisplay_h
#define _pqPipelineDisplay_h


#include "pqConsumerDisplay.h"
#include <QPair>

class pqPipelineDisplayInternal;
class pqPipelineSource;
class pqRenderViewModule;
class pqScalarsToColors;
class pqServer;

class vtkPVDataSetAttributesInformation;
class vtkPVDataSetAttributesInformation;
class vtkPVArrayInformation;

class vtkSMDataObjectDisplayProxy;

/// This is PQ representation for a single display. A pqDisplay represents
/// a single vtkSMDataObjectDisplayProxy. The display can be added to
/// only one render module or more (ofcouse on the same server, this class
/// doesn't worry about that.
class PQCORE_EXPORT pqPipelineDisplay : public pqConsumerDisplay
{
  Q_OBJECT
public:
  pqPipelineDisplay(const QString& name,
    vtkSMDataObjectDisplayProxy* display, pqServer* server,
    QObject* parent=NULL);
  virtual ~pqPipelineDisplay();

  // Get the internal display proxy.
  vtkSMDataObjectDisplayProxy* getDisplayProxy() const;

  // Sets the default color mapping for the display.
  // The rules are:
  // If the source created a NEW point scalar array, use it.
  // Else if the source created a NEW cell scalar array, use it.
  // Else if the input color by array exists in this source, use it.
  // Else color by property.
  virtual void setDefaults(); 

  // Call to select the coloring array. 
  void colorByArray(const char* arrayname, int fieldtype);

  /// get the names of the arrays that a part may be colored by
  QList<QString> getColorFields();

  /// get the data ranges for a color field
  QList<QPair<double, double> >getColorFieldRanges(const QString& array);

  /// get the data range for a particular component. if component == -1,
  /// range for the vector magnitude is returned.
  QPair<double, double> getColorFieldRange(const QString& array, int component);

  /// set the array to color the part by
  void setColorField(const QString& field);

  /// get the array the part is colored by
  /// if raw is true, it will not add (point) or (cell) but simply
  /// return the array name
  QString getColorField(bool raw=false);

  /// Returns the lookuptable proxy, if any.
  vtkSMProxy* getLookupTableProxy();

  /// Returns the pqScalarsToColors object for the lookup table
  /// proxy if any.
  pqScalarsToColors* getLookupTable();

  /// Returns the number of components for the given field.
  /// field is a string of format "<arrayname> (cell|point)".
  int getColorFieldNumberOfComponents(const QString& field);

  /// Get the data bounds for the input of this display.
  /// Returns if the operation was successful.
  bool getDataBounds(double bounds[6]);
signals:
  // emitted when colorByArray()/setColorField() is called.
  void colorChanged();

public slots:
  // If lookuptable is set up and is used for coloring,
  // then calling this method resets the table ranges to match the current 
  // range of the selected array.
  void resetLookupTableScalarRange();

  // If lookuptable is set up and coloring is enabled, the this
  // ensure that the lookuptable scalar range is greater than than the
  // color array's scalar range. This call respects the lookup table's
  // "lock" on scalar range.
  void updateLookupTableScalarRange();

protected slots:
  void deferredSetDefaults();

private:
  pqPipelineDisplayInternal *Internal; 
  static void getColorArray(
    vtkPVDataSetAttributesInformation* attrInfo,
    vtkPVDataSetAttributesInformation* inAttrInfo,
    vtkPVArrayInformation*& arrayInfo);

  int getNumberOfComponents(const char* arrayname, int fieldType);
};

#endif
