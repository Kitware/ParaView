# ensures paraview._version() works as expected.

from paraview import compatibility

assert (bool(compatibility.GetVersion()) == False),\
    "ParaView modules should never force backwards compatibility to any version"
assert ((compatibility.GetVersion() < (4, 1)) == False),\
    "less-than test should always fail when version is not specified."
assert ((compatibility.GetVersion() <= (4, 1)) == False),\
    "less-equal test should always fail when version is not specified."
assert ((compatibility.GetVersion() > (4, 1)) == True),\
    "greater-than test should always pass when version is not specified."
assert ((compatibility.GetVersion() >= (4, 1)) == True),\
    "greater-equal test should always pass when version is not specified."

# Now switch backwards compatibility to (4, 1)
compatibility.major = 4
compatibility.minor = 1

try:
    compatibility.GetVersion() < 4.1
    assert (False), "version comparison with floating value should fail"
except TypeError:
    assert (True)

assert ((compatibility.GetVersion() < (4, 1)) == False), "version comparison failed"
assert ((compatibility.GetVersion() <= (4, 1)) == True), "version comparison failed"
assert ((compatibility.GetVersion() > (4, 1)) == False), "version comparison failed"
assert ((compatibility.GetVersion() >= (4, 1)) == True), "version comparison failed"

assert ((compatibility.GetVersion() < (4, 10)) == True), "version comparison failed"
assert ((compatibility.GetVersion() <= (4, 10)) == True), "version comparison failed"
assert ((compatibility.GetVersion() > (4, 10)) == False), "version comparison failed"
assert ((compatibility.GetVersion() >= (4, 10)) == False), "version comparison failed"

compatibility.major = 4
compatibility.minor = 11

assert ((compatibility.GetVersion() < (4, 1)) == False), "version comparison failed"
assert ((compatibility.GetVersion() <= (4, 1)) == False), "version comparison failed"
assert ((compatibility.GetVersion() > (4, 1)) == True), "version comparison failed"
assert ((compatibility.GetVersion() >= (4, 1)) == True), "version comparison failed"

assert ((compatibility.GetVersion() < (4, 10)) == False), "version comparison failed"
assert ((compatibility.GetVersion() <= (4, 10)) == False), "version comparison failed"
assert ((compatibility.GetVersion() > (4, 10)) == True), "version comparison failed"
assert ((compatibility.GetVersion() >= (4, 10)) == True), "version comparison failed"

assert ((compatibility.GetVersion() < (4, 11)) == False), "version comparison failed"
assert ((compatibility.GetVersion() <= (4, 11)) == True), "version comparison failed"
assert ((compatibility.GetVersion() > (4, 11)) == False), "version comparison failed"
assert ((compatibility.GetVersion() >= (4, 11)) == True), "version comparison failed"

assert ((compatibility.GetVersion() < (5, 0)) == True), "version comparison failed"
assert ((compatibility.GetVersion() > (3, 12)) == True), "version comparison failed"

assert ((compatibility.GetVersion() < 5) == True), "major version comparison failed"
assert ((compatibility.GetVersion() > 3) == True), "major version comparison failed"
