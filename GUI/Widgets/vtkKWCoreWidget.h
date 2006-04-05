/*=========================================================================

  Module:    vtkKWCoreWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWCoreWidget - a core widget.
// .SECTION Description
// A superclass for all core widgets, i.e. C++ wrappers around simple
// Tk widgets.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWCoreWidget_h
#define __vtkKWCoreWidget_h

#include "vtkKWWidget.h"
#include "vtkKWTkOptions.h" // For option constants

class KWWidgets_EXPORT vtkKWCoreWidget : public vtkKWWidget
{
public:
  static vtkKWCoreWidget* New();
  vtkTypeRevisionMacro(vtkKWCoreWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set/Get the -state option to "normal" (1) or "disabled" (0) or "readonly"
  // (2, if supported).
  // Valid constants can be found in vtkKWTkOptions::StateType.
  // This should not be used directly, this is done by 
  // SetEnabled()/UpdateEnableState(). 
  // TODO: should be in protected:
  virtual void SetState(int);
  virtual int GetState();
  virtual void SetStateToDisabled() 
    { this->SetState(vtkKWTkOptions::StateDisabled); };
  virtual void SetStateToNormal() 
    { this->SetState(vtkKWTkOptions::StateNormal); };
  virtual void SetStateToReadOnly() 
    { this->SetState(vtkKWTkOptions::StateReadOnly); };

  // Description:
  // Arranges for window to be displayed above all of its siblings in the
  // stacking order.
  virtual void Raise();

  // Description:
  // Set/Get a Tk configuration option (ex: "-bg").
  // Make *sure* you check the class (and subclasses) API for a
  // C++ method that would already act as a front-end to the Tk option.
  // For example, the SetBackgroundColor() method should be used instead of
  // accessing the -bg Tk option. 
  // Note that SetConfigurationOption will enclose the value inside
  // curly braces {} as a convenience.
  // SetConfigurationOption returns 1 on success, 0 otherwise.
  virtual int SetConfigurationOption(const char* option, const char *value);
  virtual int HasConfigurationOption(const char* option);
  virtual const char* GetConfigurationOption(const char* option);
  virtual int GetConfigurationOptionAsInt(const char* option);
  virtual int SetConfigurationOptionAsInt(const char* option, int value);
  virtual double GetConfigurationOptionAsDouble(const char* option);
  virtual int SetConfigurationOptionAsDouble(const char* option, double value);
  virtual void GetConfigurationOptionAsColor(
    const char* option, double *r, double *g, double *b);
  virtual double* GetConfigurationOptionAsColor(const char* option);
  virtual void SetConfigurationOptionAsColor(
    const char* option, double r, double g, double b);
  virtual void SetConfigurationOptionAsColor(const char* option, double rgb[3])
    { this->SetConfigurationOptionAsColor(option, rgb[0], rgb[1], rgb[2]); };

protected:
  vtkKWCoreWidget() {};
  ~vtkKWCoreWidget() {};

  // Description:
  // Get the Tk string type of the widget.
  virtual const char* GetType();
  
  // Description:
  // Convert a Tcl string (stored internally as UTF-8/Unicode) to another
  // internal format (given the widget's application CharacterEncoding), 
  // and vice-versa.
  // The 'source' string is the source to convert.
  // It returns a pointer to a static buffer where the converted string
  // can be found (so be quick about it).
  // The 'options' can be set to perform some replacements/escaping.
  // ConvertStringEscapeInterpretable will attempt to escape all characters
  // that can be interpreted (when found between a pair of quotes for
  // example): $ [ ] "
  //BTX
  enum
  {
    ConvertStringEscapeCurlyBraces   = 1,
    ConvertStringEscapeInterpretable = 2
  };
  const char* ConvertTclStringToInternalString(
    const char *source, int options = 0);
  const char* ConvertInternalStringToTclString(
    const char *source, int options = 0);
  //ETX

  // Description:
  // Set/Get a textual Tk configuration option (ex: "-bg").
  // This should be used instead of SetConfigurationOption as it performs
  // various characted encoding and escaping tricks.
  // The characted encoding used in the string will be retrieved by querying
  // the widget's application CharacterEncoding ivar. Conversion from that
  // encoding to Tk internal encoding will be performed automatically.
  virtual void SetTextOption(const char *option, const char *value);
  virtual const char* GetTextOption(const char *option);

private:
  vtkKWCoreWidget(const vtkKWCoreWidget&); // Not implemented
  void operator=(const vtkKWCoreWidget&); // Not implemented
};


#endif



