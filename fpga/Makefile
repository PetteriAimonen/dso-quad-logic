VHDL_SOURCE = fifo.vhdl config.vhdl delay.vhdl fpga_top.vhdl

all: syntax_check run_tbs

clean:
	rm *.o *.cf

# ----- GHDL simulator -----

syntax_check: $(VHDL_SOURCE)
	ghdl -a $(GHDL_ARGS) $^

tb_fifo: fifo.vhdl tb_fifo.vhdl
	ghdl -a $(GHDL_ARGS) $^
	ghdl -e $(GHDL_ARGS) $@

tb_rle_coder: rle_coder.vhdl tb_rle_coder.vhdl
	ghdl -a $(GHDL_ARGS) $^
	ghdl -e $(GHDL_ARGS) $@

run_tbs: tb_fifo tb_rle_coder
	./tb_fifo
	./tb_rle_coder

