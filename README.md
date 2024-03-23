UZ80 is a cross-platform Z80 assembler that aims to be small and fast while still providing valuable services such as macro instructions and conditional assembly.

Internally, it departs from full two-pass methods and relies instead on assembling as much code as possible during the first pass and writing down the lines making references to still unseen symbols; these lines will be assembled when all symbols are known.

Its original home page is http://cngsoft.no-ip.org/uz80.htm but the server isn't as reliable as it used to be, so this will be its second home.
