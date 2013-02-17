-- Convert RLE pairs (timestamp, new_levels) into delta pairs
-- (timedelta, leveldelta). These are more compressible because
-- the common bit lengths repeat for both low-to-high and high-to-low
-- transitions.
--
-- This block has a latency of 2 cycles.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity RLEToDelta is
    generic (
        timestamp_width_g:      natural := 64;
        levels_width_g:         natural := 4
    );
    
    port (
        clk:            in std_logic;
        rst_n:          in std_logic;
        
        timestamp_in:   in std_logic_vector(timestamp_width_g - 1 downto 0);
        levels_in:      in std_logic_vector(levels_width_g - 1 downto 0);
        write_in:       in std_logic;
        
        timedelta_out:  out std_logic_vector(timestamp_width_g - 1 downto 0);
        leveldelta_out: out std_logic_vector(levels_width_g - 1 downto 0);
        write_out:      out std_logic;
    );
end entity;

architecture rtl of RLEToDelta is
    signal prev_timestamp_r: std_logic_vector(timestamp_width_g - 1 downto 0);
    signal prev_levels_r:    std_logic_vector(levels_width_g - 1 downto 0);
    signal level_delta_r:    std_logic_vector(levels_width_g - 1 downto 0);
    signal write_r:          std_logic;
begin
    sub1: entity work.PipelinedSubtract
        generic map (width_g => timestamp_width_g, parts_g => 4)
        port map (clk, rst_n, timestamp_in, prev_timestamp_r, timedelta_out);
    
    process (rst_n, clk) is
    begin
        if rst_n = '0' then
            prev_timestamp_r <= (others => '0');
            prev_levels_r <= (others => '0');
            level_delta_r <= (others => '0');
        elsif rising_edge(clk) then
            if write_in = '1' then
                prev_timestamp_r <= timestamp_in;
                prev_levels_r <= levels_in;
            end if;
                
            level_delta_r <= levels_in xor prev_levels_r;
            leveldelta_out <= level_delta_r;
            
            write_out <= write_r;
            write_r <= write_in;
        end if;
    end process;
end architecture;
