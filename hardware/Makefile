# Hardware (KiCad) Makefile
#
# Automates export of fabrication outputs and documentation from KiCad CLI.
# Requires KiCad 8.x+ CLI tools (kicad-cli) in PATH.

DESIGN_DIR := design-files
FAB_DIR    := fabrication
DOC_DIR    := documentation

PCB_FILE   := $(DESIGN_DIR)/gateway.kicad_pcb
SCH_FILE   := $(DESIGN_DIR)/gateway.kicad_sch

.DEFAULT_GOAL := help

.PHONY: help gerbers drill schematic-pdf zip all clean

help: ## Show this help message
	@echo ""
	@echo "Hardware (KiCad) targets:"
	@echo ""
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2}'
	@echo ""

all: gerbers drill zip schematic-pdf render ## Export all fabrication and documentation outputs

gerbers: ## Export Gerber files from PCB layout
	@mkdir -p $(FAB_DIR)
	kicad-cli pcb export gerbers $(PCB_FILE) -o $(FAB_DIR)/

drill: ## Export drill files from PCB layout
	@mkdir -p $(FAB_DIR)
	kicad-cli pcb export drill $(PCB_FILE) -o $(FAB_DIR)/

zip: gerbers drill ## Package Gerbers + drill into a single zip for fab upload
	@cd $(FAB_DIR) && zip -FS gateway_gerbers_v1.0.zip *.gbr *.gbl *.gtl *.gbs *.gts *.gbo *.gto *.gbp *.gtp *.gba *.gta *.gm1 *.gbrjob *.drl 2>/dev/null; true
	@echo "Created $(FAB_DIR)/gateway_gerbers_v1.0.zip"

render: ## Export 3D top render of assembled board (high-res PNG)
	@mkdir -p $(DOC_DIR)
	kicad-cli pcb render $(PCB_FILE) --side top --quality high --width 3000 --height 2100 -o $(DOC_DIR)/pcb_top_render.png

schematic-pdf: ## Export combined schematic PDF
	@mkdir -p $(DOC_DIR)
	kicad-cli sch export pdf $(SCH_FILE) -o $(DOC_DIR)/schematic_v1.0.pdf

clean: ## Remove generated fabrication and documentation outputs
	rm -f $(FAB_DIR)/*.gbr $(FAB_DIR)/*.gbl $(FAB_DIR)/*.gtl $(FAB_DIR)/*.gbs $(FAB_DIR)/*.gts
	rm -f $(FAB_DIR)/*.gbo $(FAB_DIR)/*.gto $(FAB_DIR)/*.gbp $(FAB_DIR)/*.gtp $(FAB_DIR)/*.gba $(FAB_DIR)/*.gta
	rm -f $(FAB_DIR)/*.gm1 $(FAB_DIR)/*.gbrjob $(FAB_DIR)/*.drl $(FAB_DIR)/*.zip
	rm -f $(DOC_DIR)/schematic_v1.0.pdf
