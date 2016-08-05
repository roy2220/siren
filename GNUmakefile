PREFIX = /usr/local
BUILDDIR = objs

CPPFLAGS =
override CPPFLAGS += -iquote include -iquote src -MMD -MT $@ -MF $(BUILDDIR)/$*.d
CXXFLAGS =
override CXXFLAGS += -std=c++14 -pedantic -Wall -Wextra -Werror
ARFLAGS =
override ARFLAGS += rc

objs := $(patsubst %.cc,%.o,$(wildcard src/*.cc))
testobjs := $(objs) $(patsubst %.cc,%.o,$(wildcard tests/*.cc))

cmds := help build test install uninstall tag clean
.PHONY: $(cmds)


help:
	@echo usage: $(MAKE) { $(foreach cmd,$(cmds),$(cmd) \|) ... }


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


tag:
	GTAGSFORCECPP= gtags -i


clean:
	rm -rf $(BUILDDIR)


$(BUILDDIR)/libsiren.a: $(addprefix $(BUILDDIR)/,$(objs))
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^


ifneq ($(filter build,$(MAKECMDGOALS)),)
-include $(objs:%.o=$(BUILDDIR)/%.d)
endif


$(BUILDDIR)/test: $(addprefix $(BUILDDIR)/,$(testobjs))
	@mkdir -p $(@D)
	$(CXX) -o $@ $^ -pthread


ifneq ($(filter test,$(MAKECMDGOALS)),)
-include $(testobjs:%.o=$(BUILDDIR)/%.d)
endif


$(BUILDDIR)/%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
