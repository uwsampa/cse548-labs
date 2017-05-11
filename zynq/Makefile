# Directories
BUILD_DIR = build
SCRIPT_DIR = tcl

# Executables
VIVADO_HLS = vivado_hls
VIVADO = vivado

.PHONY: all setup ip bit clean

all: setup ip bit

setup:
	if [ -d $(BUILD_DIR) ]; then echo "/build directory already exists. Please enter: make clean"; exit; fi
	mkdir $(BUILD_DIR)

ip:
	cd $(BUILD_DIR) && $(VIVADO_HLS) -f ../$(SCRIPT_DIR)/hls.tcl

bit:
	cd $(BUILD_DIR) && $(VIVADO) -mode tcl -source ../$(SCRIPT_DIR)/classifier.tcl -nojournal -nolog

clean:
	rm -rf $(BUILD_DIR)