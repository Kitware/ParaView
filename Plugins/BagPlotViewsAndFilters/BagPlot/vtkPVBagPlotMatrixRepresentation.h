/*=========================================================================

   Program: ParaView
   Module:  vtkPVBagPlotMatrixRepresentation.h

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
/**
 * @class   vtkPVBagPlotMatrixRepresentation
 * @brief   vtkPVPlotMatrixRepresentation subclass for
 * BagPlot specific plot matrix representation.
 *
 * vtkPVBagPlotMatrixRepresentation uses vtkPVPlotMatrixRepresentation
 * to draw the plot matrix and extract the explained variance from the data.
*/

#ifndef vtkPVBagPlotMatrixRepresentation_h
#define vtkPVBagPlotMatrixRepresentation_h

#include <vtkBagPlotViewsAndFiltersBagPlotModule.h>
#include <vtkPVPlotMatrixRepresentation.h>

class VTKBAGPLOTVIEWSANDFILTERSBAGPLOT_EXPORT vtkPVBagPlotMatrixRepresentation
  : public vtkPVPlotMatrixRepresentation
{
public:
  static vtkPVBagPlotMatrixRepresentation* New();
  vtkTypeMacro(vtkPVBagPlotMatrixRepresentation, vtkPVPlotMatrixRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Recover the extracted explained variance from the data
   */
  vtkGetMacro(ExtractedExplainedVariance, double);

protected:
  vtkPVBagPlotMatrixRepresentation() = default;
  ~vtkPVBagPlotMatrixRepresentation() override = default;

  /**
   * Overridden to transfer explained variance from server to client when necessary
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVBagPlotMatrixRepresentation(const vtkPVBagPlotMatrixRepresentation&) = delete;
  void operator=(const vtkPVBagPlotMatrixRepresentation&) = delete;

  double ExtractedExplainedVariance = -1;
};

#endif
