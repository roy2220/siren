PREFIX = /usr/local
BUILDDIR = objs

CPPFLAGS =
override CPPFLAGS += -iquote include -iquote src -MMD -MT $@ -MF $(BUILDDIR)/$*.d
CXXFLAGS =
override CXXFLAGS += -std=c++14 -pedantic -Wall -Wextra -Werror
ARFLAGS =
override ARFLAGS += rc


build: $(BUILDDIR)/libsiren.a


test: build $(BUILDDIR)/test
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


.PHONY: build test install uninstall clean


$(BUILDDIR)/libsiren.a: $(patsubst %.cc, $(BUILDDIR)/%.o, $(wildcard src/*.cc))
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^


ifneq ($(MAKECMDGOALS), clean)
-include $(patsubst %.cc, $(BUILDDIR)/%.d, $(wildcard src/*.cc))
endif


$(BUILDDIR)/test: $(patsubst %.cc, $(BUILDDIR)/%.o, $(wildcard tests/*.cc))
	@mkdir -p $(@D)
	$(CXX) -o $@ $^ -L $(BUILDDIR) -lsiren


ifneq ($(filter test, $(MAKECMDGOALS)), )
-include $(patsubst %.cc, $(BUILDDIR)/%.d, $(wildcard tests/*.cc))
endif


$(BUILDDIR)/%.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
