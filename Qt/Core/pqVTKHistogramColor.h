/*=========================================================================

   Program: ParaView
   Module:    pqVTKHistogramColor.h

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
#ifndef __pqVTKHistogramColor_h
#define __pqVTKHistogramColor_h

#include "pqHistogramColor.h"
#include "pqCoreExport.h"

class vtkSMProxy;
class pqHistogramModel;

// Implementation of pqHistogramColor which uses a vtkScalarsToColors
// object to determine the histogram bin colors.
class PQCORE_EXPORT pqVTKHistogramColor : public pqHistogramColor
{
public:
  pqVTKHistogramColor();
  virtual ~pqVTKHistogramColor();

  /// \brief
  ///   Called to get the color for a specified histogram bar.
  ///
  /// The default implementation returns a color from red to blue
  /// based on the index of the bar. Overload this method to change
  /// the default behavior.
  ///
  /// \param index The index of the bar.
  /// \param total The total number of bars in the histogram.
  /// \return
  ///   The color to paint the specified bar.
  virtual QColor getColor(int index, int total) const;

  /// We need a pointer to the histogram model that
  /// is being shown in the histogram. This is needed since getColor() does
  /// not tell us the scalar value for the bin.
  void setModel(pqHistogramModel* model);

  /// Set the lookup table.
  void setScalarsToColors(vtkSMProxy* lut);

  /// Set if the bin index is mapped to color
  /// rather than the bin value. Off by default.
  void setMapIndexToColor(bool b)
    { this->MapIndexToColor = b; }

private:
  pqVTKHistogramColor(const pqVTKHistogramColor&); // Not implemented.
  void operator=(const pqVTKHistogramColor&); // Not implemented.

  bool MapIndexToColor;
  class pqInternals;
  pqInternals* Internals;
};
#endif

