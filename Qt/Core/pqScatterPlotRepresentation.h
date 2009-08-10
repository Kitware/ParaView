/*=========================================================================

   Program: ParaView
   Module:    pqScatterPlotRepresentation.h

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

/// \file pqScatterPlotRepresentation.h
/// \date 4/24/2006

#ifndef _pqScatterPlotRepresentation_h
#define _pqScatterPlotRepresentation_h


#include "pqDataRepresentation.h"
#include <QPair>

class pqScatterPlotRepresentationInternal;
class pqScatterPlotSource;
class pqRenderViewModule;
class pqServer;
class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMScatterPlotRepresentationProxy;

/// This is PQ representation for a single display. A pqRepresentation represents
/// a single vtkSMScatterPlotRepresentationProxy. The display can be added to
/// only one render module or more (of course on the same server, this class
/// doesn't worry about that.
class PQCORE_EXPORT pqScatterPlotRepresentation : public pqDataRepresentation
{
  Q_OBJECT
  typedef pqDataRepresentation Superclass;
public:
  // Constructor.
  // \c group :- smgroup in which the proxy has been registered.
  // \c name  :- smname as which the proxy has been registered.
  // \c repr  :- the representation proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqScatterPlotRepresentation( const QString& group, 
                               const QString& name,
                               vtkSMProxy* repr, 
                               pqServer* server,
                               QObject* parent=NULL);
  virtual ~pqScatterPlotRepresentation();

  // Get the internal display proxy.
  vtkSMScatterPlotRepresentationProxy* getRepresentationProxy() const;

  /// Sets the default color mapping for the display. The ColorArrayName 
  /// property is used to color the data. The name contains the type (point, 
  /// cell, coord), and the element to use to map the colors. If no element
  /// is given or if element == -1, the magnitude of the array is used.
  /// ColorArrayName = ({coord|point|cell},)<arrayname>(,<element>)
  virtual void setDefaultPropertyValues();

  /// Utility function that returns the data range for a particular array. 
  /// The array must match the expression: 
  /// ({coord|point|cell},)<arrayname>(,<element>).
  /// If the array component == -1, range for the vector magnitude is returned.
  QPair<double, double> getColorFieldRange(const QString& array)const;

  /// Returns the range for the currently selected color field i.e.
  /// the range for the array component (or magnitude) of the array by which
  /// this display is being colored, if at all.
  /// Returns 0,1 if no array is selected or the current array is invalid.
  QPair<double, double> getColorFieldRange()const;

  /// Utility function that returns if the array is a partial array or not.
  /// The array must match the expression: 
  /// ({coord|point|cell},)<arrayname>(,<element>).
  bool isPartial(const QString& array)const;

  /// Set the array to color.
  /// field is a string of format:
  ///    "({coord|point|cell|field},)<arrayname>(,[0-9]+)".
  /// Dangerous, please set the Property "ColorArrayName" instead.
  void setColorField(const QString& field);

  /// get the array the part is colored by. 
  /// Warning: the returned array matches the format: 
  ///   ({coord|point|cell|field},)<arrayname>(,[0-9]+)
  QString getColorField()const;

  /// Get the data bounds for the input of this display.
  /// Returns if the operation was successful.
  /// It propagates the computation to the vtkSMRepresentationProxy
  bool getDataBounds(double bounds[6])const;

  /// Returns the proxy for the piecewise function used to
  /// map scalars to opacity.
  //virtual vtkSMProxy* getScalarOpacityFunctionProxy();

  /// Returns the pqScalarOpacityFunction object for the piecewise
  /// function used to map scalars to opacity.
  //virtual pqScalarOpacityFunction* getScalarOpacityFunction();

  /// Returns the opacity.
  //double getOpacity() const;

signals:
  /// This is fire when any property that affects the color
  /// mode for the display changes.
  void colorChanged();

public slots:
  // If lookuptable is set up and is used for coloring,
  // then calling this method resets the table ranges to match the current 
  // range of the selected array.
  void resetLookupTableScalarRange();

  /// If color lookuptable is set up and coloring is enabled, the this
  /// ensure that the lookuptable scalar range is greater than than the
  /// color array's scalar range. It also updates the scalar range on
  /// the scalar-opacity function, if any. Both the ColorLUT and the 
  /// ScalarOpacityFunction may choose to ignore the set scalar range
  /// based on value ScalePointsWithRange.
  void updateLookupTableScalarRange();

protected slots:
  /// Called when the "ColorArrayName" property changes. 
  /// We have to ensure that the color field is initialized 
  void onColorArrayNameChanged();

  /// called when this representations visibility changes. We check if the LUT
  /// used to color this repr is being used by any other repr. If not, we turn off
  /// the scalar bar.
  void updateScalarBarVisibility(bool visible);
protected:
  
  enum AttributeTypes{ POINT_DATA = 0, 
                       CELL_DATA = 1, 
                       FIELD_DATA = 2, 
                       COORD_DATA = 3 };

  /// Call to select the coloring array. 
  /// The user indirectly calls this function by changing the ColorArrayName
  /// property
  void colorByArray(const char* array);

  /// Utility function that extracts the name of the color field:
  /// Returns <arrayname> from a string of  format:
  ///    "(coord|point|cell|field),<arrayname>(,[0-9]+)".
  QString GetArrayName(const QString&)const;
  /// Utility function that extracts the name of the color field:
  /// Returns POINT_DATA, CELL_DATA, FIELD_DATA or COORD_DATA from a 
  /// string of  format: "({coord|point|cell|field},)<arrayname>(,[0-9]+)".
  /// Returns -1 if none of the type can be found
  int GetArrayType(const QString&)const;
  /// Utility function that extracts the component of the color field:
  /// Returns -1 if there is no component. The string must be of format: 
  ///    "({coord|point|cell|field},)<arrayname>(,[0-9]+)".
  int GetArrayComponent(const QString&)const;
  /// Utility function that retrieve the number of components of an array
  int GetArrayNumberOfComponents(const QString&)const;

private:
  class pqInternal;
  pqInternal* Internal; 
  
  /*
  static void getColorArray(
    vtkPVDataSetAttributesInformation* attrInfo,
    vtkPVDataSetAttributesInformation* inAttrInfo,
    vtkPVArrayInformation*& arrayInfo);
  */
};

#endif
