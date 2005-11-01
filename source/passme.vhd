-- standard libraries
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity PassMe is
	port
	(
		DSSLOT_CLK		: in std_logic;
		DSSLOT_ROMCS	: in std_logic;
		DSSLOT_RESET	: in std_logic;
		DSSLOT_EEPCS	: in std_logic;
		DSSLOT_IRQ		: out std_logic;
		DSSLOT_IO		: inout std_logic_vector(7 downto 0);
		
		DSCART_CLK		: out std_logic;
		DSCART_ROMCS	: out std_logic;
		DSCART_RESET	: out std_logic;
		DSCART_EEPCS	: out std_logic;
		DSCART_IRQ 		: in std_logic;
		DSCART_IO 		: inout std_logic_vector(7 downto 0);
		
		LED0 			: out std_logic
	);
end entity;

architecture rtl of passme is

	-- removes Xilinx mapping errors
	attribute CLOCK_BUFFER : string;
	attribute CLOCK_BUFFER of DSSLOT_CLK: signal is "ibuf";
	attribute CLOCK_BUFFER of DSCART_CLK: signal is "obuf";

	signal is_command		: boolean;
	signal cmddata_cnt 		: natural range 0 to 511;		-- 8 + 504
	signal patched_data		: std_logic_vector(7 downto 0);
	signal patch_en			: boolean;

begin

	-- direct passthrough
	DSCART_CLK <= DSSLOT_CLK;
	DSCART_ROMCS <= DSSLOT_ROMCS;
	DSCART_RESET <= DSSLOT_RESET;
	DSSLOT_IRQ <= DSCART_IRQ;
	DSCART_EEPCS <= DSSLOT_EEPCS;

	-- activity LED
	LED0 <= not DSSLOT_ROMCS;

	-- patch
	process (cmddata_cnt)
	begin
		case (cmddata_cnt - 8) is
--! ALL PATCHES ARE TO BE GENERATED HERE
			when others => 	patched_data <= DSCART_IO;
		end case;
	end process;

	-- dataswitcher
	process (DSSLOT_RESET, DSSLOT_ROMCS, DSSLOT_EEPCS, DSSLOT_IO, DSCART_IO, patched_data)
	begin
		DSSLOT_IO <= (others => 'Z');				-- default is high impedance
		DSCART_IO <= (others => 'Z');				-- default is high impedance

		if (DSSLOT_RESET='1') then					-- if not reset
			if (DSSLOT_ROMCS='0') then				-- ROM is selected
				if (is_command) then				-- is command byte
					DSCART_IO <= DSSLOT_IO;			-- from DS to cartridge
				else								-- is data byte
					if (patch_en) then				-- patch enabled
						DSSLOT_IO <= patched_data;
					else
						DSSLOT_IO <= DSCART_IO;
					end if;
				end if;
			elsif (DSSLOT_EEPCS='0') then			-- EEPROM is selected
				DSCART_IO(7) <= DSSLOT_IO(7);		-- pass serial data
				DSSLOT_IO(6) <= DSCART_IO(6);		-- pass serial data in opposite direction
			end if;
		end if;
	end process;

	-- patch_en
	process (DSSLOT_RESET, DSSLOT_CLK)
	begin
		if (DSSLOT_RESET='0') then
			patch_en <= true;						-- patch header
		elsif (rising_edge(DSSLOT_CLK)) then
			if (is_command) then
				if (DSCART_IO(5) = '1') then		-- detect 3C command, assume other command bytes are 00
					patch_en <= false;				-- do not patch other data
				end if;
			end if;
		end if;
	end process;

	-- cmddata_cnt, is_command
	process (DSSLOT_ROMCS, DSSLOT_CLK)
	begin
		if (DSSLOT_ROMCS='1') then
			cmddata_cnt <= 0;						-- new transfer
			is_command <= true;						-- start with command
		elsif (rising_edge(DSSLOT_CLK)) then
			if (cmddata_cnt mod 8 = 7) then
				is_command <= false;				-- next byte is data
			end if;
			cmddata_cnt <= cmddata_cnt + 1;			-- next byte
		end if;
	end process;

end architecture;
