This is the Tablelist 4.3 widget. See info and license below.

The changes are:

- added a tablelistUtil2.tcl to provide a tablelist::emptyStr proc, useful
  for -formatcommand

- the default mouse binding triggers "editing" on a single-click, which
  is downright annoying since it does prevents proper navigation or
  selection. The tablelistBind.tcl file has been edited to edit on
  double-click only. In tablelist::defineTablelistBody:
  a) copy the contents of: bind TablelistBody <Button-1> {...}
     to the empty script: bind TablelistBody <Double-Button-1> { ... }
     then in bind TablelistBody <Double-Button-1> { remove everything below: 
        tablelist::condEditContainingCell ... \ 
         $tablelist::x $tablelist::y     
  b) in: bind TablelistBody <Button-1> {...}
     remove the line: tablelist::condEditContainingCell ... \ + next line

- If an uneditable cell in a tablelist is double-clicked, let's generate
  a virtual event <<TablelistUneditableCellSelected>>. 
  In tablelistBind.tcl, locate tablelist::condEditContainingCell and find
  the lines above; Add the part after else.
      #
    	# Finish a possibly active cell editing
    	#
	    if {$data(editRow) >= 0} {
	        finisheditingSubCmd $win
    	  } else {
            event generate $win <<TablelistUneditableCellSelected>>
          }
