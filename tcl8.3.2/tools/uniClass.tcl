proc emitRange {first last} {
    global ranges numranges chars numchars

    if {$first < ($last-1)} {
	append ranges [format "{0x%04x, 0x%04x}, " \
		$first $last]
	if {[incr numranges] % 4 == 0} {
	    append ranges "\n    "
	}
    } else {
	append chars [format "0x%04x, " $first]
	incr numchars
	if {$numchars % 9 == 0} {
	    append chars "\n    "
	}
	if {$first != $last} {
	    append chars [format "0x%04x, " $last]
	    incr numchars
	    if {$numchars % 9 == 0} {
		append chars "\n    "
	    }
	}
    }
}

proc genTable {type} {
    global first last ranges numranges chars numchars
    set first -2
    set last -2

    set ranges "    "
    set numranges 0
    set chars "    "
    set numchars 0

    for {set i 0} {$i < 0x10000} {incr i} {
	if {[string is $type [format %c $i]]} {
	    if {$i == ($last + 1)} {
		set last $i
	    } else {
		if {$first > 0} {
		    emitRange $first $last
		}
		set first $i
		set last $i
	    }
	}
    }
    emitRange $first $last
    
    puts "static crange ${type}RangeTable\[\] = {\n$ranges\n};\n"
    puts "#define NUM_[string toupper $type]_RANGE (sizeof(${type}RangeTable)/sizeof(crange))\n"
    puts "static chr ${type}CharTable\[\] = {\n$chars\n};\n"
    puts "#define NUM_[string toupper $type]_CHAR (sizeof(${type}CharTable)/sizeof(chr))\n"
}


foreach type {alpha digit punct space lower upper graph } {
    genTable $type
}

