// -*- c++ -*-
/*=========================================================================

   Program: ParaView
   Module:    pqParts.h

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

#ifndef _pqParts_h
#define _pqParts_h

#include "pqWidgetsExport.h"
#include <QList>
#include <QString>

class vtkSMRenderModuleProxy;
class vtkSMDisplayProxy;
class vtkSMSourceProxy;
class vtkSMProxy;

class PQWIDGETS_EXPORT pqPart
{
public:
  /**
  Adds a part to be displayed.  The part should be a source or filter
  created with the proxy manager (retrived with GetProxyManager).  This
  method returns a vtkSMDisplayProxy, which can be used to modify how
  the part is displayed or to remove the part with RemovePart.  The
  vtkSMDisplayProxy is maintained internally, so the calling application
  does NOT have to delete it (it can be ignored).
  */
  static vtkSMDisplayProxy* Add(vtkSMRenderModuleProxy* rm, vtkSMSourceProxy* Part);

  /// color the part to its default color
  static void Color(vtkSMDisplayProxy* Part);

  /// color the part by a specific field, if fieldname is NULL, colors by actor color
  static void Color(vtkSMDisplayProxy* Part, const char* fieldname, int fieldtype,
    vtkSMProxy* lookupTable = 0);
  
  /// get the names of the arrays that a part may be colored by
  static QList<QString> GetColorFields(vtkSMDisplayProxy* Part);
  /// set the array to color the part by
  static void SetColorField(vtkSMDisplayProxy* Part, const QString& field);
  /// get the array the part is colored by
  /// if raw is true, it will not add (point) or (cell) but simply
  /// return the array name
  static QString GetColorField(vtkSMDisplayProxy* Part, bool raw=false);

};

#endif //_pqParts_h

