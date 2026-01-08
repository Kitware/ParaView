## CAVE: Add per-connection transforms

Now, whenever you add or edit a connection from the CAVE configuration panel, you can also view and update the transform associated with the connection. The transform is a 4x4 homogeneous matrix used to pre-multiply all tracker matrices coming from the connection before they are delivered to the interactor styles.

Of course, these transforms are also saved to and loaded from state along with the rest of the panel configuration elements.
