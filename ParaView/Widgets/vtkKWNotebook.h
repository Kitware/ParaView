/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
class vtkKWMenu;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
template<class DataType> class vtkVector;
template<class DataType> class vtkVectorIterator;
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
  // Accessors
  char* GetPageTitle(int id);

  // Description:
  // Return the number of pages in the notebook.
  unsigned int GetNumberOfPages();
  
  // Description:
  // Set/Get a page tag. A tag (int) can be associated to a page (given the page
  // id). This provides a way to group pages. The default tag, if not provided,
  // is 0. 
  // If a page title is provided instead of a page id, the first page matching
  // that title is considered.
  void SetPageTag(int id, int tag);
  void SetPageTag(const char *title, int tag);
  int GetPageTag(int id);
  int GetPageTag(const char *title);

  // Description:
  // Raise the specified page to be on the top (i.e. the one selected).
  // If a page title is provided instead of a page id, the first page matching
  // that title is considered. In the same way, if a tag is provided with the 
  // title, the page which title *and* tag match is considered.
  // GetRaisedPageId() returns the id of the page raised at the moment, -1 if
  // none is raised.
  // RaiseFirstPageMatchingTag() is a convenience method use to raise the first
  // page matching a given tag.
  void Raise(int id);
  void Raise(const char *title);
  void Raise(const char *title, int tag);
  int GetRaisedPageId();
  void RaiseFirstPageMatchingTag(int tag);
  
  // Description:
  // Get the vtkKWWidget corresponding to the frame of the specified page (Tab).
  // This is where the UI components should be inserted.
  // If a page title is provided instead of a page id, the first page matching
  // that title is considered. In the same way, if a tag is provided with the 
  // title, the page which title *and* tag match is considered.
  // Return NULL on error.
  vtkKWWidget *GetFrame(int id);
  vtkKWWidget *GetFrame(const char *title);
  vtkKWWidget *GetFrame(const char *title, int tag);

  // Description:
  // Remove a page from the notebook.
  // If a page title is provided instead of a page id, the first page matching
  // that title is considered.
  // If the currently selected page is removed, it is unselected first and
  // the first visible tab (if any) becomes selected instead.
  // Return 1 on success, 0 on error.
  int RemovePage(int id);
  int RemovePage(const char *title);

  // Description:
  // Convenience method to remove all pages matching a tag.
  void RemovePagesMatchingTag(int tag);
  
  // Description:
  // Show/hide a page tab (i.e. Set/Get the page visibility). Showing a page 
  // tab does not raise the page, it just makes the page selectable by displaying
  // its tab. A hidden page tab is not displayed in the tabs: the corresponding
  // page can not be selected. 
  // If a page title is provided instead of a page id, the first page matching
  // that title is considered.
  // If the currently selected page is hidden, it is unselected first and
  // the first visible tab (if any) becomes selected instead.
  void ShowPage(int id);
  void ShowPage(const char *title);
  void HidePage(int id);
  void HidePage(const char *title);
  void SetPageVisibility(int id, int flag);
  void SetPageVisibility(const char *title, int flag);
  int  GetPageVisibility(int id);
  int  GetPageVisibility(const char *title);
  void TogglePageVisibility(int id);
  void TogglePageVisibility(const char *title);
  int  CanBeHidden(int id);
  int  CanBeHidden(const char *title);
  void HideAllPages();

  // Description:
  // Return the number of visible pages in the notebook.
  unsigned int GetNumberOfVisiblePages();

  // Description:
  // Get the n-th visible page id (starting at index 0, i.e. the first visible 
  // page is at index 0, although it does not necessary reflects the way
  // the page tab are packed/ordered in the tab row).
  // Return -1 if the index is out of the range, or if there is no visible
  // page for that index.
  int GetVisiblePageId(int idx);
  
  // Description:
  // Convenience methods provided to show/hide all page tabs matching or not
  // matching a given tag. ShowPagesMatchingTagReverse processes pages starting
  // from the last one.
  void HidePagesMatchingTag(int tag);
  void ShowPagesMatchingTag(int tag);
  void ShowPagesMatchingTagReverse(int tag);
  void HidePagesNotMatchingTag(int tag);
  void ShowPagesNotMatchingTag(int tag);

  // Description:
  // Make the notebook automatically bring up page tabs with the same tag 
  // (i.e. all page tabs that have the same tag are always shown).
  virtual void SetShowAllPagesWithSameTag(int);
  vtkGetMacro(ShowAllPagesWithSameTag, int);
  vtkBooleanMacro(ShowAllPagesWithSameTag, int);
  
  // Description:
  // Make the notebook automatically show only those page tabs that have the 
  // same tag as the currently selected page (i.e. once a page tab has been
  // selected, all pages not sharing the same tag are hidden).
  virtual void SetShowOnlyPagesWithSameTag(int);
  vtkGetMacro(ShowOnlyPagesWithSameTag, int);
  vtkBooleanMacro(ShowOnlyPagesWithSameTag, int);
  
  // Description:
  // Make the notebook automatically maintain a list of most recently used
  // page. The size of this list can be set (defaults to 4). Once it is full,
  // any new shown page will make the least recent page hidden.
  // It is suggested that ShowAllPagesWithSameTag and ShowOnlyPagesWithSameTag
  // shoud be Off for this feature to work properly.
  virtual void SetShowOnlyMostRecentPages(int);
  vtkGetMacro(ShowOnlyMostRecentPages, int);
  vtkBooleanMacro(ShowOnlyMostRecentPages, int);
  vtkSetMacro(NumberOfMostRecentPages, int);
  vtkGetMacro(NumberOfMostRecentPages, int);

  // Description:
  // Get the n-th most recent page id. Most recent pages indexes start at 0 
  // (i.e. the most recent page is at index 0).
  // Return -1 if the index is out of the range, or if there is no most
  // recent page for that index.
  int GetMostRecentPageId(int idx);

  // Description:
  // Pin/unpin a page tab. A pinned page tab can not be hidden.
  // If a page title is provided instead of a page id, the first page matching
  // that title is considered.
  void PinPage(int id);
  void PinPage(const char *title);
  void UnpinPage(int id);
  void UnpinPage(const char *title);
  void TogglePagePinned(int id);
  void TogglePagePinned(const char *title);
  int  GetPagePinned(int id);
  int  GetPagePinned(const char *title);
  
  // Description:
  // Allow pages to be pinned.
  virtual void SetPagesCanBePinned(int);
  vtkGetMacro(PagesCanBePinned, int);
  vtkBooleanMacro(PagesCanBePinned, int);

  // Description:
  // Return the number of pinned pages in the notebook.
  unsigned int GetNumberOfPinnedPages();

  // Description:
  // Get the n-th pinned page id (starting at index 0, i.e. the first pinned page
  // is at index 0).
  // Return -1 if the index is out of the range, or if there is no pinned
  // page for that index.
  int GetPinnedPageId(int idx);
  
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
  // Enable the page tab context menu.
  vtkSetMacro(EnablePageTabContextMenu, int);
  vtkGetMacro(EnablePageTabContextMenu, int);
  vtkBooleanMacro(EnablePageTabContextMenu, int);
  
  // Description:
  // Get the id of the visible page which tab contains a given pair of screen
  // coordinates (-1 if not found).
  int GetPageIdContainingCoordinatesInTab(int x, int y);

  // Description:
  // Some callback routines.
  void ScheduleResize();
  void Resize();
  void PageTabContextMenuCallback(int id, int x, int y);
  
protected:
  vtkKWNotebook();
  ~vtkKWNotebook();

  int MinimumWidth;
  int MinimumHeight;
  int AlwaysShowTabs;
  int ShowIcons;
  int ShowAllPagesWithSameTag;
  int ShowOnlyPagesWithSameTag;
  int ShowOnlyMostRecentPages;
  int NumberOfMostRecentPages;
  int PagesCanBePinned;
  int EnablePageTabContextMenu;

  vtkKWWidget *TabsFrame;
  vtkKWWidget *Body;
  vtkKWWidget *Mask;
  vtkKWMenu   *TabPopupMenu;

  //BTX

  // A notebook page

  class Page
  {
  public:
    void Delete();
    void SetEnabled(int);

    int             Id;
    int             Visibility;
    int             Pinned;
    int             Tag;
    char            *Title;
    vtkKWFrame      *Frame;
    vtkKWFrame      *TabFrame;
    vtkKWLabel      *Label;
    vtkKWLabel      *ImageLabel;
    vtkKWIcon       *Icon;
  };

  // The pages container and its iterator

  typedef vtkLinkedList<Page*> PagesContainer;
  typedef vtkLinkedListIterator<Page*> PagesContainerIterator;
  PagesContainer *Pages;
  PagesContainer *MostRecentPages;

  // Return a pointer to a page.
  // If a page title is provided instead of a page id, the first page matching
  // that title is considered. In the same way, if a tag is provided with the 
  // title, the page which title *and* tag match is considered.

  Page* GetPage(int id);
  Page* GetPage(const char *title);
  Page* GetPage(const char *title, int tag);

  // Get the first visible page
  // Get the first page matching a tag
  // Get the first packed page not matching a tag

  Page* GetFirstVisiblePage();
  Page* GetFirstPageMatchingTag(int tag);
  Page* GetFirstPackedPageNotMatchingTag(int tag);
  
  // Raise, Lower, Remove, Show, Hide, Pin, Unpin, Tag a specific page

  void SetPageTag(Page*, int tag);
  void RaisePage(Page*);
  void ShowPageTab(Page*);
  void ShowPageTabAsLow(Page*);
  void LowerPage(Page*);
  int  RemovePage(Page*);
  void ShowPage(Page*);
  void HidePage(Page*);
  void PinPage(Page*);
  void UnpinPage(Page*);
  void TogglePagePinned(Page*);
  int  GetPageVisibility(Page*);
  void TogglePageVisibility(Page*);
  int  CanBeHidden(Page*);
  int  GetPageTag(Page*);
  int  GetPagePinned(Page*);
  char* GetPageTitle(Page*);
  void BindPage(Page*);
  void UnBindPage(Page*);

  int AddToMostRecentPages(Page*);
  int RemoveFromMostRecentPages(Page*);
  int PutOnTopOfMostRecentPages(Page*);

  // Update the tab frame color of a page given a selection status

  void UpdatePageTabBackgroundColor(Page*, int selected);

  //ETX

  int IdCounter;
  int CurrentId;
  int Expanding;

  // Returns true if some tabs are visible.

  int AreTabsVisible();

  // Update the position of the body and mask elements

  void UpdateBodyPosition();
  void UpdateMaskPosition();

  // Constrain the visible pages depending on:
  // ShowAllPagesWithSameTag,
  // ShowOnlyPagesWithSameTag, 
  // ShowOnlyMostRecentPages

  void ConstrainVisiblePages();

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWNotebook(const vtkKWNotebook&); // Not implemented
  void operator=(const vtkKWNotebook&); // Not implemented
};

#endif

