library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_edge_matcher is
end entity;

architecture testbench of tb_edge_matcher is
    signal clk, rst_n:  std_logic;
    signal data_in:     std_logic_vector(1 downto 0);
    signal data_out:    std_logic_vector(1 downto 0);
begin
    em1:  entity work.EdgeMatcher
        generic map (width_g => 2)
        port map (clk, rst_n, data_in, data_out);
    
    process
        procedure clock is
            constant PERIOD: time := 1 us;
        begin
            clk <= '0'; wait for PERIOD/2; clk <= '1'; wait for PERIOD/2;
        end procedure;
    begin
        data_in <= (others => '0');
        rst_n <= '0';
        clock;
        rst_n <= '1';
        
        -- Basic pass-through
        data_in <= "11";
        clock;
        clock;
        assert data_out = "11" report "Data should go through in 2 cycles.";
        
        -- Basic edge alignment
        data_in <= "01";
        clock;
        assert data_out = "11" report "Misaligned";
        data_in <= "00";
        clock;
        assert data_out = "00" report "Misaligned";
        
        -- Test that glitches are not removed
        data_in <= "01";
        clock;
        data_in <= "00";
        clock;
        assert data_out = "01" report "Glitch was removed";
        clock;
        assert data_out = "00" report "Glitch was elongated";
        
        report "Simulation ended" severity note;
        wait;
    end process;
end architecture;