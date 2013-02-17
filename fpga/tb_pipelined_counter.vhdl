library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_pipelined_counter is
end entity;

architecture testbench of tb_pipelined_counter is
    signal clk, rst_n:  std_logic;
    signal clear:       std_logic;
    signal enable:      std_logic;
    signal count:       std_logic_vector(15 downto 0);
    signal tick:        std_logic;
begin
    cnt1:       entity work.PipelinedCounter
        generic map (width_g => 16, parts_g => 4)
        port map (clk, rst_n, clear, enable, count, tick);
    
    process
        procedure clock is
            constant PERIOD: time := 1 us;
        begin
            clk <= '0'; wait for PERIOD/2; clk <= '1'; wait for PERIOD/2;
        end procedure;
    begin
        rst_n <= '0';
        clear <= '0';
        enable <= '0';
        clock;
        rst_n <= '1';
        enable <= '1';
        
        for i in 0 to 65535 loop
            assert unsigned(count) = to_unsigned(i, 16)
                report "Wrong count, " &
                    integer'image(to_integer(unsigned(count))) &
                    " expected " & integer'image(i);
            
            assert tick = '0' report "Unexpected tick";
        
            clock;
        end loop;
        
        assert count = X"0000" report "Expected to roll over";
        assert tick = '1' report "Expected tick";
        
        clock;
        
        assert count = X"0001";
        
        clear <= '1';
        clock;
        clear <= '0';
        
        assert count = X"0000" report "Counter should clear";
        
        clock;
        
        assert count = X"0001";
        
        report "Simulation ended" severity note;
        wait;
    end process;
end architecture;
