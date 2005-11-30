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

class KWWIDGETS_EXPORT vtkKWCoreWidget : public vtkKWWidget
{
public:
  static vtkKWCoreWidget* New();
  vtkTypeRevisionMacro(vtkKWCoreWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Convenience method to Set/Get the current background and foreground colors
  // of the widget
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual double* GetForegroundColor();
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
  // Description:
  // Set/get the border width, a non-negative value
  // indicating the width of the 3-D border to draw around the outside of
  // the widget (if such a border is being drawn; the Relief option typically
  // determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWTkOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised() 
    { this->SetRelief(vtkKWTkOptions::ReliefRaised); };
  virtual void SetReliefToSunken() 
    { this->SetRelief(vtkKWTkOptions::ReliefSunken); };
  virtual void SetReliefToFlat() 
    { this->SetRelief(vtkKWTkOptions::ReliefFlat); };
  virtual void SetReliefToRidge() 
    { this->SetRelief(vtkKWTkOptions::ReliefRidge); };
  virtual void SetReliefToSolid() 
    { this->SetRelief(vtkKWTkOptions::ReliefSolid); };
  virtual void SetReliefToGroove() 
    { this->SetRelief(vtkKWTkOptions::ReliefGroove); };

  // Description:
  // Set/Get the padding that will be applied around each widget (in pixels).
  // Specifies a non-negative value indicating how much extra space to request
  // for the widget in the X and Y-direction. When computing how large a
  // window it needs, the widget will add this amount to the width it would
  // normally need (as determined by the width of the things displayed
  // in the widget); if the geometry manager can satisfy this request, the 
  // widget will end up with extra internal space around what it displays 
  // inside. 
  virtual void SetPadX(int);
  virtual int GetPadX();
  virtual void SetPadY(int);
  virtual int GetPadY();

  // Description:
  // Set/Get a Tk configuration option (ex: "-bg") 
  // Please make sure you check the class (and subclasses) API for
  // a C++ method acting as a front-end for the corresponding Tk option.
  // For example, the SetBackgroundColor() method should be used to set the 
  // corresponding -bg Tk option. 
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

  // Description:
  // Convenience method to Set/Get the -state option to "normal" (1) or
  // "disabled" (0) or "readonly" (2, if supported).
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
  // Set/Get a textual Tk configuration option (ex: "-bg") 
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



