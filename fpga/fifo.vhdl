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
        depth_g: natural  -- Depth of the buffer, must be power of 2.
    );
    
    port (
        clk:        in std_logic;
        rst_n:      in std_logic;
        
        -- Data output port. Next data item is available on the
        -- output port when 'count' is not 0. When 'read' is
        -- high on clock edge, the item is removed from the FIFO
        -- and next value is available on the port one clock cycle
        -- later.
        data_out:   out std_logic_vector(width_g - 1 downto 0);
        read:       in std_logic;

        -- Data input port. When 'write' is high on clock edge,
        -- the value on data_in port is stored in the queue. If
        -- 'count' is depth_g - 1, the FIFO is full and write is
        -- ignored.
        data_in:    in std_logic_vector(width_g - 1 downto 0);
        write:      in std_logic;
        
        -- Current number of items in FIFO.
        -- Updates 1 clock cycle after write/read.
        count:      out std_logic_vector(15 downto 0)
    );
end entity;
    
architecture rtl of FIFO is
    -- Function to calculate base-2 logarithm of the generic parameters
    -- Result is rounded up to next integer value.
    function log2(x : positive) return natural is 
    begin
            if x <= 1 then
                    return 0;
            else
                    return log2(x / 2 + x mod 2) + 1;
            end if;
    end function log2;

    -- Number of bits for the read/write position counters
    constant counter_width_c : natural := log2(depth_g);

    -- buffer_r is the large RAM array that is used as a ring buffer
    subtype FIFOEntry is std_logic_vector(width_g - 1 downto 0);
    type FIFOArray is array(depth_g - 1 downto 0) of FIFOEntry;
    signal buffer_r:    FIFOArray;
    
    -- read_pos_r contains the index of the next item to read.
    -- write_pos_r contains the index of the next item to write.
    -- When read_pos_r = write_pos_r, FIFO is empty.
    -- When read_pos_r = write_pos_r + 1, FIFO is full.
    signal read_pos_r:  unsigned(counter_width_c - 1 downto 0);
    signal write_pos_r: unsigned(counter_width_c - 1 downto 0);
    
    -- Registers for output signals
    signal data_out_r:  std_logic_vector(width_g - 1 downto 0);
    signal count_r:     unsigned(counter_width_c - 1 downto 0);
    
    signal next_write_pos_r: unsigned(counter_width_c - 1 downto 0);
begin
    data_out <= data_out_r;
    count(counter_width_c - 1 downto 0) <= std_logic_vector(count_r);
    count(15 downto counter_width_c) <= (others => '0');
    
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            read_pos_r <= (others => '0');
            write_pos_r <= (others => '0');
            count_r <= (others => '0');
            data_out_r <= (others => '0');
            next_write_pos_r <= to_unsigned(1, counter_width_c);
        elsif rising_edge(clk) then
            count_r <= write_pos_r - read_pos_r;
            data_out_r <= buffer_r(to_integer(read_pos_r));
            
            -- Increment read position unless fifo is already empty
            if read = '1' and read_pos_r /= write_pos_r then
                read_pos_r <= read_pos_r + 1;
            end if;
            
            -- Always write the value (the last location is unused
            -- when fifo is full).
            if write = '1' then
                buffer_r(to_integer(write_pos_r)) <= data_in;
            end if;
            
            -- Increment write position unless fifo is already full
            if write = '1' and next_write_pos_r /= read_pos_r then
                write_pos_r <= next_write_pos_r;
                next_write_pos_r <= next_write_pos_r + 1;
            end if;
        end if;
    end process;
end architecture;