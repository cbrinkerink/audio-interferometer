#----------------------------------------
#-- Establecer nombre del componente
#----------------------------------------
#NAME = full-correlator-ethernet
NAME = full-correlator-ethernet-scalable
DEPS = liteeth_core.v

#-------------------------------------------------------
#-- Objetivo por defecto: hacer simulacion y sintesis
#-------------------------------------------------------
all: sim sint
	
#----------------------------------------------
#-- make sim
#----------------------------------------------
#-- Objetivo para hacer la simulacion del
#-- banco de pruebas
#----------------------------------------------
sim: $(NAME)_tb.vcd
	
#-----------------------------------------------
#-  make sint
#-----------------------------------------------
#-  Objetivo para realizar la sintetis completa
#- y dejar el diseno listo para su grabacion en
#- la FPGA
#-----------------------------------------------
sint: $(NAME).bin
	
#-------------------------------
#-- Compilation and simulation
#-------------------------------
$(NAME)_tb.vcd: $(NAME).v $(DEPS) $(NAME)_tb.v
	
	#-- Compile
	iverilog -o $(NAME)_tb.out $^	
	#-- Simulate
	vvp ./$(NAME)_tb.out

#------------------------------
#-- Full synthesis
#------------------------------
$(NAME).bin: $(NAME).lpf $(NAME).v $(DEPS)
	
	#-- Synthesis
	# For ECP5, the synth_ecp5 option will be needed.
	#yosys -p "synth_ice40 -blif $(NAME).blif" $(NAME).v $(DEPS)
	#yosys -l $(NAME)-yosys.log -p "synth_ecp5 -top top -json $(NAME).json" $(NAME).v $(DEPS)
	#yosys -l $(NAME)-yosys.log -p "connect_rpc -exec python3 ../ecp5pll.py" -p "synth_ecp5 -top top -json $(NAME).json" -p "write_verilog pll-test.v" $(NAME).v $(DEPS)
	yosys -l $(NAME)-yosys.log -p "synth_ecp5 -top top -json $(NAME).json" -p "flatten; opt -fast" $(NAME).v $(DEPS)
	#yosys -p "synth_ecp5 -blif $(NAME).blif" $(NAME).v $(DEPS)
	
	#-- Place & route
	#arachne-pnr -o $(NAME).txt -d 1k -p $(NAME).pcf $(NAME).blif
	nextpnr-ecp5 -l $(NAME)-nextpnr.log --um5g-45k --json $(NAME).json --lpf $(NAME).lpf --textcfg $(NAME).txt --package CABGA381 --timing-allow-fail 2> $(NAME)-nextpnr.log
	
	#-- Generate final bitfile, download to FPGA
	#icepack $(NAME).txt $(NAME).bin
	ecppack --input $(NAME).txt --bit $(NAME).bin
	echo "### WARNING: writing to SRAM! No persistent bitfile is stored on the FPGA board. ###"
	ecpprog -S $(NAME).bin

#-- Clear all
clean:
	rm -f $(NAME).bin $(NAME).txt $(NAME).blif $(NAME).out $(NAME).vcd $(NAME)~

.PHONY: all clean

