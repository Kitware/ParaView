/*=========================================================================

   Program: ParaView
   Module:    pqTestUtility.h

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

#ifndef _pqTestUtility_h
#define _pqTestUtility_h

#include "pqCoreExport.h"
#include <vtkIOStream.h>
#include <QObject>

class QString;
class pqEventPlayer;
class pqEventTranslator;
class pqProcessModuleGUIHelper;
class vtkRenderWindow;

/// Provides ParaView-specific functionality for regression testing
class PQCORE_EXPORT pqTestUtility :
  public QObject
{
  Q_OBJECT

public:
  pqTestUtility(pqProcessModuleGUIHelper&, QObject* parent = 0);
  ~pqTestUtility();

public:
  /// Handles ParaView-specific setup of a QtTesting event translator
  /// object (so QtTesting doesn't have any dependencies on ParaView/VTK)
  static void Setup(pqEventTranslator&);
  /// Handles ParaView-specific setup of a QtTesting event player object
  /// (so QtTesting doesn't have any dependencies on ParaView/VTK)
  static void Setup(pqEventPlayer&);
  /// Returns the absolute path to the PARAVIEW_DATA_ROOT in canonical form
  /// (slashes forward), or empty string
  static QString DataRoot();
  /// Saves the contents of a render window to a file for later use as a
  /// reference image
  static bool SaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File);
  /// Compares the contents of a render window to a reference image,
  /// returning true iff the two match within a given threshold
  static bool CompareImage(vtkRenderWindow* RenderWindow, 
                           const QString& ReferenceImage, 
                           double Threshold, 
                           ostream& Output, 
                           const QString& TempDirectory);

public slots:
  void runTests();

private slots:
  void testSucceeded();
  void testFailed();

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqTestUtility_h

