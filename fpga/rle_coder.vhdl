-- Monitor transitions in input data and code them as time_delta, new_state
-- pairs, all packed in 16 bit value. If counter is about to overflow,
-- post a pair even if there is no transition.
--
-- data_out(3 downto 0) = new signal levels
-- data_out(15 downto 4) = time between previous and this event

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity RLECoder is
    port (
        clk:            in std_logic;
        rst_n:          in std_logic;

        -- Data from input channels
        data_in:        in std_logic_vector(3 downto 0);

        -- RLE-encoded output data
        data_out:       out std_logic_vector(15 downto 0);
        write:          out std_logic
    );
end entity;

architecture rtl of RLECoder is
    signal levels_r:    std_logic_vector(3 downto 0);
    signal time_r:      unsigned(11 downto 0);

    signal data_out_r:  std_logic_vector(15 downto 0);
    signal write_r:     std_logic;
begin
    data_out <= data_out_r;
    write <= write_r;

    process(clk, rst_n)
    begin
        if rst_n = '0' then
            levels_r <= (others => '0');
            time_r <= (others => '0');
            data_out_r <= (others => '0');
            write_r <= '0';
        elsif rising_edge(clk) then
            if data_in /= levels_r or time_r = 4095 then
                -- Output new pair
                data_out_r <= std_logic_vector(time_r) & data_in;
                write_r <= '1';
                
                -- Store new levels and reset time
                levels_r <= data_in;
                time_r <= (others => '0');
            else
                time_r <= time_r + 1;
                write_r <= '0';
            end if;
        end if;
    end process;
end architecture;


