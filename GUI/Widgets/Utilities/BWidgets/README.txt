This is a subset of the BWidget ToolKit 1.7.0. See info and license below.

The changes are:

- the plus.xbm and minus.xbm files in the images/ subdir, used in tree.tcl
  have been converted to resources and are created at run time into the
  bwplus and bwimage Tk images. In tree.tcl, the following code has to be
  changed:
    Widget::configure $path [list -crossopenbitmap @$file]
    Widget::configure $path [list -crossclosebitmap @$file]
  into:
    Widget::configure $path [list -crossopenimage bwminus]
    Widget::configure $path [list -crosscloseimage bwplus]

- the dragfile.gif and dragicon.gif files in the images/ subdir, used in 
  dragsite.tcl have been converted to resources and are created at run time
  into the bwdragfile and bwdragicon Tk images. In dragsite.tcl, the following
  code has to be changed:
               label $_topw.l -image [Bitmap::get dragicon] -relief flat -bd 0
               label $_topw.l -image [Bitmap::get dragfile] -relief flat -bd 0
   into:
               label $_topw.l -image bwdragicon -relief flat -bd 0
               label $_topw.l -image bwdragfile -relief flat -bd 0
 
- the library uses ::BWIDGET::LIBRARY, set in pkgIndex.tcl. Since
  we include the lib at compile time, this variable has to be set manually
  (to any value really) and plugged in one of the files evaluated first
  (say, utils.tcl):
  namespace eval ::BWIDGET {};
  set ::BWIDGET::LIBRARY {};

- The documentation states: "A <<TreeSelect>> virtual event is generated any
  time the selection in the tree changes.". It is actually not true, it
  is only generated for interaction done with the mouse, not the keyboard
  for example. In order for it to work all the time, the following code
  in tree.tcl:
    switch -- $cmd {
        "add" - "clear" - "remove" - "set" - "toggle" {
            event generate $path <<TreeSelect>>
        }
    }
  should be moved from Tree::_mouse_select to the end of Tree::selection,
  *after* _redraw_idle $path 1

--------------------------------------------------------------------------

BWidget ToolKit 1.7.0				December 2003
Copyright (c) 1998-1999 UNIFIX.
Copyright (c) 2001-2002 ActiveState Corp. 

See the file LICENSE.txt for license info (uses Tcl's BSD-style license).

WHAT IS BWIDGET ?

The BWidget Toolkit is a high-level Widget Set for Tcl/Tk built using
native Tcl/Tk 8.x namespaces.

The BWidgets have a professional look&feel as in other well known
Toolkits (Tix or Incr Widgets), but the concept is radically different
because everything is pure Tcl/Tk.  No platform dependencies, and no
compiling required.  The code is 100% Pure Tcl/Tk.

The BWidget library was originally developed by UNIFIX Online, and
released under both the GNU Public License and the Tcl license.
BWidget is now maintained as a community project, hosted by
Sourceforge.  Scores of fixes and enhancements have been added by
community developers.  See the ChangeLog file for details.

CONTACTS

The BWidget toolkit is maintained on Sourceforge, at
http://www.sourceforge.net/projects/tcllib/
