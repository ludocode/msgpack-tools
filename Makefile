# GNU make is required.

# default target at the top
.PHONY: all
all: msgpack2json json2msgpack docs/msgpack2json.1 docs/json2msgpack.1

# include our config
include config.mk

ifeq ($(wildcard contrib/mpack/src/mpack/mpack.h),)
$(error "Run ./configure to fetch dependencies and set up the project.")
endif
ifeq ($(wildcard config.mk),)
$(error "Run ./configure to fetch dependencies and set up the project.")
endif

# Check for md2man-roff. If it's installed, we generate the man pages from
# the markdown source. If not, we use the pre-existing man pages.
HAS_MD2MAN_ROFF=$(shell which md2man-roff 2>/dev/null)

VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_PATCH = 0
ifeq ($(VERSION_PATCH),0)
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR)
else
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
endif

CPPFLAGS += -Icontrib/mpack/src \
			-Icontrib/rapidjson/include \
			-Icontrib/libb64/include -Icontrib/libb64/src
CPPFLAGS += -Wall -Wextra -Wpedantic -Wno-unused-parameter \
			-DVERSION=\"$(VERSION)\" \
			-DLIBB64_VERSION=\"$(LIBB64_VERSION)\"

# workarounds for warnings in dependencies
#CPPFLAGS += -Wno-implicit-fallthrough # libb64 has switch case fallthroughs
#CPPFLAGS += -Wno-class-memaccess # rapidjson copies some classes with memcpy()

CFLAGS = -g -fPIC -DPIC
ifeq ($(DEBUG),true)
CFLAGS += -DDEBUG -Og
else
CFLAGS += -DNDEBUG -Os
endif
LDFLAGS =

# Note: We don't clean the generated man pages. These are committed to the
# repository so that msgpack-tools can be installed without md2man.
.PHONY: clean
clean:
	rm -f msgpack2json json2msgpack
	rm -rf .build

%: src/%.cpp
	@mkdir -p .build
	$(TOOL_PREFIX)c++ $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -MMD -MF .build/$@.d -o $@ $^

ifneq ($(HAS_MD2MAN_ROFF),)
docs/json2msgpack.1: docs/json2msgpack.md
	md2man-roff $^ > $@

docs/msgpack2json.1: docs/msgpack2json.md
	md2man-roff $^ > $@
endif

# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#depdelete
DEPS := .build/msgpack2json.d .build/json2msgpack.d
$(DEPS):
-include $(DEPS)

.PHONY: install-bin
install-bin: msgpack2json json2msgpack
	install -Dt $(PREFIX)/bin $^
ifneq ($(DEBUG),1)
	strip $(patsubst %,$(PREFIX)/bin/%,$^)
endif

.PHONY: install-man
install-man: docs/msgpack2json.1 docs/json2msgpack.1
	install -Dt $(PREFIX)/share/man/man1 $^

uninstall:
	rm -f $(PREFIX)/bin/msgpack2json
	rm -f $(PREFIX)/bin/json2msgpack
	rm -f $(PREFIX)/share/man/man1/msgpack2json.1
	rm -f $(PREFIX)/share/man/man1/json2msgpack.1

.PHONY: install
install: all install-man install-bin

.PHONY: check
check: msgpack2json json2msgpack
	tools/test.sh

# alias for check
.PHONY: test
test: check
