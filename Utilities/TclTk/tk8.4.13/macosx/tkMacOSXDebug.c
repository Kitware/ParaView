/*
 * tkMacOSXDebug.c --
 *
 *  Implementation of Macintosh specific functions for debugging MacOS events,
 *      regions, etc...
 *
 *      Copyright 2001, Apple Computer, Inc.
 *
 *      The following terms apply to all files originating from Apple
 *      Computer, Inc. ("Apple") and associated with the software
 *      unless explicitly disclaimed in individual files.
 *
 *
 *      Apple hereby grants permission to use, copy, modify,
 *      distribute, and license this software and its documentation
 *      for any purpose, provided that existing copyright notices are
 *      retained in all copies and that this notice is included
 *      verbatim in any distributions. No written agreement, license,
 *      or royalty fee is required for any of the authorized
 *      uses. Modifications to this software may be copyrighted by
 *      their authors and need not follow the licensing terms
 *      described here, provided that the new terms are clearly
 *      indicated on the first page of each file where they apply.
 *
 *
 *      IN NO EVENT SHALL APPLE, THE AUTHORS OR DISTRIBUTORS OF THE
 *      SOFTWARE BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
 *      INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 *      THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
 *      EVEN IF APPLE OR THE AUTHORS HAVE BEEN ADVISED OF THE
 *      POSSIBILITY OF SUCH DAMAGE.  APPLE, THE AUTHORS AND
 *      DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
 *      BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS
 *      SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND APPLE,THE
 *      AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *      MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *      GOVERNMENT USE: If you are acquiring this software on behalf
 *      of the U.S. government, the Government shall have only
 *      "Restricted Rights" in the software and related documentation
 *      as defined in the Federal Acquisition Regulations (FARs) in
 *      Clause 52.227.19 (c) (2).  If you are acquiring the software
 *      on behalf of the Department of Defense, the software shall be
 *      classified as "Commercial Computer Software" and the
 *      Government shall have only "Restricted Rights" as defined in
 *      Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 *      foregoing, the authors grant the U.S. Government and others
 *      acting in its behalf permission to use and distribute the
 *      software in accordance with the terms specified in this
 *      license.
 *
 * RCS: @(#) Id
 */

#include "tkMacOSXInt.h"
#include "tkMacOSXDebug.h"

#ifdef TK_MAC_DEBUG

#include <mach-o/dyld.h>
#include <mach-o/nlist.h>

typedef struct {
 EventKind kind;
 char     * name;
} MyEventName;

typedef struct {
 EventClass    c;
 MyEventName * names;
} MyEventNameList;

static MyEventName windowEventNames [] = {
 {  kEventWindowUpdate,"Update"},
 {  kEventWindowDrawContent,"DrawContent"},
 {  kEventWindowActivated,"Activated"},
 {  kEventWindowDeactivated,"Deactivated"},
 {  kEventWindowGetClickActivation,"GetClickActivation"},
 {  kEventWindowShowing,"Showing"},
 {  kEventWindowHiding,"Hiding"},
 {  kEventWindowShown,"Shown"},
 {  kEventWindowHidden,"Hidden"},
 {  kEventWindowBoundsChanging,"BoundsChanging"},
 {  kEventWindowBoundsChanged,"BoundsChanged"},
 {  kEventWindowResizeStarted,"ResizeStarted"},
 {  kEventWindowResizeCompleted,"ResizeCompleted"},
 {  kEventWindowDragStarted,"DragStarted"},
 {  kEventWindowDragCompleted,"DragCompleted"},
 {  kEventWindowClickDragRgn,"ClickDragRgn"},
 {  kEventWindowClickResizeRgn,"ClickResizeRgn"},
 {  kEventWindowClickCollapseRgn,"ClickCollapseRgn"},
 {  kEventWindowClickCloseRgn,"ClickCloseRgn"},
 {  kEventWindowClickZoomRgn,"ClickZoomRgn"},
 {  kEventWindowClickContentRgn,"ClickContentRgn"},
 {  kEventWindowClickProxyIconRgn,"ClickProxyIconRgn"},
 {  kEventWindowCursorChange,"CursorChange" },
 {  kEventWindowCollapse,"Collapse"},
 {  kEventWindowCollapsed,"Collapsed"},
 {  kEventWindowCollapseAll,"CollapseAll"},
 {  kEventWindowExpand,"Expand"},
 {  kEventWindowExpanded,"Expanded"},
 {  kEventWindowExpandAll,"ExpandAll"},
 {  kEventWindowCollapse,"Collapse"},
 {  kEventWindowClose,"Close"},
 {  kEventWindowClosed,"Closed"},
 {  kEventWindowCloseAll,"CloseAll"},
 {  kEventWindowZoom,"Zoom"},
 {  kEventWindowZoomed,"Zoomed"},
 {  kEventWindowZoomAll,"ZoomAll"},
 {  kEventWindowContextualMenuSelect,"ContextualMenuSelect"},
 {  kEventWindowPathSelect,"PathSelect"},
 {  kEventWindowGetIdealSize,"GetIdealSize"},
 {  kEventWindowGetMinimumSize,"GetMinimumSize"},
 {  kEventWindowGetMaximumSize,"GetMaximumSize"},
 {  kEventWindowConstrain,"Constrain"},
 {  kEventWindowHandleContentClick,"HandleContentClick"},
 {  kEventWindowProxyBeginDrag,"ProxyBeginDra}"},
 {  kEventWindowProxyEndDrag,"ProxyEndDrag"},
 {  kEventWindowFocusAcquired,"FocusAcquired"},
 {  kEventWindowFocusRelinquish,"FocusRelinquish"},
 {  kEventWindowDrawFrame,"DrawFrame"},
 {  kEventWindowDrawPart,"DrawPart"},
 {  kEventWindowGetRegion,"GetRegion"},
 {  kEventWindowHitTest,"HitTest"},
 {  kEventWindowInit,"Init"},
 {  kEventWindowDispose,"Dispose"},
 {  kEventWindowDragHilite,"DragHilite"},
 {  kEventWindowModified,"Modified"},
 {  kEventWindowSetupProxyDragImage,"SetupProxyDragImage"},
 {  kEventWindowStateChanged,"StateChanged"},
 {  kEventWindowMeasureTitle,"MeasureTitle"},
 {  kEventWindowDrawGrowBox,"DrawGrowBox"},
 {  kEventWindowGetGrowImageRegion,"GetGrowImageRegion"},
 {  kEventWindowPaint,"Paint"},
 { 0, NULL },
};

static MyEventName mouseEventNames [] = {
 {  kEventMouseMoved, "Moved"},
 {  kEventMouseUp, "Up"},
 {  kEventMouseDown, "Down"},
 {  kEventMouseDragged, "Dragged"},
 {  kEventMouseWheelMoved, "WheelMoved"},
 { 0, NULL}
};

static MyEventName keyboardEventNames [] = {
 { kEventRawKeyDown, "Down"},
 { kEventRawKeyRepeat, "Repeat"},
 { kEventRawKeyUp, "Up"},
 { kEventRawKeyModifiersChanged, "ModifiersChanged"},
 { kEventHotKeyPressed, "HotKeyPressed"},
 { kEventHotKeyReleased, "HotKeyReleased"},
 { 0, NULL}
};

static MyEventName appEventNames [] = {
 { kEventAppActivated, "Activated"},
 { kEventAppDeactivated, "Deactivated"},  
 { kEventAppQuit, "Quit"},
 { kEventAppLaunchNotification, "LaunchNotification"},
 { kEventAppLaunched, "Launched"},
 { kEventAppTerminated, "Terminated"},
 { kEventAppFrontSwitched, "FrontSwitched"},
 { 0, NULL}
};

static MyEventName menuEventNames [] = {
 { kEventMenuBeginTracking, "BeginTracking"},
 { kEventMenuEndTracking, "EndTracking"},
 { kEventMenuChangeTrackingMode, "ChangeTrackingMode"},
 { kEventMenuOpening, "Opening"},
 { kEventMenuClosed, "Closed"},
 { kEventMenuTargetItem, "TargetItem"},
 { kEventMenuMatchKey, "MatchKey"},
 { kEventMenuEnableItems, "EnableItems"},
 { kEventMenuDispose, "Dispose"},
 { 0, NULL }
};

static MyEventName controlEventNames [] = {
 { kEventControlInitialize, "Initialize" },
 { kEventControlDispose, "Dispose" },
 { kEventControlGetOptimalBounds, "GetOptimalBounds" },
 { kEventControlHit, "Hit" },
 { kEventControlSimulateHit, "SimulateHit" },
 { kEventControlHitTest, "HitTest" },
 { kEventControlDraw, "Draw" },
 { kEventControlApplyBackground, "ApplyBackground" },
 { kEventControlApplyTextColor, "ApplyTextColor" },
 { kEventControlSetFocusPart, "SetFocusPart" },
 { kEventControlGetFocusPart, "GetFocusPart" },
 { kEventControlActivate, "Activate" },
 { kEventControlDeactivate, "Deactivate" },
 { kEventControlSetCursor, "SetCursor" },
 { kEventControlContextualMenuClick, "ContextualMenuClick" },
 { kEventControlClick, "Click" },
 { kEventControlTrack, "Track" },
 { kEventControlGetScrollToHereStartPoint, "GetScrollToHereStartPoint" },
 { kEventControlGetIndicatorDragConstraint, "GetIndicatorDragConstraint" },
 { kEventControlIndicatorMoved, "IndicatorMoved" },
 { kEventControlGhostingFinished, "GhostingFinished" },
 { kEventControlGetActionProcPart, "GetActionProcPart" },
 { kEventControlGetPartRegion, "GetPartRegion" },
 { kEventControlGetPartBounds, "GetPartBounds" },
 { kEventControlSetData, "SetData" },
 { kEventControlGetData, "GetData" },
 { kEventControlValueFieldChanged, "ValueFieldChanged" },
 { kEventControlAddedSubControl, "AddedSubControl" },
 { kEventControlRemovingSubControl, "RemovingSubControl" },
 { kEventControlBoundsChanged, "BoundsChanged" },
 { kEventControlOwningWindowChanged, "OwningWindowChanged" },
 { kEventControlArbitraryMessage, "ArbitraryMessage" },
 { 0, NULL }
};


static MyEventName commandEventNames [] = {
 { kEventCommandProcess, "Process" },
 { kEventCommandUpdateStatus, "UpdateStatus" },
 { 0, NULL }
};

static MyEventNameList eventNameList [] = {
 { kEventClassWindow, windowEventNames },
 { kEventClassMouse, mouseEventNames },
 { kEventClassKeyboard, keyboardEventNames },
 { kEventClassApplication, appEventNames },
 { kEventClassMenu, menuEventNames },
 { kEventClassControl, controlEventNames },
 { kEventClassCommand, commandEventNames },
 { 0, NULL}
};


static MyEventName classicEventNames [] = {
 { nullEvent,"nullEvent" },
 { mouseDown,"mouseDown" },
 { mouseUp,"mouseUp" },
 { keyDown,"keyDown" },
 { keyUp,"keyUp" },
 { autoKey,"autoKey" },
 { updateEvt,"updateEvt" },
 { diskEvt,"diskEvt" },
 { activateEvt,"activateEvt" },
 { osEvt,"osEvt" },
 { kHighLevelEvent,"kHighLevelEvent" },
 { 0, NULL }
};

char * 
CarbonEventToAscii(EventRef eventRef, char * buf)
{     
    EventClass eventClass;
    EventKind  eventKind;
    MyEventNameList * list = eventNameList;
    MyEventName     * names = NULL;
    int *       iPtr = ( int * )buf;
    char *      iBuf = buf;
    int found = 0;

    eventClass = GetEventClass(eventRef);
    eventKind = GetEventKind(eventRef);

    *iPtr = eventClass;
    buf [ 4 ] = 0;
    strcat(buf, " ");
    buf += strlen(buf);
    while (list->names && (!names) ) {
        if (eventClass == list->c) {
            names = list -> names;
        } else {
            list++;
        }
    }
    if (names) {
       found = 0;
       while (names->name && !found) {
           if (eventKind == names->kind) {
               sprintf(buf, "%-20s", names->name);
               found = 1;
           } else {
               names++;
           }
        }
        if (!found) {
            sprintf(buf, "%-20d", eventKind );
        }
    } else {
        sprintf(buf, "%-20d", eventKind );
    }
    return iBuf;
}

char *
CarbonEventKindToAscii(EventRef eventRef, char * buf )
{     
   EventClass eventClass;
   EventKind  eventKind;
   MyEventNameList * list = eventNameList;
   MyEventName     * names = NULL;
   int               found = 0;
   eventClass = GetEventClass(eventRef);
   eventKind = GetEventKind(eventRef);
   while (list->names && (!names) ) {
       if (eventClass == list -> c) {
           names = list -> names;
       } else {
           list++;
       }
   }
   if (names) {
       found = 0;
       while ( names->name && !found ) {
           if (eventKind == names->kind) {
               sprintf(buf,"%s",names->name);
               found = 1;
           } else {
               names++;
           }
       }
    }
    if (!found) {
        sprintf ( buf,"%d", eventKind );
     } else {
        sprintf ( buf,"%d", eventKind );
     }
     return buf;
}

char *
ClassicEventToAscii(EventRecord * eventPtr, char * buf )
{
    MyEventName     * names = NULL;
    int found = 0;
    names = classicEventNames;
    while ( names -> name && !found )
        if (eventPtr->what == names->kind) {
            int * iPtr;
            char cBuf[8];
            iPtr=(int *) &cBuf;
            *iPtr = eventPtr->message;
            cBuf[4] = 0;
            sprintf(buf, "%-16s %08x %04x %s", names->name,
                    (int) eventPtr->message,
                    eventPtr->modifiers, 
                    cBuf);
            found = 1;
        } else {
          names++;
        }
    if (!found) {
               sprintf(buf,"%-16d %08x %08x, %s",
                       eventPtr->what, (int) eventPtr->message,
                       eventPtr->modifiers, buf);
    }
    return buf;
 
}

void
printPoint(char * tag, Point * p )
{
    fprintf(stderr,"%s %4d %4d\n",
        tag,p->h,p->v );
}

void
printRect(char * tag, Rect * r )
{
    fprintf(stderr,"%s %4d %4d %4d %4d (%dx%d)\n",
        tag, r->left, r->top, r->right, r->bottom,
        r->right - r->left + 1, r->bottom - r->top + 1);
}

void
printRegion(char * tag, RgnHandle rgn )
{
    Rect r;
    GetRegionBounds(rgn,&r);
    printRect(tag,&r);
}

void
printWindowTitle(char * tag, WindowRef window )
{
    Str255 title;
    GetWTitle(window,title);
    title [title[0] + 1] = 0;
    fprintf(stderr, "%s %s\n", tag, title +1 );
}

typedef struct {
 int    msg;
 char * name;
} MsgName;

static MsgName msgNames [] = {
    { kMenuDrawMsg,       "Draw"},
    { kMenuSizeMsg,       "Size"},
    { kMenuPopUpMsg,      "PopUp"},
    { kMenuCalcItemMsg,   "CalcItem" },
    { kMenuThemeSavvyMsg, "ThemeSavvy"},
    { kMenuInitMsg,       "Init" },
    { kMenuDisposeMsg,    "Dispose" },
    { kMenuFindItemMsg,   "FindItem" },
    { kMenuHiliteItemMsg, "HiliteItem" },
    { kMenuDrawItemsMsg,  "DrawItems" },
    { -1, NULL }
};

char *
TkMacOSXMenuMessageToAscii(int msg, char * s)
{
    MsgName * msgNamePtr;
    for (msgNamePtr = msgNames;msgNamePtr->name;) {
        if (msgNamePtr->msg == msg) {
           strcpy(s,msgNamePtr->name);
           return s;
        } else {
            msgNamePtr++;
        }
    }
    sprintf(s,"unknown : %d", msg );
    return s;
}

static MsgName trackingNames [] = {
    { kMouseTrackingMousePressed  , "MousePressed  " },
    { kMouseTrackingMouseReleased , "MouseReleased " },
    { kMouseTrackingMouseExited   , "MouseExited   " },
    { kMouseTrackingMouseEntered  , "MouseEntered  " },
    { kMouseTrackingMouseMoved    , "MouseMoved    " },
    { kMouseTrackingKeyModifiersChanged, "KeyModifiersChanged" },
    { kMouseTrackingUserCancelled , "UserCancelled " },
    { kMouseTrackingTimedOut      , "TimedOut      " },
    { -1, NULL }
};

char *
MouseTrackingResultToAscii(MouseTrackingResult r, char * buf) 
{
    MsgName * namePtr;
    for (namePtr = trackingNames; namePtr->name; namePtr++) {
        if (namePtr->msg == r) {
            strcpy(buf, namePtr->name);
            return buf;
        }
    }
    sprintf(buf, "Unknown mouse tracking result : %d", r);
    return buf;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetNamedDebugSymbol --
 *
 *
 *    Dynamically acquire address of a named symbol from a loaded
 *    dynamic library, so that we can use API that may not be
 *    available on all OS versions.
 *    For debugging purposes, if we cannot find the symbol with the
 *    usual dynamic library APIs, we manually walk the symbol table
 *    of the loaded library. This allows access to unexported
 *    symbols such as private_extern internal debugging functions.
 *    If module is NULL or the empty string, search all loaded
 *    libraries (could be very expensive and should be avoided).
 *
 *    THIS FUCTION IS ONLY TO BE USED FOR DEBUGGING PURPOSES, IT MAY
 *    BREAK UNEXPECTEDLY IN THE FUTURE !
 *   
 * Results:
 *    Address of given symbol or NULL if unavailable.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void *
TkMacOSXGetNamedDebugSymbol(const char* module, const char* symbol)
{
    void* addr = TkMacOSXGetNamedSymbol(module, symbol);
#ifndef __LP64__
    if (!addr) {
  const struct mach_header *mh = NULL;
  uint32_t i, n = _dyld_image_count();

  for (i = 0; i < n; i++) {
      if (module && *module) {
    /* Find image with given module name */
    char *name;
    const char *path = _dyld_get_image_name(i);

    if (!path) {
        continue;
    }
    name = strrchr(path, '/') + 1;
    if (strncmp(name, module, strlen(name)) != 0) {
        continue;
    }
      }
      mh = _dyld_get_image_header(i);
      if (mh) {
    struct load_command *lc;
    struct symtab_command *st = NULL;
    struct segment_command *sg = NULL;
    uint32_t j, m, nsect = 0, txtsectx = 0;
    
    lc = (struct load_command*)((char *) mh +
      sizeof(struct mach_header));
    m = mh->ncmds;
    for (j = 0; j < m; j++) {
        /* Find symbol table and index of __text section */
        if (lc->cmd == LC_SEGMENT) {
      /* Find last segment before symbol table */
      sg = (struct segment_command*) lc;
      if (!txtsectx) {
          /* Count total sections until (__TEXT, __text) */
          uint32_t k, ns = sg->nsects;
          
          if (strcmp(sg->segname, SEG_TEXT) == 0) {
        struct section *s = (struct section *)(
          (char *)sg +
          sizeof(struct segment_command));
        
        for(k = 0; k < ns; k++) {
            if (strcmp(s->sectname, SECT_TEXT) == 0) {
          txtsectx = nsect+k+1;
          break;
            }
            s++;
        }
          }
          nsect += ns;
      }
        } else if (!st && lc->cmd == LC_SYMTAB) {
      st = (struct symtab_command*) lc;
      break;
        }
        lc = (struct load_command *)((char *) lc + lc->cmdsize);
    }
    if (st && sg && txtsectx) {
        intptr_t base, slide = _dyld_get_image_vmaddr_slide(i);
        char *strings;
        struct nlist *sym;
        uint32_t strsize = st->strsize;
        int32_t strx;
        
        /* Offset file positions by difference to actual position
           in memory of last segment before symbol table: */
        base = (intptr_t) sg->vmaddr + slide - sg->fileoff;
        strings = (char*)(base + st->stroff);
        sym = (struct nlist*)(base + st->symoff);
        m = st->nsyms;
        for (j = 0; j < m; j++) {
      /* Find symbol with given name in __text section */
      strx = sym->n_un.n_strx;
      if ((sym->n_type & N_TYPE) == N_SECT &&
        sym->n_sect == txtsectx &&
        strx > 0 && strx < strsize &&
        strcmp(strings + strx, symbol) == 0) {
          addr = (void*) sym->n_value + slide;
          break;
      }
      sym++;
        }
    }
      }
      if (module && *module) {
    /* If given a module name, only search corresponding image */
    break;
      }
  }
    }
#endif /* __LP64__ */
    return addr;
}

#endif /* TK_MAC_DEBUG */
