## Adds a multi-threaded callback queue

ParaView now instantiates a multi-threaded callback queue per process. It is accessible from
`vtkProcessModule::GetCallbackQueue()` and is an instance of `vtkThreadedCallbackQueue`.
One can push functions and arguments to this queue and the queue will execute them on the threads
that are spawned by the queue in the background.
Saving screenshots and images can now use this queue by setting the property `SaveInBackground` of
the proxy `SaveScreenshot` to true. The user should be mindful that when using this feature, reading
the screenshot that we are saving in the background needs synchronization so the file cannot be read
before it is finished being written. This can be done using the new static function
`vtkRemoteWriterHelper::Wait(const std::string& fileName)`. Just provide the file name using either
a relative or absolute path, and this function will terminate once the file is written.

The number of threads can be set in the Settings in the `General` section. This is an advanced
property named `NumberOfCallbackThreads`.
