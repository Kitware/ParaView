/*=========================================================================

  Module:    vtkKWEntry.h,v

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWEntry - a single line text entry widget
// .SECTION Description
// A simple widget used for collecting keyboard input from the user. This
// widget provides support for single line input.

#ifndef __vtkKWEntry_h
#define __vtkKWEntry_h

#include "vtkKWCoreWidget.h"

class KWWidgets_EXPORT vtkKWEntry : public vtkKWCoreWidget
{
public:
  static vtkKWEntry* New();
  vtkTypeRevisionMacro(vtkKWEntry,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the value of the entry in a few different formats.
  // In the SetValue method with double, values are printed in printf's f or e
  // format, whichever is more compact for the given value and precision. 
  // The e format is used only when the exponent of the value is less than
  // -4 or greater than or equal to the precision argument (which can be
  // controlled using the second parameter of SetValue). Trailing zeros
  // are truncated, and the decimal point appears only if one or more digits
  // follow it.
  // IMPORTANT: whenever possible, use any of the GetValueAs...() methods
  // to retrieve the value if it is meant to be a number. This is faster
  // than calling GetValue() and converting the resulting string to a number.
  virtual void SetValue(const char *);
  virtual const char* GetValue();
  virtual void SetValueAsInt(int a);
  virtual int GetValueAsInt();
  virtual void SetValueAsFormattedDouble(double f, int size);
  virtual void SetValueAsDouble(double f);
  virtual double GetValueAsDouble();
  
  // Description:
  // The width is the number of charaters wide the entry box can fit.
  // To keep from changing behavior of the entry, the default
  // value is -1 wich means the width is not explicitly set and will default
  // to whatever value Tk is using (at this point, 20). Set it to 0
  // and the widget should pick a size just large enough to hold its text.
  virtual void SetWidth(int width);
  vtkGetMacro(Width, int);

  // Description:
  // Set/Get readonly flag. This flags makes the entry read only.
  virtual void SetReadOnly(int);
  vtkBooleanMacro(ReadOnly, int);
  vtkGetMacro(ReadOnly, int);

  // Description:
  // Set/Get password mode flag. If this flag is set, then the true contents
  // of the entry are not displayed in the window. Instead, each character in
  // the entry's value will be displayed as '*'. This is useful, for example, 
  // if the entry is to be used to enter a password. If characters in the entry
  // are selected and copied elsewhere, the information copied will be what is
  // displayed, not the true contents of the entry. 
  vtkBooleanMacro(PasswordMode, int);
  virtual void SetPasswordMode(int);
  virtual int GetPasswordMode();

  // Description:
  // Restrict the value to a given type (integer, double, or no restriction).
  //BTX
  enum
  {
    RestrictNone = 0,
    RestrictInteger,
    RestrictDouble
  };
  //ETX
  vtkGetMacro(RestrictValue, int);
  virtual void SetRestrictValue(int);
  virtual void SetRestrictValueToInteger();
  virtual void SetRestrictValueToDouble();
  virtual void SetRestrictValueToNone();

  // Description:
  // Set/Get the background color of the widget.
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget.
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual double* GetForegroundColor();
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the background color of the widget when it is disabled.
  virtual void GetDisabledBackgroundColor(double *r, double *g, double *b);
  virtual double* GetDisabledBackgroundColor();
  virtual void SetDisabledBackgroundColor(double r, double g, double b);
  virtual void SetDisabledBackgroundColor(double rgb[3])
    { this->SetDisabledBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget when it is disabled.
  virtual void GetDisabledForegroundColor(double *r, double *g, double *b);
  virtual double* GetDisabledForegroundColor();
  virtual void SetDisabledForegroundColor(double r, double g, double b);
  virtual void SetDisabledForegroundColor(double rgb[3])
    { this->SetDisabledForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the background color of the widget when it is read-only.
  virtual void GetReadOnlyBackgroundColor(double *r, double *g, double *b);
  virtual double* GetReadOnlyBackgroundColor();
  virtual void SetReadOnlyBackgroundColor(double r, double g, double b);
  virtual void SetReadOnlyBackgroundColor(double rgb[3])
    { this->SetReadOnlyBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
  // Description:
  // Set/Get the border width, a non-negative value indicating the width of
  // the 3-D border to draw around the outside of the widget (if such a border
  // is being drawn; the Relief option typically determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised();
  virtual void SetReliefToSunken();
  virtual void SetReliefToFlat();
  virtual void SetReliefToRidge();
  virtual void SetReliefToSolid();
  virtual void SetReliefToGroove();

  // Description:
  // Specifies the font to use when drawing text inside the widget. 
  // You can use predefined font names (e.g. 'system'), or you can specify
  // a set of font attributes with a platform-independent name, for example,
  // 'times 12 bold'. In this example, the font is specified with a three
  // element list: the first element is the font family, the second is the
  // size, the third is a list of style parameters (normal, bold, roman, 
  // italic, underline, overstrike). Example: 'times 12 {bold italic}'.
  // The Times, Courier and Helvetica font families are guaranteed to exist
  // and will be matched to the corresponding (closest) font on your system.
  // If you are familiar with the X font names specification, you can also
  // describe the font that way (say, '*times-medium-r-*-*-12*').
  virtual void SetFont(const char *font);
  virtual const char* GetFont();

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the return key is pressed, or the focus is lost.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - current value: const char*
  virtual void SetCommand(vtkObject *object, const char *method);

  // Description:
  // Events. The EntryValueChangedEvent is triggered when the widget value
  // is changed. It is similar in concept to the 'Command' callback but can be
  // used by multiple listeners/observers at a time.
  // Important: since there is no way to robustly find out when the user
  // is done inputing characters in the text entry, the EntryValueChangedEvent
  // event is also generated when <Return> is pressed, or the entry widget
  // is losing focus (i.e. the user clicked outside the text field).
  // The following parameters are also passed as client data:
  // - current value: const char*
  //BTX
  enum
  {
    EntryValueChangedEvent = 10000
  };
  //ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ValueCallback();

protected:
  vtkKWEntry();
  ~vtkKWEntry();
  
  // Description:
  // Create the widget.
  virtual void CreateWidget();

  int Width;
  int ReadOnly;
  int RestrictValue;

  char *Command;
  virtual void InvokeCommand(const char *value);

  virtual void Configure();

  // Description:
  // Update value restriction.
  virtual void UpdateValueRestriction();

private:

  char *InternalValueString;
  vtkGetStringMacro(InternalValueString);
  vtkSetStringMacro(InternalValueString);

  vtkKWEntry(const vtkKWEntry&); // Not implemented
  void operator=(const vtkKWEntry&); // Not Implemented
};

#endif
