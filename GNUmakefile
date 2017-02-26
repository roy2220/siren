-include .makesettings

PREFIX ?= /usr/local
BUILDDIR ?= build
CXX ?=
CPPFLAGS ?=
CXXFLAGS ?=
AR ?=
ARFLAGS ?=
DEBUG ?=

override define settings :=
PREFIX = $(PREFIX)
BUILDDIR = $(BUILDDIR)
CXX = $(CXX)
CPPFLAGS = $(CPPFLAGS)
CXXFLAGS = $(CXXFLAGS)
AR = $(AR)
ARFLAGS = $(ARFLAGS)
DEBUG = $(DEBUG)
endef

$(file >.makesettings,$(settings))

override define prependflags :=
override CPPFLAGS = -iquote include -iquote src -MMD -MT $$@ -MF $$(@:%.o=%.d) $(CPPFLAGS)
override CXXFLAGS = -std=c++14 -pedantic -Wall -Wextra -Werror $(CXXFLAGS)
override ARFLAGS = rc $(ARFLAGS)
endef

$(eval $(call prependflags))

override libobjs := $(patsubst %.cc,$(BUILDDIR)/%.o,$(wildcard src/*.cc))
override testobjs := $(libobjs) $(patsubst %.cc,$(BUILDDIR)/%.o,$(wildcard test/*.cc))

override cmds := help build test install uninstall tag clean
.PHONY: $(cmds)


help:
	@echo usage: $(MAKE) { $(foreach cmd,$(cmds),$(cmd) \|) ... }


build: $(BUILDDIR)/libsiren.a


test: $(BUILDDIR)/siren-test
	$(DEBUG) $(BUILDDIR)/siren-test


install: build
	mkdir -p $(PREFIX)/lib
	cp -T $(BUILDDIR)/libsiren.a $(PREFIX)/lib/libsiren.a
	mkdir -p $(PREFIX)/include
	cp -rT include $(PREFIX)/include/siren


uninstall:
	rm -f $(PREFIX)/lib/libsiren.a
	rmdir -p --ignore-fail-on-non-empty $(PREFIX)/lib
	rm -rf $(PREFIX)/include/siren
	rmdir -p --ignore-fail-on-non-empty $(PREFIX)/include


tag:
	GTAGSFORCECPP= gtags -i


clean:
	rm -rf $(BUILDDIR)


$(BUILDDIR)/libsiren.a: $(libobjs)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^


ifneq ($(filter $(BUILDDIR)/libsiren.a build install,$(MAKECMDGOALS)),)
-include $(libobjs:%.o=%.d)
endif


$(BUILDDIR)/siren-test: $(testobjs)
	@mkdir -p $(@D)
	$(CXX) -o $@ $^ -lpthread


ifneq ($(filter $(BUILDDIR)/siren-test test,$(MAKECMDGOALS)),)
-include $(testobjs:%.o=%.d)
endif


$(BUILDDIR)/%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
