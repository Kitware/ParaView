/*=========================================================================

  Module:    vtkKWOptionDataBase.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWOptionDataBase - an option database
// .SECTION Description
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWOptionDataBase_h
#define __vtkKWOptionDataBase_h

#include "vtkKWObject.h"

class vtkKWOptionDataBaseInternals;

class KWWidgets_EXPORT vtkKWOptionDataBase : public vtkKWObject
{
public:
  static vtkKWOptionDataBase* New();
  vtkTypeRevisionMacro(vtkKWOptionDataBase, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a new entry in the database.
  // Return the unique Id of the entry
  virtual int AddEntry(
    const char *pattern, const char *option, const char *value);
  virtual int AddEntryAsInt(
    const char *pattern, const char *option, int value);
  virtual int AddEntryAsInt3(
    const char *pattern, const char *option, int v0, int v1, int v2);
  virtual int AddEntryAsInt3(
    const char *pattern, const char *option, int value3[3]);
  virtual int AddEntryAsDouble(
    const char *pattern, const char *option, double value);
  virtual int AddEntryAsDouble3(
    const char *pattern, const char *option, double v0, double v1, double v2);
  virtual int AddEntryAsDouble3(
    const char *pattern, const char *option, double value3[3]);

  // Description:
  // Remove all entries.
  virtual void RemoveAllEntries();

  // Description:
  // Configure an object according to the options in the database.
  // Return the Id of the entry if found, -1 otherwise
  virtual void ConfigureObject(vtkKWObject *obj);

protected:
  vtkKWOptionDataBase();
  ~vtkKWOptionDataBase();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWOptionDataBaseInternals *Internals;
  //ETX

  virtual void ConfigureObject(vtkKWObject *obj, const char *pattern);

private:

  vtkKWOptionDataBase(const vtkKWOptionDataBase&); // Not implemented
  void operator=(const vtkKWOptionDataBase&); // Not implemented
};

#endif
