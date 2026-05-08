local sys   = require "luci.sys"
local m, s, m2, s2, o

m = Map("nport", translate("Configuration"))
m:chain("luci")

s = m:section(TypedSection, "ttyCAN2", translate("Network Settings"))
s.anonymous = true
s.addremove = false

enable = s:option(Flag, "enable", translate("Enable"))
enable.rmempty  = false

netpro = s:option(ListValue, "netpro", translate("Network Proto"))
netpro.default = "tcpserver"
netpro:value("tcpserver", "TCP Server")
netpro:value("tcpclient", "TCP Client")
netpro:value("udpserver", "UDP Server")
netpro:value("udpclient", "UDP Client")
netpro:value("modbustcp", "Modbus TCP")

tranpro = s:option(ListValue, "tranpro", translate("Transport Proto"))
tranpro.default = "raw"
tranpro:value("raw", translate("Raw data"))
tranpro:value("telnet", translate("Telnet (RFC2217)"))
tranpro:depends("netpro", "udpclient")
tranpro:depends("netpro", "udpserver")
tranpro:depends("netpro", "tcpserver")

localport = s:option(Value, "localport", translate ("Local Port"))
localport.default = "4004"
localport.datatype="port"
localport:depends("netpro", "tcpserver")
localport:depends("netpro", "udpserver")
localport:depends("netpro", "udpclient")
localport:depends("netpro", "modbustcp")

maxconn = s:option(ListValue, "maxconn", translate ("Maximum number"))
maxconn.default = "6"
maxconn:value("1", translate("1"))
maxconn:value("2", translate("2"))
maxconn:value("3", translate("3"))
maxconn:value("4", translate("4"))
maxconn:value("5", translate("5"))
maxconn:value("6", translate("6"))
maxconn:depends("netpro", "udpserver")
maxconn:depends("netpro", "tcpserver")
maxconn:depends("netpro", "modbustcp")
maxconn:depends("netpro", "udpclient")

timeout = s:option(Value, "timeout", translate ("Time Out(s)"))
timeout.default = "300"
timeout:depends("netpro", "udpserver")
timeout:depends("netpro", "tcpserver")
timeout:depends("netpro", "udpclient")
timeout:depends("netpro", "modbustcp")

serverip = s:option(Value, "serverip", translate("Server IP Address"))
serverip.datatype = "ip4addr"
serverip.default = "192.168.0.100"
serverip:depends("netpro", "tcpclient")
serverip:depends("netpro", "udpclient")

serverport = s:option(Value, "serverport", translate ("Server Port"))
serverport.datatype="port"
serverport.default = "10000"
serverport:depends("netpro", "tcpclient")
serverport:depends("netpro", "udpclient")

hb = s:option(Flag, "hb", translate("Heart-Beat"))
hb.rmempty = false
hb:depends("netpro", "tcpclient")

hbp = s:option(Value, "hbp", translate("Heart-Beat Content"))
hbp.default = "FF012345678"
hbp:depends("hb", "1")

hex = s:option(Flag, "hex", translate("Hexadecimal"))
hex:depends("hb", "1")

hbpt = s:option(Value, "hbpt", translate("Heart-Beat Interval(s)"))
hbpt.datatype = "and(uinteger, min(1))"
hbpt.default = "30"
hbpt:depends("hb", "1")

s2 = m:section(TypedSection, "ttyCAN2", translate("CAN Settings"))
s2.anonymous = true
s2.addremove = false

	s2:tab("translate", translate("Coversion Configuration"))
	s2:tab("spi", translate("SPI Configuration"))
	s2:tab("uart", translate("UART Configuration"))
	s2:tab("can", translate("CAN Configuration"))


conversion_mode = s2:taboption("translate", ListValue, "conversion_mode", translate("Conversion Mode"))
conversion_mode:value("01", translate("Transparent Conversion"))
conversion_mode:value("02", translate("Transparent Conversion with Identification"))
conversion_mode:value("03", translate("Custom Conversion Protocol"))
conversion_mode.default = "01"

conversion_direction = s2:taboption("translate", ListValue, "conversion_direction", translate("Conversion Direction"))
conversion_direction:value("00", translate("Bidirectional Transmission"))
conversion_direction:value("01", translate("Only UART to CAN"))
conversion_direction:value("02", translate("Only CAN to UART"))
conversion_direction.default = "00"

allow_can_info_to_uart = s2:taboption("translate", Flag, "frame_info_to_uart", translate("Frame Info to UART"), translate("Allows CAN frame information to be forwarded to serial frames."))
allow_can_info_to_uart.rmempty  = false
allow_can_info_to_uart:depends("conversion_mode", "01")

allow_can_id_to_uart = s2:taboption("translate", Flag, "frame_id_to_uart", translate("Frame ID to UART"), translate("Allows CAN frame ids to be forwarded to serial frames."))
allow_can_id_to_uart.rmempty  = false
allow_can_id_to_uart:depends("conversion_mode", "01")

length_can_id = s2:taboption("translate", ListValue, "can_id_length", translate("CAN ID Length"), translate("The length of the CAN identifies in the serial frame."))
length_can_id:value("01", translate("1"))
length_can_id:value("02", translate("2"))
length_can_id:value("03", translate("3"))
length_can_id:value("04", translate("4"))
length_can_id:depends("conversion_mode", "02")

saddr_can_id = s2:taboption("translate", ListValue, "can_id_saddr", translate("CAN ID Address"), translate("The starting address of the CAN identifies in the serial frame."))
saddr_can_id:value("01", translate("1"))
saddr_can_id:value("02", translate("2"))
saddr_can_id:value("03", translate("3"))
saddr_can_id:value("04", translate("4"))
saddr_can_id:value("05", translate("5"))
saddr_can_id:value("06", translate("6"))
saddr_can_id:value("07", translate("7"))
saddr_can_id:depends("conversion_mode", "02")

frame_header = s2:taboption("translate", Value, "frame_header", translate("Frame Head"), translate("Only 2-bit HEX code is supported."))
frame_header:depends("conversion_mode", "03")

frame_tail = s2:taboption("translate", Value, "frame_tail", translate("Frame Tail"), translate("Only 2-bit HEX code is supported."))
frame_tail:depends("conversion_mode", "03")

uart_baud = s2:taboption("uart", ListValue, "uart_baud", translate("Baud Rate"),translate("Unit: bps"))
uart_baud:value("300", translate("300"))
uart_baud:value("600", translate("600"))
uart_baud:value("600", translate("1200"))
uart_baud:value("1200", translate("2400"))
uart_baud:value("4800", translate("4800"))
uart_baud:value("9600", translate("9600"))
uart_baud:value("14400", translate("14400"))
uart_baud:value("19200", translate("19200"))
uart_baud:value("38400", translate("38400"))
uart_baud:value("57600", translate("57600"))
uart_baud:value("115200", translate("115200"))
uart_baud:value("128000", translate("128000"))
uart_baud:value("230400", translate("230400"))
uart_baud:value("256000", translate("256000"))
uart_baud:value("460800", translate("460800"))
uart_baud:value("921600", translate("921600"))
uart_baud.default = "115200"

uart_frame_interval = s2:taboption("uart", ListValue, "frame_interval", translate("Frame Interval"), translate("Unit: bits time"))
uart_frame_interval:value("02", translate("2"))
uart_frame_interval:value("04", translate("4"))
uart_frame_interval:value("06", translate("6"))
uart_frame_interval:value("08", translate("8"))
uart_frame_interval:value("0A", translate("10"))
uart_frame_interval.default = "02"

can_baud = s2:taboption("can", ListValue, "can_baud", translate("Baud Rate"), translate("Unit: bps"))
can_baud:value("01", translate("5k"))
can_baud:value("02", translate("10k"))
can_baud:value("03", translate("20k"))
can_baud:value("04", translate("40k"))
can_baud:value("05", translate("50k"))
can_baud:value("06", translate("80k"))
can_baud:value("07", translate("100k"))
can_baud:value("08", translate("125k"))
can_baud:value("09", translate("200k"))
can_baud:value("0A", translate("250k"))
can_baud:value("0B", translate("400k"))
can_baud:value("0C", translate("500k"))
can_baud:value("0D", translate("666k"))
can_baud:value("0E", translate("800k"))
can_baud:value("0F", translate("1000k"))
can_baud.default = "08"

type_send_frame = s2:taboption("can", ListValue, "frame_send_type", translate("Frame Send Type"))
type_send_frame:value("08", translate("Extended Frame"))
type_send_frame:value("00", translate("Standard Frame"))
type_send_frame.default = "08"
type_send_frame:depends("conversion_mode", "01")
type_send_frame:depends("conversion_mode", "02")

id_send1 = s2:taboption("can", Value, "can_id_send1", translate("ID Send"), translate("Identification ID3~ID0, 000 ~7FF is valid."))
id_send1.default = "00 00 00 00"
id_send1:depends("frame_send_type","00")

id_send2 = s2:taboption("can", Value, "can_id_send2", translate("ID Send"), translate("Identification ID3~ID0, 00 00 00 00 ~1F FF FF FF is valid."))
id_send2.default = "00 00 00 00"
id_send2:depends("frame_send_type","08")

filter_enable = s2:taboption("can", Flag, "enable_filter", translate("Enable Filter"))
filter_enable.rmempty  = false

filter_mode = s2:taboption("can", ListValue, "filter_mode", translate("Filter Mode"))
filter_mode:value("08", translate("Only Extended Frame Filter"))
filter_mode:value("00", translate("Only Standard Frame Filter"))
filter_mode.default = "08"
filter_mode:depends("enable_filter","1")

filter_code = s2:taboption("can", Value, "filter_code", translate("Filter Code"), translate("00 00 00 00 ~FF FF FF FF is valid."))
filter_code.default = "FF FF FF FF"
filter_code:depends("enable_filter","1")

filter_acceptance_code0 = s2:taboption("can", Value, "acceptance_code0", translate("Acceptance Code0"), translate("00 00 00 00 ~FF FF FF FF is valid."))
filter_acceptance_code0.default = "00 00 00 00"
filter_acceptance_code0:depends("enable_filter","1")

filter_acceptance_code1 = s2:taboption("can", Value, "acceptance_code1", translate("Acceptance Code1"), translate("00 00 00 00 ~FF FF FF FF is valid."))
filter_acceptance_code1.default = "00 00 00 00"
filter_acceptance_code1:depends("enable_filter","1")

filter_acceptance_code2 = s2:taboption("can", Value, "acceptance_code2", translate("Acceptance Code2"), translate("00 00 00 00 ~FF FF FF FF is valid."))
filter_acceptance_code2.default = "00 00 00 00"
filter_acceptance_code2:depends("enable_filter","1")

filter_acceptance_code3 = s2:taboption("can", Value, "acceptance_code3", translate("Acceptance Code3"), translate("00 00 00 00 ~FF FF FF FF is valid."))
filter_acceptance_code3.default = "00 00 00 00"
filter_acceptance_code3:depends("enable_filter","1")

filter_acceptance_code4 = s2:taboption("can", Value, "acceptance_code4", translate("Acceptance Code4"), translate("00 00 00 00 ~FF FF FF FF is valid."))
filter_acceptance_code4.default = "00 00 00 00"
filter_acceptance_code4:depends("enable_filter","1")

filter_acceptance_code5 = s2:taboption("can", Value, "acceptance_code5", translate("Acceptance Code5"), translate("00 00 00 00 ~FF FF FF FF is valid."))
filter_acceptance_code5.default = "00 00 00 00"
filter_acceptance_code5:depends("enable_filter","1")

function Returnpopen(liunxmsg)
	local brightness = io.popen(liunxmsg) --io.popen()每一次使用，都必须使用close()关闭 否则长时间使用程序崩溃。
	local get_brightness = brightness:read("*all")
	brightness:close()
	get_brightness = string.gsub(get_brightness,"%s+","")
	if tonumber(get_brightness) then --判断字符串是否可以转成数字，true 转成数字，否则字符串。
		get_brightness = tonumber(get_brightness)
	end
	return get_brightness
end

local apply = luci.http.formvalue("cbi.apply")
if apply then
    io.popen("sleep 1s;/etc/setcan 2 &")
end

return m,m2
