/*=========================================================================

  Module:    vtkKWMessage.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMessage - message widget
// .SECTION Description
// A simple widget that represents a message (a long unit of text). A message
// is a widget that displays a textual string. A message widget (vtkKWMessage)
// has several special features compared to a label (vtkKWLabel). 
// First, It breaks up its string into lines in order to produce a given
// aspect ratio for the window. The line breaks are chosen at word boundaries
// wherever possible (if not even a single word would fit on a line, then the
// word will be split across lines). Newline characters in the string will
// force line breaks; they can be used, for example, to leave blank lines in
// the display. 
// The second feature of a message widget is justification. The text may be
// displayed left-justified (each line starts at the left side of
// the window), centered on a line-by-line basis, or right-justified (each
// line ends at the right side of the window). 
// For short text, or to set an image as a label, see vtkKWLabel.
// .SECTION See Also
// vtkKWLabel

#ifndef __vtkKWMessage_h
#define __vtkKWMessage_h

#include "vtkKWCoreWidget.h"

class KWWidgets_EXPORT vtkKWMessage : public vtkKWCoreWidget
{
public:
  static vtkKWMessage* New();
  vtkTypeRevisionMacro(vtkKWMessage,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();
  
  // Description:
  // Set the text on the message.
  virtual void SetText(const char*);
  vtkGetStringMacro(Text);

  // Description:
  // Set/Get width of the message (in pixels).
  // Specifies the length of lines in the window. If this option has a value
  // greater than zero then the AspectRatio setting is ignored and the width
  // setting determines the line length. If this option has a value less than
  // or equal to zero, then the AspectRatio determines the line length.
  virtual void SetWidth(int);
  virtual int GetWidth();

  // Description:
  // Set/Get aspect ratio of the message.
  // Specifies a non-negative integer value indicating desired aspect ratio
  // for the text. The aspect ratio is specified as 100*width/height. 
  // 100 means the text should be as wide as it is tall, 200 means the text
  // should be twice as wide as it is tall, 50 means the text should be twice
  // as tall as it is wide, and so on. It is used to choose line length for
  // the text if the Width setting isn't specified.
  virtual void SetAspectRatio(int);
  virtual int GetAspectRatio();

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
  // Set/Get the justification mode.
  // When there are multiple lines of text displayed in a widget, this option
  // determines how the lines line up with each other.
  // This option works together with the Anchor, AspectRatio, and Width 
  // settings to provide a variety of arrangements of the text within the
  // window. The AspectRatio and Width settings determine the amount of screen
  // space needed to display the text. The Anchor setting determines where 
  // this rectangular area is displayed within the widget's window, and the
  // Justification setting determines how each line is displayed within that
  // rectangular region. For example, suppose Anchor is East and Justification
  // is Left, and that the message window is much larger than needed for the
  // text. The text will displayed so that the left edges of all the lines
  // line up and the right edge of the longest line is on the right side of
  // the window; the entire text block will be centered in the vertical span
  // of the window. Note that the superclass's PadX and PadY setting can be
  // use to provide additional margins.
  // Valid constants can be found in vtkKWOptions::JustificationType.
  virtual void SetJustification(int);
  virtual int GetJustification();
  virtual void SetJustificationToLeft();
  virtual void SetJustificationToCenter();
  virtual void SetJustificationToRight();

  // Description:
  // Set/Get the anchoring.
  // Specifies how the information in a widget (e.g. text) is to
  // be displayed in the widget. See example in the Justification method doc.
  // Valid constants can be found in vtkKWOptions::AnchorType.
  virtual void SetAnchor(int);
  virtual int GetAnchor();
  virtual void SetAnchorToNorth();
  virtual void SetAnchorToNorthEast();
  virtual void SetAnchorToEast();
  virtual void SetAnchorToSouthEast();
  virtual void SetAnchorToSouth();
  virtual void SetAnchorToSouthWest();
  virtual void SetAnchorToWest();
  virtual void SetAnchorToNorthWest();
  virtual void SetAnchorToCenter();

protected:
  vtkKWMessage();
  ~vtkKWMessage();

  virtual void UpdateText();

private:
  char* Text;

  vtkKWMessage(const vtkKWMessage&); // Not implemented
  void operator=(const vtkKWMessage&); // Not implemented
};

#endif
