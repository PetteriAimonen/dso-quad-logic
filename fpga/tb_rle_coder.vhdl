library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_rle_coder is
end entity;

architecture testbench of tb_rle_coder is
    signal clk, rst_n:  std_logic;
    signal data_in:     std_logic_vector(3 downto 0);
    signal data_out:    std_logic_vector(15 downto 0);
    signal write:       std_logic;
begin
    rle1:  entity work.RLECoder
        port map (clk, rst_n, data_in, data_out, write);
    
    process
        procedure clock is
            constant PERIOD: time := 1 us;
        begin
            clk <= '0'; wait for PERIOD/2; clk <= '1'; wait for PERIOD/2;
        end procedure;
    begin
        rst_n <= '0';
        data_in <= "0000";
        clock;
        rst_n <= '1';
        
        clock;
        clock;

        data_in <= "1010";
        
        clock;
        
        assert write = '1' and data_out = X"002A" report "Wrong output pair";
        
        clock;
        
        assert write = '0' report "Unexpected write";

        for i in 1 to 255 loop
            clock;
        end loop;
        
        data_in <= "1111";
        
        clock;
        
        assert write = '1' and data_out = X"100F" report "Wrong output pair";
        
        report "Simulation ended" severity note;
        wait;
    end process;
end architecture;