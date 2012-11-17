-- This is a 16-bit configuration register for setting operation mode.
--
-- Configuration register bits:
--  0: Read FIFO data (0) or FIFO count (1)

library ieee;
use ieee.std_logic_1164.all;

entity Config is
    port (
        clk:            in std_logic;
        rst_n:          in std_logic;

        fsmc_ce:        in std_logic;
        fsmc_nwr:       in std_logic;
        fsmc_db:        in std_logic_vector(15 downto 0);

        cfg_read_count: out std_logic 
    );
end entity;

architecture rtl of Config is
    signal config_r: std_logic_vector(15 downto 0);
begin
    cfg_read_count <= config_r(0);

    process(clk, rst_n)
    begin
        if rst_n = '0' then
            config_r <= (others => '0');
        elsif rising_edge(clk) then
            if fsmc_ce = '1' and fsmc_nwr = '0' then
                config_r <= fsmc_db;
            end if;
        end if;
    end process;
end architecture;