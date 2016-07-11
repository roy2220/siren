PREFIX := /usr/local
BUILDDIR := objs

CPPFLAGS =
override CPPFLAGS += -iquote include -iquote src -MMD -MT $@ -MF $(BUILDDIR)/$*.d
CXXFLAGS =
override CXXFLAGS += -std=c++14 -pedantic -Wall -Wextra -Werror
ARFLAGS =
override ARFLAGS += rc

OBJS := $(patsubst %.cc,%.o,$(wildcard src/*.cc))
TESTOBJS := $(OBJS) $(patsubst %.cc,%.o,$(wildcard tests/*.cc))


help:
	@echo $(MAKE) help
	@echo $(MAKE) build
	@echo $(MAKE) test
	@echo $(MAKE) install
	@echo $(MAKE) uninstall
	@echo $(MAKE) clean


build: $(BUILDDIR)/libsiren.a


test: $(BUILDDIR)/test
	$(BUILDDIR)/test


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


clean:
	rm -rf $(BUILDDIR)


.PHONY: help build test install uninstall clean


$(BUILDDIR)/libsiren.a: $(addprefix $(BUILDDIR)/,$(OBJS))
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^


ifneq ($(filter build,$(MAKECMDGOALS)),)
-include $(OBJS:%.o=$(BUILDDIR)/%.d)
endif


$(BUILDDIR)/test: $(addprefix $(BUILDDIR)/,$(TESTOBJS))
	@mkdir -p $(@D)
	$(CXX) -o $@ $^


ifneq ($(filter test,$(MAKECMDGOALS)),)
-include $(TESTOBJS:%.o=$(BUILDDIR)/%.d)
endif


$(BUILDDIR)/%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
