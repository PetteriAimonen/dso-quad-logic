-- This block breaks a wide counter into pipelined parts.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity PipelinedCounter is
    generic (
        width_g: natural := 64; -- Must be divisible by parts_g.
        parts_g: natural := 4
    );
    
    port (
        clk:            in std_logic;
        rst_n:          in std_logic;
        
        clear:          in std_logic;
        enable:         in std_logic;
        
        count:          out std_logic_vector(width_g - 1 downto 0);
        tick:           out std_logic
    );
end entity;

architecture rtl of PipelinedCounter is
    constant part_width_c:      natural := width_g / parts_g;
    signal almost_tick_r:       std_logic_vector(parts_g - 1 downto 0);
    signal count_r:             std_logic_vector(width_g - 1 downto 0);
begin
    count <= count_r;

    process (clk, rst_n)
        variable part_v:        unsigned(part_width_c - 1 downto 0);
        variable tick_v:        std_logic;
    begin
        if rst_n = '0' then
            count_r <= (others => '0');
            almost_tick_r <= (others => '0');
            tick <= '0';
        elsif rising_edge(clk) then
            
            tick_v := enable;
            for i in 0 to parts_g - 1 loop
                part_v := unsigned(count_r((i + 1) * part_width_c - 1 downto i * part_width_c));
                
                if tick_v = '1' then
                    -- Value is max - 1?
                    if part_v = to_unsigned(2**part_width_c - 2, part_width_c) then
                        almost_tick_r(i) <= '1';
                    else
                        almost_tick_r(i) <= '0';
                    end if;
                
                    part_v := part_v + 1;
                end if;
                
                count_r((i + 1) * part_width_c - 1 downto i * part_width_c) <=
                    std_logic_vector(part_v);
                
                tick_v := tick_v and almost_tick_r(i);
            end loop;
            tick <= tick_v;
            
            if clear = '1' then
                count_r <= (others => '0');
                almost_tick_r <= (others => '0');
                tick <= '0';
            end if;
        end if;
    end process;
end architecture;
