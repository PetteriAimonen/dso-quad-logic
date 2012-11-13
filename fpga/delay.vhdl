-- This is a simple delay line that delays the input by N clock cycles.

library ieee;
use ieee.std_logic_1164.all;

entity Delay is
    generic (
        width_g: natural; -- Width (in bits) of the data items.
        delay_g: natural  -- Number of clock cycles to delay.
    );

    port (
        clk:            in std_logic;
        rst_n:          in std_logic;

        data_in:        in std_logic_vector(width_g - 1 downto 0);

        data_out:       out std_logic_vector(width_g - 1 downto 0)
    );
end entity;

architecture rtl of Delay is
    subtype DelayEntry is std_logic_vector(width_g - 1 downto 0);
    type DelayArray is array(delay_g - 1 downto 0) of DelayEntry;
    signal delayline_r: DelayArray;

begin
    data_out <= delayline_r(0);

    process(clk, rst_n)
    begin
        if rst_n = '0' then
            delayline_r <= (others => (others => '0'));
        elsif rising_edge(clk) then
            delay_line: for i in 0 to delay_g - 2 loop
                delayline_r(i) <= delayline_r(i + 1);
            end loop delay_line;
            
            delayline_r(delay_g - 1) <= data_in;
        end if;
    end process;
end architecture;