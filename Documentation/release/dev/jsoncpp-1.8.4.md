# jsoncpp-1.8.4


* There was a recent API change in jsoncpp-1.8.4, which changed all
  `removeMember()` methods of `Json::Value` to either return `bool`
  or being `void` type. The now used overloaded `removeMember()`
  method is available since jsoncpp-0.8.0 and returns the removed
  value in a pre-allocated buffer instead of returning a new created
  `Json::Value` object. The underlying semantics of the lifetime and
  memory allocation of the used buffer are kept the same way as before.
