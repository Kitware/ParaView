Proxy Hints {#ProxyHints}
===========

This page documents *Proxy Hints*, which are XML tags accepted under *Hints*
for a *Proxy* element in the Server-Manager configuration XMLs.

WarnOnRepresentationChange
--------------------------
Warn the user on changing to a specific representation type.

For the motivation behind this hint, see BUG #15117.
This is used to indicate to the pqDisplayRepresentationWidget that the user must
be prompted with a *'Are you sure?'* if they manually switch to this
representation from the UI.

    <RepresentationProxy ...>
      ...
      <Hints>
        <WarnOnRepresentationChange value="Volume" />
      </Hints>
    </RepresentationProxy>
