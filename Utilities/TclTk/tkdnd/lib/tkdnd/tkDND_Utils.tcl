##
## tkDND_Utils.tcl --
## 
##    This file implements some tcl procedures that are used by the tkDND
##    package.
##
## This software is copyrighted by:
## George Petasis, National Centre for Scientific Research "Demokritos",
## Aghia Paraskevi, Athens, Greece.
## e-mail: petasis@iit.demokritos.gr
##
## The following terms apply to all files associated
## with the software unless explicitly disclaimed in individual files.
##
## The authors hereby grant permission to use, copy, modify, distribute,
## and license this software and its documentation for any purpose, provided
## that existing copyright notices are retained in all copies and that this
## notice is included verbatim in any distributions. No written agreement,
## license, or royalty fee is required for any of the authorized uses.
## Modifications to this software may be copyrighted by their authors
## and need not follow the licensing terms described here, provided that
## the new terms are clearly indicated on the first page of each file where
## they apply.
## 
## IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
## FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
## ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
## DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
## 
## THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
## IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
## NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
## MODIFICATIONS.
##

namespace eval ::dnd {

  variable AskSelectedAction
  
  ## This procedure is used just to ensure that the object given as its
  ## argument can be accessed as a binary object. Many thanks to Paul Duffin
  ## for the idea :-)
  proc ConvertToBinary {object} {
    binary scan $object {}
    return $object
  };# ConvertToBinary

  ## This procedure handles the special case where we want items into a canvas
  ## widget to be drop targets. It emulates this case as follows:
  ##
  ##   The tkDND extension is able to deliver events only to real windows
  ##   (that means tk widgets). This procedure can be the binding script of all
  ##   dnd events we are interested in being received by the canvas items.
  ##   When this function is called, it tries to find the item that the mouse
  ##   is over (if any). Then it examines its bindings, and if it finds the
  ##   dnd related event that is processing it delivers this event to the
  ##   particular item. Fianlly, it tries to emulate <DragEnter>/<DragLeave>
  ##   on every canvas item...
  proc CanvasDeliverEvent {event actions action button data descriptions 
                           mods type win X x Y y} {
    ## This function will find the topmost item that the mouse is over, and
    ## Deliver the event specified by the "event" arg to this item...
    global CanvasDeliverEventStatus
    switch $event {
       <DragEnter>  -
      <<DragEnter>> {set CanvasDeliverEventStatus(item) {}}
      default {}
    }

    ## Translate mouse coordinates to canvas coordinates...
    set cx [$win canvasx $x]
    set cy [$win canvasy $y]
    set cx_1 [expr {$cx+1}]
    set cy_1 [expr {$cy+1}]
    ## Find all tags that are under the mouse...
    set tags [$win find overlapping $cx $cy $cx_1 $cy_1]
    ## ... and select the topmost...
    set length [llength $tags]

    ## If no tags under the mouse, return...
    if {!$length} {
      ##puts -->$CanvasDeliverEventStatus(item)
      if {[string length $CanvasDeliverEventStatus(item)]} {
        ## Send <<DragLeave>>...
        set _id $CanvasDeliverEventStatus(item)
        set _binding {}
        foreach _tag [concat $_id [$win gettags $_id]] {
          set _binding [$win bind $_tag <<DragLeave>>]
          if {[string length $_binding]} {break}
        }
        ## puts "Sending <DragLeave> (1) to $_id ($_binding)"
        set script {}
        foreach element $_binding {
          switch $element {
            %% {lappend script %}       
            %A {lappend script $action} %a {lappend script $actions}
            %b {lappend script $button}
            %D {lappend script $data}   %d {lappend script $descriptions}
            %m {lappend script $mods}
            %T {lappend script $type}
            %W {lappend script $win}
            %X {lappend script $X}      %x {lappend script $x}
            %Y {lappend script $Y}      %y {lappend script $y}
            %I {lappend script $_id}
            default {lappend script $element}
          }
        }
        if {[llength $script]} {eval $script}
      }
      set CanvasDeliverEventStatus(item) {}
      update
      if {[string equal $event <<Drag>>]} {
        return -code break
      }
      return $action
    }
    if {$length == 1} {
      set id $tags
    } else {
      set id [$win find closest $cx $cy]
    }

    ## Now in "id" we have the tag of the item below the mouse...
    ## Has this item a binding?
    foreach tag [concat $id [$win gettags $id]] {
      set binding [$win bind $tag $event]
      if {[string length $binding]} {break}
    }
  
    ## Is this tag the same as the last one? If is different, we have to send
    ## a leave event to the previous item and an enter event to this one...
    if {$CanvasDeliverEventStatus(item) != $id} {
      if {[string length $CanvasDeliverEventStatus(item)]} {
        ## Send <<DragLeave>>...
        set _id $CanvasDeliverEventStatus(item)
        set _binding {}
        foreach _tag [concat $_id [$win gettags $_id]] {
          set _binding [$win bind $_tag <<DragLeave>>]
          if {[string length $_binding]} {break}
        }
        ## puts "Sending <DragLeave> (2) to $_id ($_binding)"
        set script {}
        foreach element $_binding {
          switch $element {
            %% {lappend script %}       
            %A {lappend script $action} %a {lappend script $actions}
            %b {lappend script $button}
            %D {lappend script $data}   %d {lappend script $descriptions}
            %m {lappend script $mods}
            %T {lappend script $type}
            %W {lappend script $win}
            %X {lappend script $X}      %x {lappend script $x}
            %Y {lappend script $Y}      %y {lappend script $y}
            %I {lappend script $_id}
            default {lappend script $element}
          }
        }
        if {[llength $script]} {eval $script}
      }
      ## Send <<DndEnter>>...
      set _id $id
      set _binding {}
      foreach _tag [concat $_id [$win gettags $_id]] {
        set _binding [$win bind $_tag <<DragEnter>>]
        if {[string length $_binding]} {break}
      }
      ## puts "Sending <DragEnter> to $tag ($_binding)"
      set script {}
      foreach element $_binding {
        switch $element {
          %% {lappend script %}       
          %A {lappend script $action} %a {lappend script $actions}
          %b {lappend script $button}
          %D {lappend script $data}   %d {lappend script $descriptions}
          %m {lappend script $mods}
          %T {lappend script $type}
          %W {lappend script $win}
          %X {lappend script $X}      %x {lappend script $x}
          %Y {lappend script $Y}      %y {lappend script $y}
          %I {lappend script $_id}
          default {lappend script $element}
        }
      }
      if {[llength $script]} {eval $script}
      set CanvasDeliverEventStatus(item) $id
    }

    set script {}
    foreach element $binding {
      switch $element {
        %% {lappend script %}       
        %A {lappend script $action} %a {lappend script $actions}
        %b {lappend script $button}
        %D {lappend script $data}   %d {lappend script $descriptions}
        %m {lappend script $mods}
        %T {lappend script $type}
        %W {lappend script $win}
        %X {lappend script $X}      %x {lappend script $x}
        %Y {lappend script $Y}      %y {lappend script $y}
        %I {lappend script $id}
        default {lappend script $element}
      }
    }
    if {[llength $script]} {
      return [eval $script]
    }
    set CanvasDeliverEventStatus(item) {}
    update
    if {[string equal $event <<Drag>>]} {
      return -code break
    }
    return $action
  };# CanvasDeliverEvent

  ## ChooseAskAction --
  ##   This procedure displays a dialog with the help of which the user can
  ##   select one of the supported actions...
  proc ChooseAskAction {window x y actions descriptions args} {
    variable AskSelectedAction
    set title {Please Select Action:}
    foreach action $actions descr $descriptions {
      if {[string equal $action ask]} {
        set title $descr
        break
      }
    }

    set menu $window.__tk_dnd[pwd]__action_ask__Drop_window_[pid]
    catch {destroy $menu}
    menu $menu -title $title -tearoff 0 -disabledforeground darkgreen
    $menu add command -font {helvetica 12 bold} \
          -label $title -command "destroy $menu" -state disabled
    $menu add separator

    set items 0
    foreach action $actions descr $descriptions {
      if {[string equal $action ask]} continue
      $menu add command -label $descr -command \
        "set ::dnd::AskSelectedAction $action; destroy $menu"
      incr items
    }
    if {!$items} {
      ## The drag source accepts the ask action, but has no defined action
      ## list? Add copy action at least...
      $menu add command -label Copy -command \
        "set ::dnd::AskSelectedAction copy; destroy $menu"
    }
    
    $menu add separator
    $menu add command -label {Cancel Drop} -command \
        "set ::dnd::AskSelectedAction none; destroy $menu"

    set AskSelectedAction none
    tk_popup $menu $x $y
    update
    bind $menu <Unmap> {after idle {catch {destroy %W}}}
    tkwait window $menu
    
    return $AskSelectedAction
  }
};# namespace eval ::dnd

# EOF
