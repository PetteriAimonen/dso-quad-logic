-- Monitor transitions in input data and code them as time_delta, new_state
-- pairs, all packed in 16 bit value. If counter is about to overflow,
-- post a pair even if there is no transition.
--
-- data_out(15) is data type. 0 = time + data, 1 = time only
-- 
-- time + data:
-- data_out(3 downto 0) = new signal levels
-- data_out(14 downto 4) = time between previous and this event
--
-- time only:
-- data_out(14 downto 0) = top 15 bits of time, multiply by 2048.

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
    signal time_r:      unsigned(25 downto 0);

    -- If we are writing out a value >= 2^12, we need
    -- two 16-bit words. However, we can only write
    -- one per clock cycle. Therefore store the other
    -- in a temporary buffer. It is guaranteed that
    -- there will be an empty slot before next dual-word
    -- value will have to be written.
    signal temp_r:      std_logic_vector(15 downto 0);
    signal temp_full_r: std_logic;
    
    -- Registers for block outputs
    signal data_out_r:  std_logic_vector(15 downto 0);
    signal write_r:     std_logic;
begin
    data_out <= data_out_r;
    write <= write_r;

    process(clk, rst_n)
        procedure output_temp is
        begin
            data_out_r <= temp_r;
            write_r <= temp_full_r;
            temp_r <= (others => '-');
            temp_full_r <= '0';
        end procedure;
    begin
        if rst_n = '0' then
            levels_r <= (others => '0');
            time_r <= (others => '0');
            data_out_r <= (others => '0');
            write_r <= '0';
            temp_r <= (others => '0');
            temp_full_r <= '0';
        elsif rising_edge(clk) then
            if data_in /= levels_r then
                -- If it was a large time delta, output extra time segment.
                -- In this situation it is guaranteed that temp_full_r was 0.
                if time_r >= 2048 then
                    assert temp_full_r = '0' report "Overwrote temp_r!";
                    data_out_r <= "1" & std_logic_vector(time_r(25 downto 11));
                    write_r <= '1';
                else
                    output_temp;
                end if;
                
                -- Output new pair
                temp_r <= "0" & std_logic_vector(time_r(10 downto 0)) & data_in;
                temp_full_r <= '1';
                
                -- Store new levels and reset time
                levels_r <= data_in;
                time_r <= (others => '0');
                
            elsif time_r = "11111111111111011111111111" then
                -- Output 0xFFFF time segment
                -- Cutoff time is chosen so that we can neatly reset time_r
                -- to 0. Also it guarantees that there is no double-word
                -- write on the next cycle.
                temp_r <= X"FFFF";
                temp_full_r <= '1';
                time_r <= (others => '0');
                
                output_temp;
            else
                time_r <= time_r + 1;
                output_temp;
            end if;
        end if;
    end process;
end architecture;


