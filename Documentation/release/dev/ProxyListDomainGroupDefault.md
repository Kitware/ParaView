## ProxyListDomain support default for group

Using the following syntax

```
    <ProxyListDomain name="proxy_list">
      <Group name="sources" default="SphereSource" />
    </ProxyListDomain>
```

A proxy list domain containing a group can define the default proxy to use

Most filter using ProxyListDomain have been converted to use group and default
in order for it to be easier to add new proxy to a proxy list domain trough plugins.

A example plugin demonstrating this, AddToProxyGroupXMLOnly.
