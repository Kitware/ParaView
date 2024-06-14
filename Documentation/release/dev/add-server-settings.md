## Add support for server settings

Settings are now supported for `pvserver`, `pvrenderserver` and `pvdataserver`.

These executable now parse a file named `ParaViewServer-UserSettings.json` and look for settings named according to `pvserver` CLI connection options.

This file is expected to look like this:

```
{
  "cli-options" :
  {
    "connection" :
    {
      "connect-id" : 17
      "reverse-connection" : true
      "client-host" : "bailey"
    }
  }
}
```

The name of the setting is the "long" version of the option, without the two leading minuses.

**only connection options are supported.**
