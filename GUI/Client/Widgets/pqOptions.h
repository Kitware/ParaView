/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef __pqOptions_h
#define __pqOptions_h

#include <vtkPVOptions.h>

/// This is a pqServer implementation detail
/** \todo Make this private to pqServer */
class pqOptions : public vtkPVOptions
{
public:
  vtkTypeRevisionMacro(pqOptions, vtkPVOptions);
  static pqOptions *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  void SetClientMode(int Mode);
  void SetServerHost(const char* const HostName);
  void SetServerPort(int Port);

protected:
  pqOptions();
  virtual ~pqOptions();

  virtual void Initialize();
  virtual int PostProcess(int argc, const char * const *argv);

private:
  pqOptions(const pqOptions &);
  void operator=(const pqOptions &);
};

#endif //__pqOptions_h

