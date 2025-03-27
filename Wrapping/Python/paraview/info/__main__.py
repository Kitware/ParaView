from . import print_env

import sys
as_json = False
if len(sys.argv) > 1 and sys.argv[1] == "--json":
  as_json = True
print_env(as_json)
