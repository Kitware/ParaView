package provide football 1.0
    
namespace eval ::pro:: {
    #
    # export only public functions.
    #
    namespace export {[a-z]*}
}
namespace eval ::college:: {
    #
    # export only public functions.
    #
    namespace export {[a-z]*}
}

proc ::pro::team {} {
    puts "go packers!"
    return true
}

proc ::college::team {} {
    puts "go badgers!"
    return true
}

