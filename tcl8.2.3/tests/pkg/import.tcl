package provide fubar 1.0
    
namespace eval ::fubar:: {
    #
    # export only public functions.
    #
    namespace export {[a-z]*}
}

proc ::fubar::foo {bar} {
    puts "$bar"
    return true
}

namespace import ::fubar::foo

