TARGET = example-hal

CPP_SOURCES = main.cpp

LIBDAISY_DIR = ../libDaisy

# Override OCD / OCD_DIR on the command line if openocd isn't on your PATH,
# e.g.: make OCD_DIR=/path/to/openocd/scripts program
ifdef TEST
C_DEFS += -DDASHBOARD_TEST
endif

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

.PHONY: test
test:
	$(MAKE) TEST=1
