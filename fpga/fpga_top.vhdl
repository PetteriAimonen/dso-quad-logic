library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity fpga_top is
    generic (
        fifo_depth_g:   natural := 4096
    );
    
    port (
        clk:      in     std_logic;
        rst_n:    in     std_logic;
        
        -- Memory bus
        fsmc_ce:  in     std_logic;
        fsmc_nwr: in     std_logic;
        fsmc_nrd: in     std_logic;
        fsmc_db:  inout  std_logic_vector(15 downto 0);
        
        -- ADC signals
        adc_mode:  in    std_logic;
        adc_sleep: out   std_logic;
        cha_clk:   out   std_logic;
        chb_clk:   out   std_logic;
        
        -- Oscilloscope data inputs
        cha_din:   in    std_logic_vector(7 downto 0);
        chb_din:   in    std_logic_vector(7 downto 0);
        chc_din:   in    std_logic;
        chd_din:   in    std_logic
     );
end entity;

architecture rtl of fpga_top is
    -- Digital input channels
    signal ch_cd_in:            std_logic_vector(1 downto 0);
    signal ch_cd_delayed:       std_logic_vector(1 downto 0);
    signal ch_abcd:             std_logic_vector(3 downto 0);
    
    -- Configuration register signals
    signal cfg_read_count:       std_logic;
    
    -- RLE encoder outputs
    signal rle_data_out:        std_logic_vector(15 downto 0);
    signal rle_write:           std_logic;
    
    -- FIFO outputs
    signal fifo_data_out:       std_logic_vector(15 downto 0);
    signal fifo_count:          std_logic_vector(15 downto 0);
    signal fifo_read:           std_logic;
    
    -- Output data bus
    signal output_data:         std_logic_vector(15 downto 0);
begin
    -- ADC is clocked directly from 72 MHz output from the STM32
    cha_clk <= clk;
    chb_clk <= clk;
    
    -- Digital data is delayed to align it with ADC data
    ch_cd_in <= chc_din & chd_din;
    ch_abcd(3 downto 2) <= ch_cd_delayed;
    delay1: entity work.Delay
        generic map (width_g => 2, delay_g => 5)
        port map (clk, rst_n, ch_cd_in, ch_cd_delayed);
    
    -- Configuration register
    config1: entity work.Config
        port map (clk, rst_n, fsmc_ce, fsmc_nwr, fsmc_db, 
            cfg_read_count);
    
    -- Binarization of ADC data.
    -- In 200mV range, din >= 128 gives 1 V threshold voltage
    ch_abcd(3) <= cha_din(7);
    ch_abcd(2) <= chb_din(7);
    
    -- RLE encoding of input data
    rle1: entity work.RLECoder
        port map (clk, rst_n, ch_abcd, rle_data_out, rle_write);
    
    -- FIFO storage of data
    fifo1: entity work.FIFO
        generic map (width_g => 16, depth_g => fifo_depth_g)
        port map (clk, rst_n, fifo_data_out, fifo_read,
            rle_data_out, rle_write, fifo_count);
    
    -- Output either fifo data (if cfg_read_count = 0) or fifo count
    output_data <= fifo_data_out when (cfg_read_count = '0') else fifo_count;
    fifo_read <= (not fsmc_nrd) when (cfg_read_count = '0') else '0';
    
    -- FSMC bus control
    fsmc_db <= output_data
        when (fsmc_nrd = '0' and fsmc_ce = '1')
        else (others => 'Z');
end architecture;

