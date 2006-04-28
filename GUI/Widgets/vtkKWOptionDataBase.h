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
// This class can be used to store entries specifying how to automatically
// override the default settings/look&feel of any vtkKWWidget subclass at
// run-time. 
//
// For example, you may want all your vtkKWPushButton objects to use a blue
// background by default; this can be done by adding the following entry
// to the application's option database at startup:
//   myapplication->GetOptionDataBase()->AddEntry(
//      "vtkKWPushButton", "SetBackgroundColor", "0.2 0.2 0.8");
// From then on, anytime a vtkKWPushButton is created (using Create()), its 
// look&feel is configured and overriden automatically given the entries in
// the database (here, its BackgroundColor is set to a blue-ish color). 
//
// Collections of entries can be grouped inside a *theme*, subclass of
// vtkKWTheme. Check the Examples/Cxx/Theme for more details.
// Each vtkKWApplication object has a unique instance of a vtkKWOptionDataBase.
//
// Note that each entry is added as a pattern, a command, and a value:
//
// - the value can be empty if the command does not support any parameter, say:
//     AddEntry("vtkKWPushButton", "SetReliefToGroove", NULL)
//
// - the pattern can specify a constraint on the object context, i.e. require
//   that the command/value should only be applied if the object is of a 
//   specific class *and* has specific parents; this provides a way to 
//   configure widgets only when they are found inside other widgets:
//     AddEntry("vtkKWMessageDialog*vtkKWPushButton", "SetReliefToGroove",NULL)
//   => this entry will configure all vtkKWPushButton objects only if
//   they are found to be a child *or* a sub-child of a vtkKWMessageDialog.
//     AddEntry("vtkKWFrame.vtkKWPushButton", "SetReliefToGroove",NULL)
//   => this entry will configure all vtkKWPushButton objects only if
//   they are found to be an *immediate* child of a vtkKWFrame.
//   Of course, combinations can be used, say:
//     AddEntry("vtkKWMessageDialog*vtkKWFrame.vtkKWPushButton", ...
//
// - the pattern can specify a unique (terminal) slot suffix, that will be
//   used to configure a sub-object instead of the object itself. The 
//   sub-object is retrieved by calling Get'slot name' on the object.
//     AddEntry("vtkKWFrameWithLabel:CollapsibleFrame", "SetReliefToSolid", 0);
//   => this entry will configure the sub-object retrieved by calling
//   GetCollapsibleFrame on any vtkKWFrameWithLabel object, not the object
//   itself. This can be useful to customize specific part of a mega-widget.
//
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWTheme

#ifndef __vtkKWOptionDataBase_h
#define __vtkKWOptionDataBase_h

#include "vtkKWObject.h"

class vtkKWOptionDataBaseInternals;
class vtkKWWidget;

class KWWidgets_EXPORT vtkKWOptionDataBase : public vtkKWObject
{
public:
  static vtkKWOptionDataBase* New();
  vtkTypeRevisionMacro(vtkKWOptionDataBase, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  void DeepCopy(vtkKWOptionDataBase *p);

  // Description:
  // Add a new entry in the database.
  // Return the unique Id of the entry
  virtual int AddEntry(
    const char *pattern, const char *command, const char *value);
  virtual int AddEntryAsInt(
    const char *pattern, const char *command, int value);
  virtual int AddEntryAsInt3(
    const char *pattern, const char *command, int v0, int v1, int v2);
  virtual int AddEntryAsInt3(
    const char *pattern, const char *command, int value3[3]);
  virtual int AddEntryAsDouble(
    const char *pattern, const char *command, double value);
  virtual int AddEntryAsDouble3(
    const char *pattern, const char *command, double v0, double v1, double v2);
  virtual int AddEntryAsDouble3(
    const char *pattern, const char *command, double value3[3]);

  // Description:
  // Remove all entries.
  virtual void RemoveAllEntries();

  // Description:
  // Get number of entries.
  virtual int GetNumberOfEntries();

  // Description:
  // Configure a widget according to the options in the database.
  // Return the Id of the entry if found, -1 otherwise
  virtual void ConfigureWidget(vtkKWWidget *obj);

  // Description:
  // Convenience method to set all the known background color options to a 
  // specific color.
  virtual void AddBackgroundColorOptions(double r, double g, double b);
  virtual void AddBackgroundColorOptions(double rgb[3])
    { this->AddBackgroundColorOptions(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Convenience method to set all the known font options to a 
  // specific font.
  virtual void AddFontOptions(const char *font);

protected:
  vtkKWOptionDataBase();
  ~vtkKWOptionDataBase();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWOptionDataBaseInternals *Internals;
  //ETX

private:

  vtkKWOptionDataBase(const vtkKWOptionDataBase&); // Not implemented
  void operator=(const vtkKWOptionDataBase&); // Not implemented
};

#endif
