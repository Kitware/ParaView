# Add a server-side timeout command

So far the timeout value for a pvserver instance is once and for all given during its launch (example : ./pvserver --timeout=60). Time counting is then totally handled by the client, without taking into account possible changes on the server side. This adds a feature that lets the user give a command to the pvserver instance, that will be regularly called on the server side in order to directly ask it the remaining time available (example : ./pvserver --timeout-command="timeoutCommand").
