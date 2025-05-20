## Avoid deadlock when outputting messages

There were conditions that under some circumstances would cause deadlock when a
message was written to the output. The problem was that the output window's
display text method would first lock a mutex and then send some Qt signals to
handle the message. However, it was possible for one of the slots attached to
the signal to send its own message. (In my particular observation, this happened
when the dock for the output messages was shown.) That would cause the display
text method to be reentrant. When it tried to grab the mutex (again), it would
lock because it already locked the mutex higher up the stack.

Fixed this problem by introducing a thread-local variable that tracks when we
are in the display text function. The variable is set to true on entry and false
on exit. Before setting to true, the variable is checked to see if it is already
true. If so, that means the function was called recursively and is about to
deadlock on the mutex (and could potentially have an infinite loop if the mutex
was not there.) On this condition, the method returns immediately to prevent any
of these conditions.
