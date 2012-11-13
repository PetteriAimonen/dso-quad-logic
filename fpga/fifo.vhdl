-- This is a generic FIFO queue. It has a memory ring buffer, which can be
-- read and written simultaneously. The output data is acknowledged by 'read'
-- signal, which advances the read pointer.
-- The input data is simply written to a port and 'write' is asserted high.
-- Current level of FIFO is available on the 'count' port.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity FIFO is
    generic (
        width_g: natural; -- Width (in bits) of the data items.
        depth_g: natural  -- Depth of the buffer, as number of items.
    );
    
    port (
        clk:        in std_logic;
        rst_n:      in std_logic;
        
        -- Data output port. Next data item is available on the
        -- output port when 'count' is not 0. When 'read' is
        -- high on clock edge, the item is removed from the FIFO
        -- and next value is available on the port.
        data_out:   out std_logic_vector(width_g - 1 downto 0);
        read:       in std_logic;

        -- Data input port. When 'write' is high on clock edge,
        -- the value on data_in port is stored in the queue. If
        -- 'count' is depth_g, the FIFO is full and write is
        -- ignored.
        data_in:    in std_logic_vector(width_g - 1 downto 0);
        write:      in std_logic;
        
        -- Current number of items in FIFO.
        count:      out std_logic_vector(15 downto 0)
    );
end entity;
    
architecture rtl of FIFO is
    subtype FIFOEntry is std_logic_vector(width_g - 1 downto 0);
    type FIFOArray is array(depth_g - 1 downto 0) of FIFOEntry;
    
    -- buffer_r is the large RAM array that is used as a ring buffer
    signal buffer_r: FIFOArray;
    
    -- read_pos_r contains the index of the next item to read.
    -- write_pos_r contains the index of the next item to write.
    signal read_pos_r: natural range 0 to depth_g - 1;
    signal write_pos_r: natural range 0 to depth_g - 1;
    
    -- count_r contains the number of items in FIFO currently.
    signal count_r: natural range 0 to depth_g;
begin
    data_out <= buffer_r(read_pos_r);
    count <= std_logic_vector(to_unsigned(count_r, 16));
    
    process(clk, rst_n)
        -- Using variables to store the read / write status.
        variable read_v: boolean;
        variable write_v: boolean;
    begin
        if rst_n = '0' then
            read_pos_r <= 0;
            write_pos_r <= 0;
            count_r <= 0;
        elsif rising_edge(clk) then
            read_v := (read = '1') and (count_r /= 0);
            write_v := (write = '1') and (count_r /= depth_g);
        
            if read_v then
                if read_pos_r = depth_g - 1 then
                    read_pos_r <= 0;
                else
                    read_pos_r <= read_pos_r + 1;
                end if;
                
                if not write_v then
                    count_r <= count_r - 1;
                end if;
            end if;
            
            if write_v then
                buffer_r(write_pos_r) <= data_in;
                
                if write_pos_r = depth_g - 1 then
                    write_pos_r <= 0;
                else
                    write_pos_r <= write_pos_r + 1;
                end if;
                
                if not read_v then
                    count_r <= count_r + 1;
                end if;
            end if;
        end if;
    end process;
end architecture;