## Enable customization of scalar bar titles for array names

Settings JSON configuration files can now specify default scalar bar titles for arrays of specific names. In a settings JSON file, a `<custom title> `for `<array name>` can be specified with the following JSON structure:

```
{
  "array_lookup_tables" :
  {
    "<array name>" :
    {
      "Title" : "<custom title>"
    }
  }
}
```

Custom titles can only be read from JSON - this change does not include a mechanism to save custom titles from ParaView.
