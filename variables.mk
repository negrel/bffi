TARGET_LIB ?= lib$(shell basename $(PROJECT_DIR)).a


BUILD_DIR ?= ./dist
CMD_DIR ?= ./cmd
INC_DIR ?= ./inc

export FLUX_CMD
FLUX_CMDS ?= $(wildcard $(CMD_DIR)/*)

INCS := $(shell find $(INC_DIR) -regex '.+\.h')
CMD_SRCS := $(shell find $(CMD_DIR)/$(FLUX_CMD) -regex '.+\.c')

CC := clang
CFLAGS ?=
CFLAGS := $(CFLAGS) -std=c11 -ggdb \
	-Wall -Wextra -Werror -pedantic -Wmissing-prototypes -Wstrict-prototypes \
	-I$(INC_DIR) -I$$PWD

AR := ar

MKDIR := mkdir
MKDIR_P := mkdir -p
