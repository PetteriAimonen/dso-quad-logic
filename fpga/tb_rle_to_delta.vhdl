library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_rle_to_delta is
end entity;

architecture testbench of tb_rle_to_delta is
    signal clk, rst_n:          std_logic;
    signal timestamp_in:        std_logic_vector(15 downto 0);
    signal levels_in:           std_logic_vector(3 downto 0);
    signal write_in:            std_logic;
    signal timedelta_out:       std_logic_vector(15 downto 0);
    signal leveldelta_out:      std_logic_vector(3 downto 0);
    signal write_out:           std_logic;
begin
    conv1: entity work.RLEToDelta
        generic map (timestamp_width_g => 16, levels_width_g => 4)
        port map (clk, rst_n, timestamp_in, levels_in, write_in,
                              timedelta_out, leveldelta_out, write_out);

    process
        procedure clock is
            constant PERIOD: time := 1 us;
        begin
            clk <= '0'; wait for PERIOD/2; clk <= '1'; wait for PERIOD/2;
        end procedure;
    begin
        rst_n <= '0';
        timestamp_in <= (others => '0');
        levels_in <= (others => '0');
        clock;
        rst_n <= '1';
        
        timestamp_in <= X"0001";
        levels_in <= "0000";
        clock;
        
        assert write_out = '0';
        timestamp_in <= X"0005";
        levels_in <= "0001";
    
    end process;
end architecture;