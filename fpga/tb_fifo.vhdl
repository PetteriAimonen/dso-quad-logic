library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_fifo is
    generic (
        fifo_width_g:   natural := 16;
        fifo_depth_g:   natural := 64
    );
end entity;

architecture testbench of tb_fifo is
    signal clk, rst_n:  std_logic;
    signal data_out:    std_logic_vector(fifo_width_g - 1 downto 0);
    signal read:        std_logic;
    signal data_in:     std_logic_vector(fifo_width_g - 1 downto 0);
    signal write:       std_logic;
    signal count:       std_logic_vector(15 downto 0);
begin
    fifo1:  entity work.FIFO
        generic map (width_g => fifo_width_g, depth_g => fifo_depth_g)
        port map (clk, rst_n, data_out, read, data_in, write, count);
    
    process
        procedure clock is
            constant PERIOD: time := 1 us;
        begin
            clk <= '0'; wait for PERIOD/2; clk <= '1'; wait for PERIOD/2;
        end procedure;
        
        variable output: natural;
    begin
        read <= '0';
        write <= '0';
        rst_n <= '0';
        clock;
        rst_n <= '1';
        
        assert unsigned(count) = 0 report "FIFO should be empty after reset";
        
        -- Write slowly
        for i in 1 to 10 loop
            data_in <= std_logic_vector(to_unsigned(i, fifo_width_g));
            write <= '1';
            clock;
            write <= '0';
            clock;
            assert unsigned(count) = i report "FIFO count is wrong in write";
        end loop;
        
        -- Write every clock cycle
        write <= '1';
        for i in 11 to fifo_depth_g - 1 loop
            data_in <= std_logic_vector(to_unsigned(i, fifo_width_g));
            clock;
        end loop;
        write <= '0';
        
        clock;
        assert unsigned(count) = fifo_depth_g - 1 report "FIFO should be full";
        
        -- Do a few dummy writes to test writing when full. These should
        -- be ignored by the FIFO.
        write <= '1';
        clock; clock; clock;
        write <= '0';
        assert unsigned(count) = fifo_depth_g - 1
            report "FIFO should ignore writes when full";
       
        for i in 1 to fifo_depth_g - 1 loop
            output := to_integer(unsigned(data_out));
            assert output = i
                report "FIFO output is wrong: " & natural'Image(output)
                & " should be " & natural'Image(i);
            
            read <= '1';
            clock;
            read <= '0';
            clock;
            
            assert unsigned(count) = fifo_depth_g - 1 - i
                report "FIFO count is wrong in read";
        end loop;
        read <= '0';
        
        clock;
        
        assert unsigned(count) = 0 report "FIFO should be empty";
        
        -- Do a few dummy reads to test reading when empty.
        read <= '1';
        clock; clock; clock;
        read <= '0';
        assert unsigned(count) = 0 report "FIFO should be empty after dummy reads";
        
        report "Simulation ended" severity note;
        wait;
    end process;
end architecture;