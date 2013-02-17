VHDL_SOURCE = fifo.vhdl rle_coder.vhdl edge_matcher.vhdl \
	config.vhdl delay.vhdl pipelined_subtract.vhdl \
	pipelined_counter.vhdl fpga_top.vhdl

TESTBENCHES = tb_fifo tb_rle_coder tb_edge_matcher \
	tb_pipelined_subtract tb_pipelined_counter

all: syntax_check run_tbs

clean:
	rm -f *.o *.cf $(TESTBENCHES) *.ghw

# ----- GHDL simulator -----

syntax_check: $(VHDL_SOURCE)
	ghdl -a $(GHDL_ARGS) $^

tb_%: tb_%.vhdl | syntax_check
	ghdl -a $(GHDL_ARGS) $^
	ghdl -e $(GHDL_ARGS) $@

run_tbs: $(TESTBENCHES)
	$(foreach test, $^, \
	echo $(test) && \
	./$(test) --wave=$(test).ghw && \
	) true

