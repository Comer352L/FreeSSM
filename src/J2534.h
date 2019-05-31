/*
 * J2534.h - SAE-J2534 definitions
 *
 * Copyright (C) 2009-2019 Comer352L
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef J2534_H
#define J2534_H


// Protocols:
#define		J1850VPW				0x01
#define		J1850PWM				0x02
#define		ISO9141					0x03
#define		ISO14230				0x04
#define		CAN					0x05
#define		ISO15765				0x06
#define		SCI_A_ENGINE				0x07
#define		SCI_A_TRANS				0x08
#define		SCI_B_ENGINE				0x09
#define		SCI_B_TRANS				0x0A


// Ioctls
#define		GET_CONFIG				0x01	// SCONFIG_LIST		NULL
#define		SET_CONFIG				0x02	// SCONFIG_LIST		NULL
#define		READ_VBATT				0x03	// NULL			unsigned long
#define		FIVE_BAUD_INIT				0x04	// SBYTE_ARRAY		SBYTE_ARRAY
#define		FAST_INIT				0x05	// PASSTHRU_MSG		PASSTHRU_MSG
#define		CLEAR_TX_BUFFER				0x07	// NULL			NULL
#define		CLEAR_RX_BUFFER				0x08	// NULL			NULL
#define		CLEAR_PERIODIC_MSGS			0x09	// NULL			NULL
#define		CLEAR_MSG_FILTERS			0x0A	// NULL			NULL
#define		CLEAR_FUNCT_MSG_LOOKUP_TABLE		0x0B	// NULL			NULL
#define		ADD_TO_FUNCT_MSG_LOOKUP_TABLE		0x0C	// SBYTE_ARRAY		NULL
#define		DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE	0x0D	// SBYTE_ARRAY		NULL
#define		READ_PROG_VOLTAGE			0x0E	// NULL			unsigned long


// Ioctl parameters for GET_CONFIG and SET_CONFIG
#define		DATA_RATE		0x01	// 5 – 500000 	// Baud rate value used for vehicle network. No default value specified.
#define		LOOPBACK		0x03	// 0(OFF)/1(ON)	// 0 = Do not echo transmitted messages to the Receive queue. 1 = Echo transmitted messages to the Receive queue.
								// Default value is 0(OFF).
#define		NODE_ADDRESS		0x04	// 0x00-0xFF	// J1850PWM specific, physical address for node of interest in the vehicle network. Default is no nodes are recognized by scan tool.
#define		NETWORK_LINE		0x05	// 0(BUS_NORMAL)/1(BUS_PLUS)/2(BUS_MINUS)	// J1850PWM specific, network line(s) active during message transfers. Default value is 0(BUS_NORMAL).
#define		P1_MIN			0x06	// 0x0-0xFFFF	// ISO-9141/14230 specific, min. ECU inter-byte time for responses [02.02-API: ms]. Default value is 0 ms. 04.04-API: NOT ADJUSTABLE, 0ms.
#define		P1_MAX			0x07	// 0x0/0x1-0xFFFF // ISO-9141/14230 specific, max. ECU inter-byte time for responses [02.02-API: ms, 04.04-API: *0.5ms]. Default value is 20 ms.
#define		P2_MIN			0x08	// 0x0-0xFFFF	// ISO-9141/14230 specific, min. ECU response time to a tester request or between ECU responses [02.02-API: ms, 04.04-API: *0.5ms]. 04.04-API: NOT ADJUSTABLE, 0ms. Default value is 25 ms.
#define		P2_MAX			0x09	// 0x0-0xFFFF	// ISO-9141/14230 specific, max. ECU response time to a tester request or between ECU responses [02.02-API: ms, 04.04-API: *0.5ms]. 04.04-API: NOT ADJUSTABLE, all messages up to P3_MIN are receoved. Default value is 50 ms.
#define		P3_MIN			0x0A	// 0x0-0xFFFF	// ISO-9141/14230 specific, min. ECU response time between end of ECU response and next tester request [02.02-API: ms, 04.04-API: *0.5ms]. Default value is 55 ms.
#define		P3_MAX			0x0B	// 0x0-0xFFFF	// ISO-9141/14230 specific, max. ECU response time between end of ECU response and next tester request [02.02-API: ms, 04.04-API: *0.5ms]. 04.04-API: NOT ADJUSTABLE, messages can be sent at anytime after P3_MIN. Default value is 5000 ms.
#define		P4_MIN			0x0C	// 0x0-0xFFFF	// ISO-9141/14230 specific, min. tester inter-byte time for a request [02.02-API: ms, 04.04-API: *0.5ms]. Default value is 5 ms.
#define		P4_MAX			0x0D	// 0x0-0xFFFF	// ISO-9141/14230 specific, max. tester inter-byte time for a request [02.02-API: ms, 04.04-API: *0.5ms]. 04.04-API: NOT ADJUSTABLE, P4_MIN is always used. Default value is 20 ms.
#define		W1			0x0E	// 0x0-0xFFFF	// ISO 9141 specific, max. time [ms] from the address byte end to synchronization pattern start. Default value is 300 ms.
#define		W2			0x0F	// 0x0-0xFFFF	// ISO 9141 specific, max. time [ms] from the synchronization byte end to key byte 1 start. Default value is 20 ms.
#define		W3			0x10	// 0x0-0xFFFF	// ISO 9141 specific, max. time [ms] between key byte 1 and key byte 2. Default value is 20 ms.
#define		W4			0x11	// 0x0-0xFFFF	// ISO 9141 specific, 02.02-API: max. time [ms] between key byte 2 and its inversion from the tester. Default value is 50 ms.
								// ISO 9141 specific, 04.04-API: min. time [ms] between key byte 2 and its inversion from the tester. Default value is 50 ms.
#define		W5			0x12	// 0x0-0xFFFF	// ISO 9141 specific, min. time [ms] before the tester begins retransmission of the address byte. Default value is 300 ms.
#define		TIDLE			0x13	// 0x0-0xFFFF	// ISO 9141 specific, bus idle time required before starting a fast initialization sequence. Default value is W5 value.
#define		TINL			0x14	// 0x0-0xFFFF	// ISO 9141 specific, the duration [ms] of the fast initialization low pulse. Default value is 25 ms.
#define		TWUP			0x15	// 0x0-0xFFFF	// ISO 9141 specific, the duration [ms] of the fast initialization wake-up pulse. Default value is 50 ms.
#define		PARITY			0x16	// 0(NO_PARITY)/1(ODD_PARITY)/2(EVEN_PARITY)	// ISO9141 specific, parity type for detecting bit errors.  Default value is 0(NO_PARITY).
#define		BIT_SAMPLE_POINT	0x17	// 0-100	// CAN specific, the desired bit sample point as a percentage of bit time. Default value is 80%.
#define		SYNCH_JUMP_WIDTH	0x18	// 0-100	// CAN specific, the desired synchronization jump width as a percentage of the bit time. Default value is 15%.
#define		W0			0x19
#define		T1_MAX			0x1A	// 0x0-0xFFFF	// SCI_X_XXXX specific, the max. interframe response delay. Default value is 20 ms.
#define		T2_MAX			0x1B	// 0x0-0xFFFF	// SCI_X_XXXX specific, the max. interframe request delay.Default value is 100 ms.
#define		T4_MAX			0x1C	// 0x0-0xFFFF	// SCI_X_XXXX specific, the max. intermessage response delay. Default value is 20 ms.
#define		T5_MAX			0x1D	// 0x0-0xFFFF	// SCI_X_XXXX specific, the max. intermessage request delay. Default value is 100 ms.
#define		ISO15765_BS		0x1E	// 0x0-0xFF	// ISO15765 specific, the block size for segmented transfers.
								// The scan tool may override this value to match the capabilities reported by the ECUs. Default value is 0. */
#define		ISO15765_STMIN		0x1F	// 0x0-0xFF	// ISO15765 specific, the separation time for segmented transfers.
								// The scan tool may override this value to match the capabilities reported by the ECUs. Default value is 0.
#define		DATA_BITS		0x20	// 04.04-API only
#define		FIVE_BAUD_MOD		0x21
#define		BS_TX			0x22
#define		STMIN_TX		0x23
#define		T3_MAX			0x24
#define		ISO15765_WFT_MAX	0x25


// Error definitions
#define		STATUS_NOERROR			0x00	// Function completed successfully.
#define		ERR_SUCCESS			0x00	// Function completed successfully.
#define		ERR_NOT_SUPPORTED		0x01	// Function option is not supported.
#define		ERR_INVALID_CHANNEL_ID		0x02	// Channel Identifier or handle is not recognized.
#define		ERR_INVALID_PROTOCOL_ID		0x03	// Protocol Identifier is not recognized.
#define		ERR_NULL_PARAMETER		0x04	// NULL pointer presented as a function parameter, NULL is an invalid address.
#define		ERR_INVALID_IOCTL_VALUE		0x05	// Ioctl GET_CONFIG/SET_CONFIG parameter value is not recognized.
#define		ERR_INVALID_FLAGS		0x06	// Flags bit field(s) contain(s) an invalid value.
#define		ERR_FAILED			0x07	// Unspecified error, use PassThruGetLastError for obtaining error text string.
#define		ERR_DEVICE_NOT_CONNECTED	0x08	// PassThru device is not connected to the PC.
#define		ERR_TIMEOUT			0x09	/* Timeout violation. PassThru device is unable to read specified number of messages from the vehicle network.
							   The actual number of messages returned is in NumMsgs. */
#define		ERR_INVALID_MSG			0x0A	// Message contained a min/max length, ExtraData support or J1850PWM specific source address conflict violation.
#define		ERR_INVALID_TIME_INTERVAL	0x0B	// The time interval value is outside the specified range.
#define		ERR_EXCEEDED_LIMIT		0x0C	// The limit(ten) of filter/periodic messages has been exceeded for the protocol associated the communications channel.
#define		ERR_INVALID_MSG_ID		0x0D	// The message identifier or handle is not recognized.
#define		ERR_DEVICE_IN_USE		0x0E	// The specified PassThru device is already in use.
#define		ERR_INVALID_IOCTL_ID		0x0F	// Ioctl identifier is not recognized.
#define		ERR_BUFFER_EMPTY		0x10	// PassThru device could not read any messages from the vehicle network.
#define		ERR_BUFFER_FULL			0x11	// PassThru device could not queue any more transmit messages destined for the vehicle network.
#define		ERR_BUFFER_OVERFLOW		0x12	// PassThru device experienced a buffer overflow and receive messages were lost.
#define		ERR_PIN_INVALID			0x13	// Unknown pin number specified for the J1962 connector.
#define		ERR_CHANNEL_IN_USE		0x14	// An existing communications channel is currently using the specified network protocol.
#define		ERR_MSG_PROTOCOL_ID		0x15	/* The specified protocol type within the message structure is different from the protocol associated with
							   the communications channel when it was opened. */
#define		ERR_INVALID_FILTER_ID		0x16	// Filter identifier is not recognized.
#define		ERR_NO_FLOW_CONTROL		0x17	// No ISO15765 flow control filter is set, or no filter matches the header of an outgoing message.
#define		ERR_NOT_UNIQUE			0x18	// An existing filter already matches this header or node identifier.
#define		ERR_INVALID_BAUDRATE		0x19	// Unable to honor requested Baud rate within required tolerances.
#define		ERR_INVALID_DEVICE_ID		0x1A	// PassThru device identifier is not recognized.


// Message filter types for fcns PassThruStartMsgFilter(), PassThruStopMsgFilter():
#define		PASS_FILTER			0x01	// PassThru device adds receive messages matching the Mask and Pattern criteria to its receive message queue.
#define		BLOCK_FILTER			0x02	// PassThru device ignores receive messages matching the Mask and Pattern criteria.
#define		FLOW_CONTROL_FILTER		0x03	// PassThru device adds receive messages matching the Mask and Pattern criteria to its receive message queue.
							// The PassThru device transmits a flow control message (only for ISO 15765-4) when receiving multi-segmented frames.


// Programming Voltages (Pins 0, 6, 9, 11-15):
// => value in mV (valid range: 5000 mV = 0x1388 to 20000 mV = 0x4e20) => only pins 0, 6, 9, 11-14
#define		SHORT_TO_GROUND			0xFFFFFFFE	// only pin 15
#define		VOLTAGE_OFF			0xFFFFFFFF	// all pins (0, 6, 9, 11-15)


// Loopback setting (ioctl GET_CONFIG/SET_CONFIG: parameter LOOPBACK):
#define		OFF				0x00
#define		ON				0x01


// Data bits setting (ioctl GET_CONFIG/SET_CONFIG: parameter DATA_BITS):
#define		DATA_BITS_8			0x00
#define		DATA_BITS_7			0x01


// Parity setting (ioctl GET_CONFIG/SET_CONFIG: parameter PARITY):
#define		NO_PARITY			0x00
#define		ODD_PARITY			0x01
#define		EVEN_PARITY			0x02


// J1850-PWM (ioctl GET_CONFIG/SET_CONFIG: parameter NETWORK_LINE):
#define		BUS_NORMAL			0x00
#define		BUS_PLUS			0x01
#define		BUS_MINUS			0x02


// Connect flags:
#define		CAN_29BIT_ID		0x00000100
#define		ISO9141_NO_CHECKSUM	0x00000200
#define		CAN_ID_BOTH		0x00000800
#define		ISO9141_K_LINE_ONLY	0x00001000


// Rx status flags:
#define		CAN_29BIT_ID		0x00000100	// CAN ID Type: 0 = 11-bit, 1 = 29-bit
#define		ISO15765_ADDR_TYPE	0x00000080
#define		ISO15765_PADDING_ERROR	0x00000010
#define		TX_DONE			0x00000008
#define		RX_BREAK		0x00000004	// Receive Break: 0 = No Break indication, 1 = Break indication present
#define		ISO15765_FIRST_FRAME	0x00000002	// ISO15765-2 only: 0 = No First Frame indication, 1 = First Frame indication
#define		START_OF_MESSAGE	0x00000002	// ISO15765-2 only: 0 = No First Frame indication, 1 = First Frame indication
#define		TX_MSG_TYPE		0x00000001	// Receive Indication/Transmit Confirmation: 0 = Rx Frame indication, 1 = Tx Frame confirmation


// Tx flags:
#define		SCI_TX_VOLTAGE		0x00800000	// SCI programming: 0 = do not apply voltage after transmitting message, 1 = apply voltage(20V) after transmitting message
#define		SCI_MODE		0x00400000
#define		BLOCKING		0x00010000	/* 02.02-API: Tx blocking mode: 0 = non-blocking transmit request, 1 = blocking transmit request
							   04.04-API: this value is reserved for J2534-2 !
							   NOTE: not really needed, instead a timeout value > 0 can be used with PassThruWriteMsgs() */
#define		WAIT_P3_MIN_ONLY	0x00000200
#define		CAN_29BIT_ID		0x00000100	// CAN ID Type: 0 = 11-bit, 1 = 29-bit
#define		CAN_EXTENDED_ID		0x00000100
#define		ISO15765_ADDR_TYPE	0x00000080	// ISO15765-2 Addressing mode: 0 = No extended address, 1 = Extended address is first byte following the CAN ID
#define		ISO15765_EXT_ADDR	0x00000080
#define		ISO15765_FRAME_PAD	0x00000040	// ISO15765-2 Frame Pad mode: 0 = No frame padding, 1 = Zero pad FlowControl, Single and Last ConsecutiveFrame to full CAN frame size.
#define		TX_NORMAL_TRANSMIT	0x00000000




typedef struct {
	unsigned long ProtocolID;	/* vehicle network protocol */
	unsigned long RxStatus;		/* receive message status */
	unsigned long TxFlags;		/* transmit message flags */
	unsigned long Timestamp;	/* receive message timestamp(in microseconds) */
	unsigned long DataSize;		/* byte size of message payload in the Data array */
	unsigned long ExtraDataIndex;	/* start of extra data(i.e. CRC, checksum, etc) in Data array */
	unsigned char Data[4128];	/* message payload or data */
} PASSTHRU_MSG;


typedef struct {
	unsigned long NumOfBytes;	/* Number of functional addresses in array */
	unsigned char *BytePtr;		/* pointer to functional address array */
} SBYTE_ARRAY;


typedef struct {
	unsigned long Parameter;	/* Name of configuration parameter */
	unsigned long Value;		/* Value of configuration parameter */
} SCONFIG;


typedef struct {
	unsigned long NumOfParams;	/* size of SCONFIG array */
	SCONFIG *ConfigPtr;		/* array containing configuration item(s) */
} SCONFIG_LIST;



// Function prototypes:

#if defined __WIN32__
	#define APICALL __stdcall
#else
	#define APICALL
#endif

typedef long APICALL (*J2534_PassThruOpen)(void* pName, unsigned long *pDeviceID);	// 0404-API only
typedef long APICALL (*J2534_PassThruClose)(unsigned long DeviceID);			// 0404-API only
typedef long APICALL (*J2534_PassThruConnect_0202)(unsigned long ProtocolID, unsigned long Flags, unsigned long *pChannelID);							// 0202-API
typedef long APICALL (*J2534_PassThruConnect_0404)(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long BaudRate, unsigned long *pChannelID);	// 0404-API
typedef long APICALL (*J2534_PassThruDisconnect)(unsigned long ChannelID);
typedef long APICALL (*J2534_PassThruReadVersion_0202)(char *pFirmwareVersion, char *pDllVersion, char *pApiVersion);								// 0202-API
typedef long APICALL (*J2534_PassThruReadVersion_0404)(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion);					// 0404-API
typedef long APICALL (*J2534_PassThruGetLastError)(char *pErrorDescription);
typedef long APICALL (*J2534_PassThruReadMsgs)(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
typedef long APICALL (*J2534_PassThruStartMsgFilter)(unsigned long ChannelID, unsigned long FilterType, PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg, PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID);
typedef long APICALL (*J2534_PassThruStopMsgFilter)(unsigned long ChannelID, unsigned long MsgID);
typedef long APICALL (*J2534_PassThruWriteMsgs)(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
typedef long APICALL (*J2534_PassThruStartPeriodicMsg)(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval);
typedef long APICALL (*J2534_PassThruStopPeriodicMsg)(unsigned long ChannelID, unsigned long MsgID);
typedef long APICALL (*J2534_PassThruIoctl)(unsigned long HandleID, unsigned long IoctlID, void *pInput, void *pOutput);
typedef long APICALL (*J2534_PassThruSetProgrammingVoltage_0202)(unsigned long PinNumber, unsigned long Voltage);								// 0202-API
typedef long APICALL (*J2534_PassThruSetProgrammingVoltage_0404)(unsigned long DeviceID, unsigned long PinNumber, unsigned long Voltage);					// 0404-API



#endif
