typeset
=======

This project is used to experiment with using traditional typesetting
techniques in fixed-column terminals, which also obviously use fixed-width
fonts.

This is still a work in progress (i.e., cannot be used with any actual user
input as of yet--everything is currently hard-coded).

Main features:
* Text justification, using a distributed average of spacing.
* Paragraph indentation.
* Initial support for hanging punctuation (currently only on right-side)

Planned features:
* Read from stdin, files, etc.
* "list" indentation (i.e., lines beginning with "a.) ...", "* ...", etc. being
  indented)
* Unicode list bullet point replacement
* Better support for hanging punctuation.
