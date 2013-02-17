-- This block breaks wide subtraction operations into two clock cycles.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity PipelinedSubtract is
    generic (
        width_g: natural := 64; -- Must be divisible by parts_g.
        parts_g: natural := 4
    );
    
    port (
        clk:            in std_logic;
        rst_n:          in std_logic;
        
        value0_in:      in std_logic_vector(width_g - 1 downto 0);
        value1_in:      in std_logic_vector(width_g - 1 downto 0);
        
        result_out:     out std_logic_vector(width_g - 1 downto 0)
    );
end entity;

architecture rtl of PipelinedSubtract is
    constant part_width_c: natural := width_g / parts_g;
    
    subtype Intermediate is unsigned(part_width_c - 1 downto 0);
    type IntermediateArray is array(parts_g - 1 downto 0) of Intermediate;
    signal intermediate_r:      IntermediateArray;
    
    signal borrow_r:            std_logic_vector(parts_g - 1 downto 0);
    signal almost_borrow_r:     std_logic_vector(parts_g - 1 downto 0);
begin
    process (clk, rst_n)
        variable v0:            std_logic_vector(part_width_c - 1 downto 0);
        variable v1:            std_logic_vector(part_width_c - 1 downto 0);
        variable r:             unsigned(part_width_c downto 0);
        variable borrow:        std_logic;
    begin
        if rst_n = '0' then
            intermediate_r <= (others => (others => '0'));
        elsif rising_edge(clk) then
            for i in 0 to parts_g - 1 loop
                v0 := value0_in((i + 1) * part_width_c - 1 downto i * part_width_c);
                v1 := value1_in((i + 1) * part_width_c - 1 downto i * part_width_c);
                r := unsigned("1" & v0) - unsigned("0" & v1);
                
                intermediate_r(i) <= r(part_width_c - 1 downto 0);
                borrow_r(i) <= not r(part_width_c);
                
                if r(part_width_c - 1 downto 0) = to_unsigned(0, part_width_c) then
                    almost_borrow_r(i) <= '1';
                else
                    almost_borrow_r(i) <= '0';
                end if;
            end loop;
            
            borrow := '0';
            for i in 0 to parts_g - 1 loop
                if borrow = '1' then
                    result_out((i + 1) * part_width_c - 1 downto i * part_width_c)
                        <= std_logic_vector(intermediate_r(i) - 1);
                else
                    result_out((i + 1) * part_width_c - 1 downto i * part_width_c)
                        <= std_logic_vector(intermediate_r(i));
                end if;
                
                borrow := borrow_r(i) or (borrow and almost_borrow_r(i));
            end loop;
        end if;
    end process;
end architecture;
