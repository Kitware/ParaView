set dollar1 "this string contains an unescaped dollar sign -> \\$foo"
set dollar2 "this string contains an escaped dollar sign -> \$foo \\\$foo"
set bracket1 "this contains an unescaped bracket [NoSuchProc]"
set bracket2 "this contains an escaped bracket \[NoSuchProc\]"
set bracket3 "this contains nested unescaped brackets [[NoSuchProc]]"
proc testProc {} {}
