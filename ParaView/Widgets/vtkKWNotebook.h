/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWNotebook.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWNotebook - a tabbed notebook of UI pages
// .SECTION Description
// The notebook represents a tabbed notebook component where you can
// add or remove pages.

#ifndef __vtkKWNotebook_h
#define __vtkKWNotebook_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWIcon;
class vtkKWLabel;
class vtkKWImageLabel;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWNotebook : public vtkKWWidget
{
public:

  static vtkKWNotebook* New();
  vtkTypeRevisionMacro(vtkKWNotebook,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Add a new page to the notebook. By setting balloon string, the page will
  // display a balloon help. An optional icon can also be specified and will 
  // be displayed on the left side of the tab label (all icons can be hidden
  // later on using the SetShowIcons() method). Finally, an optional tag
  // can be provided and will be associated to the page (see SetPageTag()) ; this
  // tag will default to 0 otherwise.
  // Return a unique positive ID corresponding to that page, or < 0 on error.
  int AddPage(const char *title, const char* balloon, vtkKWIcon *icon, int tag);
  int AddPage(const char *title, const char* balloon, vtkKWIcon *icon);
  int AddPage(const char *title, const char* balloon);
  int AddPage(const char *title);

  // Description:
  // Return the number of pages in the notebook.
  unsigned int GetNumberOfPages();
  
  // Description:
  // Set/Get a page tag. A tag (int) can be associated to a page. This provides
  // a way to group pages. The default tag, if not provided, is 0.
  void SetPageTag(int id, int tag);
  void SetPageTag(const char *title, int tag);
  int GetPageTag(int id);
  int GetPageTag(const char *title);

  // Description:
  // Raise the specified page to be on the top (i.e. the one selected).
  // If a tag is provided, the page which title and tag match will be raised.
  // This provide a way to search within a group of pages, since several pages
  // could have the same title.
  void Raise(int id);
  void Raise(const char *title);
  void Raise(const char *title, int tag);

  // Description:
  // Get the vtkKWWidget for the frame of the specified page (Tab).
  // The UI components should be inserted into these frames.
  // If a tag is provided, the page which title and tag match will be accessed.
  // This provide a way to search within a group of pages, since several pages
  // could have the same title.
  // Return NULL on error.
  vtkKWWidget *GetFrame(int id);
  vtkKWWidget *GetFrame(const char *title);
  vtkKWWidget *GetFrame(const char *title, int tag);

  // Description:
  // Remove a page from the notebook.
  // If the currently selected page is removed, it is unselected first and
  // the first visible tab is selected.
  // Return 1 on success, 0 on error.
  int RemovePage(int id);
  int RemovePage(const char *title);

  // Description:
  // Convenience methode to all pages matching a tag.
  void RemovePagesMatchingTag(int tag);
  
  // Description:
  // Show/hide a page tab (i.e. Set/Get the page visibility). Showing a page 
  // tab does not raise a page, it just makes the page selectable by displaying
  // its tab. A hidden page tab is not displayed in the tabs: the corresponding
  // page can not be selected. If the currently selected page is about to be
  // hidden, it is unselected first and the first visible tab becomes selected.
  void ShowPage(int id);
  void ShowPage(const char *title);
  void HidePage(int id);
  void HidePage(const char *title);
  void SetPageVisibility(int id, int flag);
  void SetPageVisibility(const char *title, int flag);
  int GetPageVisibility(int id);
  int GetPageVisibility(const char *title);

  // Description:
  // Return the number of visible pages in the notebook.
  unsigned int GetNumberOfVisiblePages();

  // Description:
  // Convenience methods provided to show/hide all page tabs matching or not
  // matching a given tag.
  void HidePagesMatchingTag(int tag);
  void ShowPagesMatchingTag(int tag);
  void HidePagesNotMatchingTag(int tag);
  void ShowPagesNotMatchingTag(int tag);

  // Description:
  // Automatically bring up page tabs so that all page tabs that have the same 
  // tag are always shown.
  virtual void SetShowAllPagesWithSameTag(int);
  vtkGetMacro(ShowAllPagesWithSameTag, int);
  vtkBooleanMacro(ShowAllPagesWithSameTag, int);
  
  // Description:
  // Automatically hide page tabs so that only those page tabs that have the 
  // same tag as the currently selected page are shown.
  virtual void SetShowOnlyPagesWithSameTag(int);
  vtkGetMacro(ShowOnlyPagesWithSameTag, int);
  vtkBooleanMacro(ShowOnlyPagesWithSameTag, int);
  
  // Description:
  // Pin/unpin a page tab. A pinned page tab can not be hidden.
  void PinPage(int id);
  void PinPage(const char *title);
  void UnpinPage(int id);
  void UnpinPage(const char *title);
  
  // Description:
  // Allow pages to be pinned.
  virtual void SetPagesCanBePinned(int);
  vtkGetMacro(PagesCanBePinned, int);
  vtkBooleanMacro(PagesCanBePinned, int);
  
  // Description:
  // The notebook will automatically resize itself to fit its
  // contents. This can lead to a lot of resizing. So you can
  // specify a minimum width and height for the notebook. This
  // can be used to significantly reduce or eliminate the resizing
  // of the notebook.
  virtual void SetMinimumWidth(int);
  vtkGetMacro(MinimumWidth,int);
  virtual void SetMinimumHeight(int);
  vtkGetMacro(MinimumHeight,int);

  // Description:
  // Normally, the tab frame is not shown when there is only
  // one page. Turn this on to override that behaviour.
  virtual void SetAlwaysShowTabs(int);
  vtkGetMacro(AlwaysShowTabs, int);
  vtkBooleanMacro(AlwaysShowTabs, int);
  
  // Description:
  // Show/hide all tab icons (if any).
  virtual void SetShowIcons(int);
  vtkGetMacro(ShowIcons, int);
  vtkBooleanMacro(ShowIcons, int);
  
  // Description:
  // Some callback routines.
  void ScheduleResize();
  void Resize();
  void PinPageToggle(int id);
  
protected:
  vtkKWNotebook();
  ~vtkKWNotebook();

  int MinimumWidth;
  int MinimumHeight;
  int AlwaysShowTabs;
  int ShowIcons;
  int ShowAllPagesWithSameTag;
  int ShowOnlyPagesWithSameTag;
  int PagesCanBePinned;

  vtkKWWidget *TabsFrame;
  vtkKWWidget *Body;
  vtkKWWidget *Mask;

  //BTX

  // A notebook page

  class Page
  {
  public:
    void Delete();
    int Id;
    char *Title;
    vtkKWFrame      *Frame;
    vtkKWFrame      *TabFrame;
    vtkKWLabel      *Label;
    vtkKWImageLabel *ImageLabel;
    vtkKWIcon       *Icon;
    int             Visibility;
    int             Pinned;
    int             Tag;
  };

  // The pages container and its iterator

  typedef vtkLinkedList<Page*> PagesContainer;
  typedef vtkLinkedListIterator<Page*> PagesContainerIterator;
  PagesContainer *Pages;

  // Return a pointer to a page.
  // Note that this method accepts both a page ID or a page title (in the later 
  // case, if two tabs have the same title, the first one will be chosen).

  Page* GetPage(int id);
  Page* GetPage(const char *title);
  Page* GetPage(const char *title, int tag);

  // Get the first visible page

  Page* GetFirstVisiblePage();
  
  // Raise, Lower, Remove, Show, Hide, Pin, Unpin, Tag a specific page

  void SetPageTag(Page*, int tag);
  void RaisePage(Page*);
  void ShowPageTabAsLow(Page*);
  void LowerPage(Page*);
  int RemovePage(Page*);
  void ShowPage(Page*);
  void HidePage(Page*);
  void PinPage(Page*);
  void UnpinPage(Page*);
  int GetPageVisibility(Page*);
  int GetPageTag(Page*);

  // Update the tab frame color of a page given a selection status

  void UpdatePageTabBackgroundColor(Page*, int selected);

  //ETX

  int IdCounter;
  int CurrentId;
  int Expanding;

  // Get the number of pages in the notebook (and the number of visible pages).

  int AreTabsVisible();

  // Update the position of the body and mask elements

  void UpdateBodyPosition();
  void UpdateMaskPosition();

private:
  vtkKWNotebook(const vtkKWNotebook&); // Not implemented
  void operator=(const vtkKWNotebook&); // Not implemented
};

#endif
