include variables.mk

.PHONY: build/all
build/all: $(FLUX_CMDS:$(CMD_DIR)/%=build/%)

build/%: FLUX_CMD=$*
build/%:
	$(MAKE) $(BUILD_DIR)/$(FLUX_CMD)

$(BUILD_DIR)/$(FLUX_CMD): $(CMD_SRCS)
	$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

.PHONY: compile_flags.txt
compile_flags.txt:
	echo $(CFLAGS) | tr ' ' '\n' > compile_flags.txt

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)
