/*=========================================================================

   Program: ParaView
   Module:    pqMain.h

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

#ifndef __pqMain_h
#define __pqMain_h

#include "pqCoreExport.h"

class pqProcessModuleGUIHelper;
class pqOptions;
class vtkPVMain;
class QApplication;

/*! \brief
 * Helper class that wraps all of the boilerplate to create a ParaView client
 * into a small number of functions -- one, or three depending on whether the
 * client wants control of the Qt event loop, or not.
 */ 
class PQCORE_EXPORT pqMain
{
public:
  /// Call pqMain::preRun() in your client's main(), returning the result.
  /// NOTE: use preRun() only with associated Run() and postRun()
  static int preRun(QApplication& app, pqProcessModuleGUIHelper * helper,
    pqOptions * & options);

  /// Call pqMain::Run() in your client's main(), returning the result.
  /// This overloaded method is intended to be run stand-alone, i.e. 
  /// when not running preRun() nor postRun()
  static int Run(QApplication& app, pqProcessModuleGUIHelper* helper);

  /// Call pqMain::Run() in your client's main(), returning the result. This
  /// overload of Run is intended to be run after preRun(...)
  static int Run(pqOptions * options);

  /// call pqMain::postRun() for cleanup - this calls Delete() on pvmain ivar
  /// only. Since options, and helper are passed in to preRun() it is assumed
  /// the caller will free any memory allocated for those objects.
  /// NOTE: use postRun() only with associated preRun() Run()
  static void postRun();

protected:
  static vtkPVMain                * PVMain;
  static pqOptions                * PVOptions;
  static pqProcessModuleGUIHelper * PVHelper;
};

#endif // !__pqMain_h
