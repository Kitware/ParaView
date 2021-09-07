## PVSC updates

ParaView server configuration files (PVSC) now support two new predefined
variables:

1. `PV_APPLICATION_DIR`: This is set to the directory containing the application
   executable. For macOS app bundles, for example, this will be inside the
   bundle. This is simply `QCoreApplication::applicationDirPath`.
2. `PV_APPLICATION_NAME`: This is set to the name of the Qt application as
   specified during application initialization. This is same as
   `QCoreApplication::applicationName`.
