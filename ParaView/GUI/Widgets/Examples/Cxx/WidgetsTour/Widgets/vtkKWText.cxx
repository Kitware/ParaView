#include "vtkKWText.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWLabel.h"
#include "vtkKWIcon.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWTextItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win);
};

void vtkKWTextItem::Create(vtkKWWidget *parent, vtkKWWindow *win)
{
  vtkKWApplication *app = parent->GetApplication();

  const char *lorem_ipsum = "**Lorem ipsum** dolor sit ~~amet~~, consectetuer __adipiscing__ elit. Nunc felis. Nulla gravida. Aliquam erat volutpat. Mauris accumsan quam non sem. Sed commodo, magna quis bibendum lacinia, elit turpis iaculis augue, eget hendrerit elit dui vel elit.\n\nInteger ante eros, auctor eu, dapibus ac, ultricies vitae, lacus. Fusce accumsan mauris. Morbi felis. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos hymenaeos. Maecenas convallis imperdiet nunc.";

  // -----------------------------------------------------------------------

  // Create a text

  vtkKWText *text1 = vtkKWText::New();
  text1->SetParent(parent);
  text1->Create();
  text1->SetText(lorem_ipsum);
  text1->SetWidth(50);
  text1->SetHeight(12);
  text1->SetBalloonHelpString(
    "A text. The width and height are explicitly set to a given number of "
    "characters");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    text1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another text

  vtkKWText *text2 = vtkKWText::New();
  text2->SetParent(parent);
  text2->Create();
  text2->SetText(lorem_ipsum);
  text2->SetHeight(7);
  text2->SetWrapToChar();
  text2->ReadOnlyOn();
  text2->SetBalloonHelpString(
    "Another text, no explicit width is specified so that the text can expand "
    "with the window, word-wrapping is done at character boundary, and the "
    "the whole text area is read-only");

  app->Script(
    "pack %s -side top -anchor nw -expand n -fill x -padx 2 -pady 6", 
    text2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another text, with a label this time

  vtkKWTextWithScrollbars *text4 = vtkKWTextWithScrollbars::New();
  text4->SetParent(parent);
  text4->Create();
  text4->GetWidget()->SetText(lorem_ipsum);
  text4->GetWidget()->QuickFormattingOn();
  text4->GetWidget()->SetWidth(25);
  text4->SetBalloonHelpString(
    "This is a vtkKWTextWithScrollbars, i.e. a text associated to horizontal "
    "and vertical scrollbars that can be displayed or not. "
    "The QuickFormatting option is On, which enables basic formatting "
    "features to be used with simple tags like **, ~~, or __");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    text4->GetWidgetName());

  text1->Delete();
  text2->Delete();
  text4->Delete();
}

int vtkKWTextItem::GetType()
{
  return KWWidgetsTourItem::TypeCore;
}

KWWidgetsTourItem* vtkKWTextEntryPoint()
{
  return new vtkKWTextItem();
}
