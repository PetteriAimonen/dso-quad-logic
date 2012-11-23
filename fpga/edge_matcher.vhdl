-- The edges on different channels are often slightly mismatched.
-- This unnecessarily wastes storage space. Also the matching
-- between channels is never 1-cycle accurate so we can safely
-- align the edges without losing any meaningful information.
-- However, a single-cycle glitch will not be removed.
--
-- __|^^^^|___           __|^^^|____
-- ___|^^|____   gives   __|^^^|____
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity EdgeMatcher is
    generic (
        width_g:        natural
    );

    port (
        clk:            in std_logic;
        rst_n:          in std_logic;

        data_in:        in std_logic_vector(width_g - 1 downto 0);
        data_out:       out std_logic_vector(width_g - 1 downto 0)
    );
end entity;

architecture rtl of EdgeMatcher is
    -- New input data enters the output register through an intermediate
    -- staging register. If two edges on different channels occur on
    -- consecutive clock cycles, the later data sample bypasses the
    -- staging register.
    signal staging_r:   std_logic_vector(width_g - 1 downto 0);
    signal output_r:    std_logic_vector(width_g - 1 downto 0);
    
    constant zeros_c:   std_logic_vector(width_g - 1 downto 0) := (others => '0');
begin
    data_out <= output_r;

    process(clk, rst_n)
    begin
        if rst_n = '0' then
            staging_r <= (others => '0');
            output_r <= (others => '0');
        elsif rising_edge(clk) then
            staging_r <= data_in;

            if staging_r /= data_in and output_r /= staging_r then
                -- Ok, two changes in consecutive cycles.
                -- Check for glitch.
                if ((staging_r xor data_in) and
                    (output_r xor staging_r)) /= zeros_c then
                    -- There was a glitch, don't bypass.
                    output_r <= staging_r;
                else
                    -- All ok, bypass the staging register.
                    output_r <= data_in;
                end if;
            else
                -- No consecutive changes, go through staging as usual.
                output_r <= staging_r;
            end if;
        end if;
    end process;
end architecture;
