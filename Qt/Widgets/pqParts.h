// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqParts_h
#define _pqParts_h

#include "QtWidgetsExport.h"

class vtkSMRenderModuleProxy;
class vtkSMDisplayProxy;
class vtkSMSourceProxy;

class QTWIDGETS_EXPORT pqPart
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
  static void Color(vtkSMDisplayProxy* Part, const char* fieldname, int fieldtype);
};

#endif //_pqParts_h

