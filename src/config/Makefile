it: command include library library.so


s6-portable-utils-config.h: stupid s6pu-version.stupid s6pu-config-header s6pu-config-nosp s6pu-config-sp.stupid s6pu-config-footer
	{ \
	  cat s6pu-config-header ; \
	  ./stupid < s6pu-version.stupid ; \
	  if test -f flag-slashpackage ; then ./stupid < s6pu-config-sp.stupid ; else cat s6pu-config-nosp ; fi ; \
	  cat s6pu-config-footer ; \
	} > s6-portable-utils-config.h


command:

include: s6-portable-utils-config.h

library:

library.so:
