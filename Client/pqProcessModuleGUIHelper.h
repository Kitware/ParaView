// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME pqProcessModuleGUIHelper
//
// .SECTION Description
// This class provides callbacks for the process module to handle GUI
// stuff.  The most important method is the RunGUIStart method that
// starts the user interaction.  Other callbacks are used to update the
// user on progress with executing filters and an application exit.
//

#ifndef __pqProcessModuleGUIHelper_h
#define __pqProcessModuleGUIHelper_h

#include <vtkProcessModuleGUIHelper.h>

class pqProcessModuleGUIHelper : public vtkProcessModuleGUIHelper
{
public:
  vtkTypeRevisionMacro(pqProcessModuleGUIHelper, vtkProcessModuleGUIHelper);
  static pqProcessModuleGUIHelper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Open a dialog box that allows the user to establish any connection
  // options that were not given on the command line.
  virtual int OpenConnectionDialog(int *);

  // Description:
  // Update user on progress of filters.
  virtual void SendPrepareProgress();
  virtual void SetLocalProgress(const char *filter, int progress);
  virtual void SendCleanupPendingProgress();

  // Description:
  // Exit the application.
  virtual void ExitApplication();

  // Description:
  // Start the main UI loop.
  virtual int RunGUIStart(int argc, char **argv, int numServerProcs, int myId);

protected:
  pqProcessModuleGUIHelper();
  virtual ~pqProcessModuleGUIHelper();

private:
  pqProcessModuleGUIHelper(const pqProcessModuleGUIHelper &);  // Not implemented
  void operator=(const pqProcessModuleGUIHelper &);  // Not implemented
};

#endif //__pqProcessModuleGUIHelper_h
