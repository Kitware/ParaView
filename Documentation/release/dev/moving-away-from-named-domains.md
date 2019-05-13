# Moving away from named domains

The name of domains in xml property used to have an impact
on how this property will be shown in the UI. This was quite error prone.
We have now completelly moved away from named domains, the name of the domain in the xml property
does not matter, with the notable exeception of :

InputArrayDomain name still matters has it can then be used in an ArrayListDomain.

glyph_scale_factor panel widget still requires the usage of two domains named scalar_range and vector_range
This is error prone and will be corrected in the future with a domain dedicated to it.
