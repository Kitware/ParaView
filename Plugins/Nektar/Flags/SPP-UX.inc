message  += You may want to to specify CACHE (eg. 256) or TARGSPP (eq. spp1200).
CC       = cc
ifdef CACHE
override CACHE  := -cache $(CACHE)
endif
ifdef TARGSPP
override TARGSPP := -tm $(TARGSPP)
endif
options += -O2 $(CACHE) $(TARGSPP) -ds -mrl -or all # -ipo # -alias no_addr -alias no_global -alias array_args -alias ptr_args -alias restrict_args
LIBDIR  = $(PRISM)/lib/$(ARCH)/Convex
LIBS    = -L$(LIBDIR) -lspec -lblas -lvec -lveclib
