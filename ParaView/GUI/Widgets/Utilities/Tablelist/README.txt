This is the Tablelist 4.0 widget. See info and license below.

The changes are:

- the checked.xbm and unchecked.xbm files from the images/ subdir, used in
  tablelistEdit.tcl have been converted to resources and are created at run
  time into the tablelistChecked and tablelistUnchecked Tk images. 
  In tablelistEdit.tcl, the following code has to be changed:
		set checkedImg [image create bitmap -file \
		    [file join $library images checked.xbm]]
		set uncheckedImg [image create bitmap -file \
		    [file join $library images unchecked.xbm]]
  into:
		set checkedImg tablelistChecked
		set uncheckedImg tablelistUnchecked

- added a tablelistUtil2.tcl to provide a tablelist::emptyStr proc, useful
  for -formatcommand

- the default mouse binding triggers "editing" on a single-click, which
  is downright annoying since it does prevents proper navigation or
  selection. The tablelistBind.tcl file has been edited to edit on
  double-click only. 
  a) copy the contents of: bind TablelistBody <Button-1> {...}
     to the empty script: bind TablelistBody <Double-Button-1> { ... }
     then remove everything below: tablelist::condEditContainingCell ... \ 
  b) in: bind TablelistBody <Button-1> {...}
     remove the line: tablelist::condEditContainingCell ... \ + next line

- in tablelistUtil.tcl, fix a bug related to images in column label. Quoting 
  the author: "In the interim you can work around this bug by inserting the
  statement
    set data(lastCol) $col
  after line #986".

- another bug fix, in tablelistConfig.tcl please locate the 2 statements
    if {$existsAux && [string compare $val $data($name)] == 0} {
  and replace:
    if {$existsAux && [info exists data($name)] && [string compare $val $data($name)] == 0} {

--------------------------------------------------------------------------

               The Multi-Column Listbox Package Tablelist

                                   by

                             Csaba Nemethi

                       csaba.nemethi@t-online.de 


What is Tablelist?
------------------

Tablelist is a library package for Tcl/Tk version 8.0 or higher,
written in pure Tcl/Tk code.

    http://www.nemethi.de
