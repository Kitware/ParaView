/*=========================================================================

   Program: ParaView
   Module:    pqChartOptionsHandler.h

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

#ifndef _pqChartOptionsHandler_h
#define _pqChartOptionsHandler_h

#include "pqComponentsExport.h"
#include "pqOptionsPage.h"

class pqChartOptionsEditor;
class pqView;


class PQCOMPONENTS_EXPORT pqChartOptionsHandler :
  public pqOptionsPageApplyHandler
{
public:
  enum ModifiedFlag
    {
    TitleModified = 0x00000001,
    TitleFontModified = 0x00000002,
    TitleColorModified = 0x00000004,
    TitleAlignmentModified = 0x00000008,
    ShowLegendModified = 0x00000010,
    LegendLocationModified = 0x00000020,
    LegendFlowModified = 0x00000030,
    ShowAxisModified = 0x00000080,
    ShowAxisGridModified = 0x00000100,
    AxisGridTypeModified = 0x00000200,
    AxisColorModified = 0x00000400,
    AxisGridColorModified = 0x00000800,
    ShowAxisLabelsModified = 0x00001000,
    AxisLabelFontModified = 0x00002000,
    AxisLabelColorModified = 0x00004000,
    AxisLabelNotationModified = 0x00008000,
    AxisLabelPrecisionModified = 0x00010000,
    AxisScaleModified = 0x00020000,
    AxisBehaviorModified = 0x00040000,
    AxisMinimumModified = 0x00080000,
    AxisMaximumModified = 0x00100000,
    AxisLabelsModified = 0x00200000,
    AxisTitleModified = 0x00400000,
    AxisTitleFontModified = 0x00800000,
    AxisTitleColorModified = 0x01000000,
    AxisTitleAlignmentModified = 0x02000000
    };

public:
  pqChartOptionsHandler();
  virtual ~pqChartOptionsHandler() {}

  pqChartOptionsEditor *getOptions() const {return this->Options;}
  void setOptions(pqChartOptionsEditor *options);

  pqView *getView() const {return this->View;}
  void setView(pqView *chart);

  void setModified(ModifiedFlag flag);

  virtual void applyChanges();
  virtual void resetChanges();

private:
  void initializeOptions();

private:
  unsigned int ModifiedData;
  pqChartOptionsEditor *Options;
  pqView *View;
};

#endif
