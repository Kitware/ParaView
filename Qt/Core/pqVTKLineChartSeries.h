/*=========================================================================

   Program: ParaView
   Module:    pqVTKLineChartSeries.h

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
#ifndef __pqVTKLineChartSeries_h
#define __pqVTKLineChartSeries_h


#include "pqLineChartSeries.h"
#include "pqCoreExport.h"

class pqVTKLineChartSeriesInternal;
class vtkDataArray;


class PQCORE_EXPORT pqVTKLineChartSeries : public pqLineChartSeries
{
public:
  pqVTKLineChartSeries(QObject *parent=0);
  virtual ~pqVTKLineChartSeries();

  /// \name pqLineChartSeries Methods.
  //@{
  virtual int getNumberOfSequences() const;
  virtual int getTotalNumberOfPoints() const;
  virtual SequenceType getSequenceType(int sequence) const;
  virtual int getNumberOfPoints(int sequence) const;
  virtual bool getPoint(int sequence, int index,
      pqChartCoordinate &coord) const;
  virtual void getErrorBounds(int sequence, int index, pqChartValue &upper,
      pqChartValue &lower) const;
  virtual void getErrorWidth(int sequence, pqChartValue &width) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

  /// \brief
  ///   Sets the data arrays for the model to use.
  /// \param xarray The x-axis data array.
  /// \param yarray The y-axis data array.
  void setDataArrays(vtkDataArray *xarray, vtkDataArray *yarray);

  /// \brief
  ///   Sets the mask arrays to be used for each axis. A mask array indicates
  ///   which indices are valid. If a mask array is 0, then all indices are
  ///   assumed to be valid.
  /// \param xmask The x-axis mask array.
  /// \param ymask The y-axis mask array.
  void setMaskArrays(vtkDataArray* xmask, vtkDataArray*  ymask);

private:
  pqVTKLineChartSeries(const pqVTKLineChartSeries&); // Not implemented.
  void operator=(const pqVTKLineChartSeries&); // Not implemented.

  pqVTKLineChartSeriesInternal *Internal; ///< Stores the series data.
};

#endif

