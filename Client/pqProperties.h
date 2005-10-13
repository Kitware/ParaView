/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqProperties_h
#define _pqProperties_h

class vtkSMProxy;
class QString;

void pqSetProperty(vtkSMProxy* Proxy, const QString& Name, const double Value);
void pqSetProperty(vtkSMProxy* Proxy, const QString& Name, const int Value);
void pqSetProperty(vtkSMProxy* Proxy, const QString& Name, const QString& Value);

#endif // !_pqProperties_h

