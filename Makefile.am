AUTOMAKE_OPTIONS = subdir-objects serial-tests
ACLOCAL_AMFLAGS=-I m4

### Module and test library

TWRPABX_DOC = man/twrpabx.1

dist_man_MANS = $(TWRPABX_DOC)

TWRPABX_SRC = src/twadbstream.h src/twrpabx.h src/twrpabx.cpp

bin_PROGRAMS = twrpabx

twrpabx_SOURCES = $(TWRPABX_SRC) src/main.cpp

### Tests
TEST_NAMES = \
	test_twrpabx 

TESTS = $(TEST_NAMES)

check_PROGRAMS = $(TEST_NAMES)

TEST_CFLAGS = $(AM_CFLAGS) -DTWRPABX_TEST

# Test template
test_twrpabx_CFLAGS = $(TEST_CFLAGS)
test_twrpabx_CPPFLAGS = -I$(top_srcdir)/src
test_twrpabx_LDADD = $(TEST_LDFLAGS)
test_twrpabx_SOURCES = tests/test_twrpabx.cpp $(TWRPABX_SRC)

.PHONY: doc

doc: $(PAM_ACCESS_OSX_DOC)

man/twrpabx.1: man/twrpabx.1.md
	pandoc --standalone --to man man/twrpabx.1.md > man/twrpabx.1

