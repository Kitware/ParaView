/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectNaming.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqObjectNaming_h
#define _pqObjectNaming_h

#include "QtTestingExport.h"

class QObject;
class QString;
class vtkRenderWindow;

/// Provides functionality to ensuring that Qt objects can be uniquely identified for recording and playback of regression tests
class QTTESTING_EXPORT pqObjectNaming
{
public:
  /// Recursively validates that every child of the given QObject is named correctly for testing
  static bool Validate(QObject& Parent);

  /// Returns a unique identifier for the given object that can be serialized for later regression test playback
  static const QString GetName(QObject& Object);
  /// Given a unique identifier returned by GetName(), returns the corresponding object, or NULL
  static QObject* GetObject(const QString& Name);
  
private:
  static bool Validate(QObject& Parent, const QString& Path);
};

#endif // !_pqObjectNaming_h

