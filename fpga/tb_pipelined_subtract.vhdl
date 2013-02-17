library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_pipelined_subtract is
end entity;

architecture testbench of tb_pipelined_subtract is
    signal clk, rst_n:  std_logic;
    signal value0_in:   std_logic_vector(63 downto 0);
    signal value1_in:   std_logic_vector(63 downto 0);
    signal result_out:  std_logic_vector(63 downto 0);
begin
    sub1:       entity work.PipelinedSubtract
        port map (clk, rst_n, value0_in, value1_in, result_out);
    
    process
        procedure clock is
            constant PERIOD: time := 1 us;
        begin
            clk <= '0'; wait for PERIOD/2; clk <= '1'; wait for PERIOD/2;
        end procedure;
    begin
        value0_in <= (others => '0');
        value1_in <= (others => '0');
        rst_n <= '0';
        clock;
        rst_n <= '1';
        
        value0_in <= X"0101020203030404";
        value1_in <= X"0001020203030405";
        clock;
        
        value0_in <= X"1122334455667788";
        value1_in <= X"0000000000000000";
        clock;
        
        assert result_out = X"00FFFFFFFFFFFFFF";
        
        clock;
        
        assert result_out = X"1122334455667788";
        
        report "Simulation ended" severity note;
        
        wait;
    end process;
end architecture;
