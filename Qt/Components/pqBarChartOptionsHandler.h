/*=========================================================================

   Program: ParaView
   Module:    pqBarChartOptionsHandler.h

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

#ifndef _pqBarChartOptionsHandler_h
#define _pqBarChartOptionsHandler_h

#include "pqComponentsExport.h"
#include "pqOptionsPage.h"

class pqBarChartOptionsEditor;
class pqView;


class PQCOMPONENTS_EXPORT pqBarChartOptionsHandler :
  public pqOptionsPageApplyHandler
{
public:
  enum ModifiedFlag
    {
    HelpFormatModified = 0x00000001,
    OutlineStyleModified = 0x00000002,
    GroupFractionModified = 0x00000004,
    WidthFractionModified = 0x00000008
    };

public:
  pqBarChartOptionsHandler();
  virtual ~pqBarChartOptionsHandler() {}

  pqBarChartOptionsEditor *getOptions() const {return this->Options;}
  void setOptions(pqBarChartOptionsEditor *options);

  pqView *getView() const {return this->View;}
  void setView(pqView *chart);

  void setModified(ModifiedFlag flag);

  virtual void applyChanges();
  virtual void resetChanges();

private:
  void initializeOptions();

private:
  unsigned int ModifiedData;
  pqBarChartOptionsEditor *Options;
  pqView *View;
};

#endif
