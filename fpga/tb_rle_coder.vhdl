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
        variable time_v: integer;
    
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
        clock;
        
        assert write = '1' and data_out = X"002A" report "Wrong output pair";
        
        clock;
        
        assert write = '0' report "Unexpected write";

        for i in 2 to 255 loop
            clock;
        end loop;
        
        data_in <= "1111";
        
        clock;
        clock;
        
        assert write = '1' and data_out = X"100F" report "Wrong output pair";
        
        -- Test long segment write
        for i in 1 to 4095 loop
            clock;
        end loop;
        
        data_in <= "0000";
        clock;
        
        assert write = '1' and data_out = X"8002" report "Wrong time segment";
        
        data_in <= "1111";
        clock;
        
        assert write = '1' and data_out = X"0000" report "Wrong output pair";
        clock;
        
        assert write = '1' and data_out = X"000F" report "Wrong output pair";
        
        -- Test periodic time segment write
        -- Too slow test :)
--         time_v := 0;
--         for i in 2 to 100000000 loop
--             clock;
--             if write = '1' then
--                 assert data_out(15) = '1' report "Expected time segment";
--                 time_v := time_v + to_integer(unsigned(data_out(14 downto 0))) * 2048;
--             end if;
--         end loop;
--         
--         data_in <= "0000";
--         clock;
--         assert data_out(15) = '1' report "Expected time segment";
--         time_v := time_v + to_integer(unsigned(data_out(14 downto 0))) * 2048;
--         clock;
--         assert data_out(15) = '0' report "Expected data segment";
--         assert data_out(3 downto 0) = "1111" report "Wrong data";
--         time_v := time_v + to_integer(unsigned(data_out(14 downto 4)));
--         assert time_v = 100000000 report "Wrong time";
        
        report "Simulation ended" severity note;
        wait;
    end process;
end architecture;