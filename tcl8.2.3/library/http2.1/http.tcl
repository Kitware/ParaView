# http.tcl --
#
#	Client-side HTTP for GET, POST, and HEAD commands.
#	These routines can be used in untrusted code that uses 
#	the Safesock security policy.  These procedures use a 
#	callback interface to avoid using vwait, which is not 
#	defined in the safe base.
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# RCS: @(#) Id

package provide http 2.2	;# This uses Tcl namespaces

namespace eval http {
    variable http
    array set http {
	-accept */*
	-proxyhost {}
	-proxyport {}
	-useragent {Tcl http client package 2.2}
	-proxyfilter http::ProxyRequired
    }

    variable formMap
    variable alphanumeric a-zA-Z0-9
    variable c
    variable i 0
    for {} {$i <= 256} {incr i} {
	set c [format %c $i]
	if {![string match \[$alphanumeric\] $c]} {
	    set formMap($c) %[format %.2x $i]
	}
    }
    # These are handled specially
    array set formMap {
	" " +   \n %0d%0a
    }

    variable urlTypes
    array set urlTypes {
	http	{80 ::socket}
    }

    namespace export geturl config reset wait formatQuery register unregister
    # Useful, but not exported: data size status code
}

# http::register --
#
#     See documentaion for details.
#
# Arguments:
#     proto           URL protocol prefix, e.g. https
#     port            Default port for protocol
#     command         Command to use to create socket
# Results:
#     list of port and command that was registered.

proc http::register {proto port command} {
    variable urlTypes
    set urlTypes($proto) [list $port $command]
}

# http::unregister --
#
#     Unregisters URL protocol handler
#
# Arguments:
#     proto           URL protocol prefix, e.g. https
# Results:
#     list of port and command that was unregistered.

proc http::unregister {proto} {
    variable urlTypes
    if {![info exists urlTypes($proto)]} {
	return -code error "unsupported url type \"$proto\""
    }
    set old $urlTypes($proto)
    unset urlTypes($proto)
    return $old
}

# http::config --
#
#	See documentaion for details.
#
# Arguments:
#	args		Options parsed by the procedure.
# Results:
#        TODO

proc http::config {args} {
    variable http
    set options [lsort [array names http -*]]
    set usage [join $options ", "]
    if {[llength $args] == 0} {
	set result {}
	foreach name $options {
	    lappend result $name $http($name)
	}
	return $result
    }
    regsub -all -- - $options {} options
    set pat ^-([join $options |])$
    if {[llength $args] == 1} {
	set flag [lindex $args 0]
	if {[regexp -- $pat $flag]} {
	    return $http($flag)
	} else {
	    return -code error "Unknown option $flag, must be: $usage"
	}
    } else {
	foreach {flag value} $args {
	    if {[regexp -- $pat $flag]} {
		set http($flag) $value
	    } else {
		return -code error "Unknown option $flag, must be: $usage"
	    }
	}
    }
}

# http::Finish --
#
#	Clean up the socket and eval close time callbacks
#
# Arguments:
#	token	Connection token.
#	errormsg	If set, forces status to error.
#
# Side Effects:
#        Closes the socket

 proc http::Finish { token {errormsg ""} } {
    variable $token
    upvar 0 $token state
    global errorInfo errorCode
    if {[string length $errormsg] != 0} {
	set state(error) [list $errormsg $errorInfo $errorCode]
	set state(status) error
    }
    catch {close $state(sock)}
    catch {after cancel $state(after)}
    if {[info exists state(-command)]} {
	if {[catch {eval $state(-command) {$token}} err]} {
	    if {[string length $errormsg] == 0} {
		set state(error) [list $err $errorInfo $errorCode]
		set state(status) error
	    }
	}
	if {[info exist state(-command)]} {
	    # Command callback may already have unset our state
	    unset state(-command)
	}
    }
}

# http::reset --
#
#	See documentaion for details.
#
# Arguments:
#	token	Connection token.
#	why	Status info.
#
# Side Effects:
#       See Finish

proc http::reset { token {why reset} } {
    variable $token
    upvar 0 $token state
    set state(status) $why
    catch {fileevent $state(sock) readable {}}
    Finish $token
    if {[info exists state(error)]} {
	set errorlist $state(error)
	unset state(error)
	eval error $errorlist
    }
}

# http::geturl --
#
#	Establishes a connection to a remote url via http.
#
# Arguments:
#       url		The http URL to goget.
#       args		Option value pairs. Valid options include:
#				-blocksize, -validate, -headers, -timeout
# Results:
#	Returns a token for this connection.
#	This token is the name of an array that the caller should
#	unset to garbage collect the state.

proc http::geturl { url args } {
    variable http
    variable urlTypes

    # Initialize the state variable, an array.  We'll return the
    # name of this array as the token for the transaction.

    if {![info exists http(uid)]} {
	set http(uid) 0
    }
    set token [namespace current]::[incr http(uid)]
    variable $token
    upvar 0 $token state
    reset $token

    # Process command options.

    array set state {
	-blocksize 	8192
	-validate 	0
	-headers 	{}
	-timeout 	0
	-type           application/x-www-form-urlencoded
	state		header
	meta		{}
	currentsize	0
	totalsize	0
        type            text/html
        body            {}
	status		""
    }
    set options {-blocksize -channel -command -handler -headers \
		-progress -query -validate -timeout -type}
    set usage [join $options ", "]
    regsub -all -- - $options {} options
    set pat ^-([join $options |])$
    foreach {flag value} $args {
	if {[regexp $pat $flag]} {
	    # Validate numbers
	    if {[info exists state($flag)] && \
		    [string is integer -strict $state($flag)] && \
		    ![string is integer -strict $value]} {
		return -code error "Bad value for $flag ($value), must be integer"
	    }
	    set state($flag) $value
	} else {
	    return -code error "Unknown option $flag, can be: $usage"
	}
    }
    if {![regexp -nocase {^(([^:]*)://)?([^/:]+)(:([0-9]+))?(/.*)?$} $url \
	    x prefix proto host y port srvurl]} {
	error "Unsupported URL: $url"
    }
    if {[string length $proto] == 0} {
	set proto http
	set url ${proto}://$url
    }
    if {![info exists urlTypes($proto)]} {
	return -code error "unsupported url type \"$proto\""
    }
    set defport [lindex $urlTypes($proto) 0]
    set defcmd [lindex $urlTypes($proto) 1]

    if {[string length $port] == 0} {
	set port $defport
    }
    if {[string length $srvurl] == 0} {
	set srvurl /
    }
    if {[string length $proto] == 0} {
	set url http://$url
    }
    set state(url) $url
    if {![catch {$http(-proxyfilter) $host} proxy]} {
	set phost [lindex $proxy 0]
	set pport [lindex $proxy 1]
    }

    # If a timeout is specified we set up the after event
    # and arrange for an asynchronous socket connection.

    if {$state(-timeout) > 0} {
	set state(after) [after $state(-timeout) \
		[list http::reset $token timeout]]
	set async -async
    } else {
	set async ""
    }

    # If we are using the proxy, we must pass in the full URL that
    # includes the server name.

    if {[info exists phost] && [string length $phost]} {
	set srvurl $url
	set s [eval $defcmd $async {$phost $pport}]
    } else {
	set s [eval $defcmd $async {$host $port}]
    }
    set state(sock) $s

    # Wait for the connection to complete

    if {$state(-timeout) > 0} {
	fileevent $s writable [list http::Connect $token]
	http::wait $token
	if {![string equal $state(status) "connect"]} {
	    return $token
	}
	fileevent $s writable {}
	set state(status) ""
    }

    # Send data in cr-lf format, but accept any line terminators

    fconfigure $s -translation {auto crlf} -buffersize $state(-blocksize)

    # The following is disallowed in safe interpreters, but the socket
    # is already in non-blocking mode in that case.

    catch {fconfigure $s -blocking off}
    set len 0
    set how GET
    if {[info exists state(-query)]} {
	set len [string length $state(-query)]
	if {$len > 0} {
	    set how POST
	}
    } elseif {$state(-validate)} {
	set how HEAD
    }
    if {[catch {
	puts $s "$how $srvurl HTTP/1.0"
	puts $s "Accept: $http(-accept)"
	puts $s "Host: $host"
	puts $s "User-Agent: $http(-useragent)"
	foreach {key value} $state(-headers) {
	    regsub -all \[\n\r\]  $value {} value
	    set key [string trim $key]
	    if {[string length $key]} {
		puts $s "$key: $value"
	    }
	}
	if {$len > 0} {
	    puts $s "Content-Length: $len"
	    puts $s "Content-Type: $state(-type)"
	    puts $s ""
	    fconfigure $s -translation {auto binary}
	    puts $s $state(-query)
	} else {
	    puts $s ""
	}
	flush $s
	fileevent $s readable [list http::Event $token]
    } err]} {
	# The socket probably was never connected, or the connection
	# dropped later.

	reset $token ioerror
	return $token
    }

    if {! [info exists state(-command)]} {
	wait $token
    }
    return $token
}

# Data access functions:
# Data - the URL data
# Status - the transaction status: ok, reset, eof, timeout
# Code - the HTTP transaction code, e.g., 200
# Size - the size of the URL data

proc http::data {token} {
    variable $token
    upvar 0 $token state
    return $state(body)
}
proc http::status {token} {
    variable $token
    upvar 0 $token state
    return $state(status)
}
proc http::code {token} {
    variable $token
    upvar 0 $token state
    return $state(http)
}
proc http::size {token} {
    variable $token
    upvar 0 $token state
    return $state(currentsize)
}

# http::cleanup
#
#	Garbage collect the state associated with a transaction
#
# Arguments
#	token	The token returned from http::geturl
#
# Side Effects
#	unsets the state array

proc http::cleanup {token} {
    variable $token
    upvar 0 $token state
    if {[info exist state]} {
	unset state
    }
}

# http::Connect
#
#	Wait for an asynchronous connection to complete
#
# Arguments
#	token	The token returned from http::geturl
#
# Side Effects
#	Sets the status of the connection, which unblocks
# 	the waiting geturl call

 proc http::Connect {token} {
    variable $token
    upvar 0 $token state
    if {[eof $state(sock)]} {
	set state(status) ioerror
    } else {
	set state(status) connect
    }
 }

# http::Event
#
#	Handle input on the socket
#
# Arguments
#	token	The token returned from http::geturl
#
# Side Effects
#	Read the socket and handle callbacks.

 proc http::Event {token} {
    variable $token
    upvar 0 $token state
    set s $state(sock)

     if {[::eof $s]} {
	Eof $token
	return
    }
    if {[string equal $state(state) "header"]} {
	if {[catch {gets $s line} n]} {
	    Finish $token $n
	} elseif {$n == 0} {
	    set state(state) body
	    if {![regexp -nocase ^text $state(type)]} {
		# Turn off conversions for non-text data
		fconfigure $s -translation binary
		if {[info exists state(-channel)]} {
		    fconfigure $state(-channel) -translation binary
		}
	    }
	    if {[info exists state(-channel)] &&
		    ![info exists state(-handler)]} {
		# Initiate a sequence of background fcopies
		fileevent $s readable {}
		CopyStart $s $token
	    }
	} elseif {$n > 0} {
	    if {[regexp -nocase {^content-type:(.+)$} $line x type]} {
		set state(type) [string trim $type]
	    }
	    if {[regexp -nocase {^content-length:(.+)$} $line x length]} {
		set state(totalsize) [string trim $length]
	    }
	    if {[regexp -nocase {^([^:]+):(.+)$} $line x key value]} {
		lappend state(meta) $key $value
	    } elseif {[regexp ^HTTP $line]} {
		set state(http) $line
	    }
	}
    } else {
	if {[catch {
	    if {[info exists state(-handler)]} {
		set n [eval $state(-handler) {$s $token}]
	    } else {
		set block [read $s $state(-blocksize)]
		set n [string length $block]
		if {$n >= 0} {
		    append state(body) $block
		}
	    }
	    if {$n >= 0} {
		incr state(currentsize) $n
	    }
	} err]} {
	    Finish $token $err
	} else {
	    if {[info exists state(-progress)]} {
		eval $state(-progress) {$token $state(totalsize) $state(currentsize)}
	    }
	}
    }
}

# http::CopyStart
#
#	Error handling wrapper around fcopy
#
# Arguments
#	s	The socket to copy from
#	token	The token returned from http::geturl
#
# Side Effects
#	This closes the connection upon error

 proc http::CopyStart {s token} {
    variable $token
    upvar 0 $token state
    if {[catch {
	fcopy $s $state(-channel) -size $state(-blocksize) -command \
	    [list http::CopyDone $token]
    } err]} {
	Finish $token $err
    }
}

# http::CopyDone
#
#	fcopy completion callback
#
# Arguments
#	token	The token returned from http::geturl
#	count	The amount transfered
#
# Side Effects
#	Invokes callbacks

 proc http::CopyDone {token count {error {}}} {
    variable $token
    upvar 0 $token state
    set s $state(sock)
    incr state(currentsize) $count
    if {[info exists state(-progress)]} {
	eval $state(-progress) {$token $state(totalsize) $state(currentsize)}
    }
    # At this point the token may have been reset
    if {[string length $error]} {
	Finish $token $error
    } elseif {[catch {::eof $s} iseof] || $iseof} {
	Eof $token
    } else {
	CopyStart $s $token
    }
}

# http::Eof
#
#	Handle eof on the socket
#
# Arguments
#	token	The token returned from http::geturl
#
# Side Effects
#	Clean up the socket

 proc http::Eof {token} {
    variable $token
    upvar 0 $token state
    if {[string equal $state(state) "header"]} {
	# Premature eof
	set state(status) eof
    } else {
	set state(status) ok
    }
    set state(state) eof
    Finish $token
}

# http::wait --
#
#	See documentaion for details.
#
# Arguments:
#	token	Connection token.
# Results:
#        The status after the wait.

proc http::wait {token} {
    variable $token
    upvar 0 $token state

    if {![info exists state(status)] || [string length $state(status)] == 0} {
	# We must wait on the original variable name, not the upvar alias
	vwait $token\(status)
    }
    if {[info exists state(error)]} {
	set errorlist $state(error)
	unset state(error)
	eval error $errorlist
    }
    return $state(status)
}

# http::formatQuery --
#
#	See documentaion for details.
#	Call http::formatQuery with an even number of arguments, where 
#	the first is a name, the second is a value, the third is another 
#	name, and so on.
#
# Arguments:
#	args	A list of name-value pairs.
# Results:
#        TODO

proc http::formatQuery {args} {
    set result ""
    set sep ""
    foreach i $args {
	append result $sep [mapReply $i]
	if {[string compare $sep "="]} {
	    set sep =
	} else {
	    set sep &
	}
    }
    return $result
}

# http::mapReply --
#
#	Do x-www-urlencoded character mapping
#
# Arguments:
#	string	The string the needs to be encoded
#
# Results:
#       The encoded string

 proc http::mapReply {string} {
    variable formMap

    # The spec says: "non-alphanumeric characters are replaced by '%HH'"
    # 1 leave alphanumerics characters alone
    # 2 Convert every other character to an array lookup
    # 3 Escape constructs that are "special" to the tcl parser
    # 4 "subst" the result, doing all the array substitutions

    set alphanumeric	a-zA-Z0-9
    regsub -all \[^$alphanumeric\] $string {$formMap(&)} string
    regsub -all \n $string {\\n} string
    regsub -all \t $string {\\t} string
    regsub -all {[][{})\\]\)} $string {\\&} string
    return [subst $string]
}

# http::ProxyRequired --
#	Default proxy filter. 
#
# Arguments:
#	host	The destination host
#
# Results:
#       The current proxy settings

 proc http::ProxyRequired {host} {
    variable http
    if {[info exists http(-proxyhost)] && [string length $http(-proxyhost)]} {
	if {![info exists http(-proxyport)] || ![string length $http(-proxyport)]} {
	    set http(-proxyport) 8080
	}
	return [list $http(-proxyhost) $http(-proxyport)]
    } else {
	return {}
    }
}
