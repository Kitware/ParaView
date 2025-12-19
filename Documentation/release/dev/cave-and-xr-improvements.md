## Improvement to collaboration between HMD and CAVE

Collaborators using the XRInterface plugin now send their avatar up vector based on their own navigation. For example, using the in-world ParaView menu to select camera presets results in navigation, and previously this caused collaborators to see your avatars head suddenly appear at a 90 or 180 angle to its torso. Now, your avatars head and torso always appear in proper alignment.

To go along with this change, the CAVEInteraction plugin no longer has a field to allow specifying the default avatar up vector for new avatars coming into the session, since this is now controlled on the sending side.

## CAVEInteraction UI improvements

The list edit for "VR Connections" took up too much space, given that the vast majority of users will define only one or two connections. So this field has been fixed in size at the height needed to display 2.5 list items, making it clear if you need to scroll to see more items.

Similarly, the "Interactions" list edit has been fixed in size at the height needed to display 4.5 items.

The entire configuration panel is now resizable and lives within a scroll area, so that even when quite small, all the contents are reachable.

The regular expression for connection address validation has been updated several times in the past to be more inclusive of characters that might appear in an address, yet users still have issues with legitimate characters not being allowed in the field. Rather than continue to grow the size of that regular expression, validation of that field has simply been removed.

Navigation is now shared by default by CAVEInteraction users who join a collaboration session, though this behavior can be disabled using the same checkbox as before.
