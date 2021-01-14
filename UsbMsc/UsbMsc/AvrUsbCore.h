/*
 * AvrUsbCore.h
 *
 * Created: 18.11.2020 13:04:03
 *  Author: Harald
 */ 


#ifndef AVRUSBCORE_H_
#define AVRUSBCORE_H_



#include <avr/io.h>
#define F_CPU 16000000ul
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include "SSpiDebug.h"

#include "AvrCommon.h"

#include "SDCard.h"

/*****************************************************************************************************************************/
/*                    DEFINITIONS                                                                                            */
/*****************************************************************************************************************************/


typedef struct
{
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint8_t wValueL;
	uint8_t wValueH;
	uint16_t wIndex;
	uint16_t wLength;
} USBSetup;

#define USB_EP_0 0
#define USB_EP_1 1
#define USB_EP_2 2
#define USB_EP_3 3
#define USB_EP_4 4
#define USB_EP_5 5
#define USB_EP_6 6

// only for memory
#define TRANSFER_PGM 1

#define TRUE 1
#define FALSE 0

// for UECFG0X
// xx.. ....
// 00b - Control
// 01b - Isochronous
// 10b - Bulk
// 11b - Interrupt
// .... ...x
//         0 - OUT direction
//         1 - IN  direction
#define USB_EP_TYPE_CONTROL				( 0x00 )
#define USB_EP_TYPE_BULK_IN				( (1<<EPTYPE1)				   | (1<<EPDIR) )
#define USB_EP_TYPE_BULK_OUT			( (1<<EPTYPE1)                              )
#define USB_EP_TYPE_INTERRUPT_IN		( (1<<EPTYPE1)  | (1<<EPTYPE0) | (1<<EPDIR) )
#define USB_EP_TYPE_INTERRUPT_OUT		( (1<<EPTYPE1)  | (1<<EPTYPE0)              )
#define USB_EP_TYPE_ISOCHRONOUS_IN		( (1<<EPTYPE0)				   | (1<<EPDIR) )
#define USB_EP_TYPE_ISOCHRONOUS_OUT		( (1<<EPTYPE0)                              )


// for UECFG1X
//	.xxx ....
//	 000b - 8   bytes
//   001b - 16  bytes
//   010b - 32  bytes
//   011b - 64  bytes
//
//   110b - 512 bytes
//   111b - Do not use!!!
//	.... xx..
//       00b - one bank
//       01b - double bank
//  .... ..x.
//         1 - for allocate
#define USB_EP_MEM_64_SINGLE 0b00110010 // for EP0
#define USB_EP_MEM_64_DOUBLE 0b00110110

// additions:
#define USB_EP_MEM_8_SINGLE   0b00000010
#define USB_EP_MEM_16_SINGLE  0b00010010
#define USB_EP_MEM_32_SINGLE  0b00100010
#define USB_EP_MEM_128_SINGLE 0b01000010

#define USB_EP_MEM_8_DOUBLE   0b00000110
#define USB_EP_MEM_16_DOUBLE  0b00010110
#define USB_EP_MEM_32_DOUBLE  0b00100110
#define USB_EP_MEM_128_DOUBLE 0b01000110




//	Standard requests
#define GET_STATUS			0
#define CLEAR_FEATURE		1
#define SET_FEATURE			3
#define SET_ADDRESS			5
#define GET_DESCRIPTOR		6
#define SET_DESCRIPTOR		7
#define GET_CONFIGURATION	8
#define SET_CONFIGURATION	9
#define GET_INTERFACE		10
#define SET_INTERFACE		11


//Setup
//bmRequestType
// x... ....
// 0 - host to dev
// 1 - dev to host
// .xx. ....
//  00 - STD
//  01 - CLASS
//  10 - VENDOR
//  11 - RESERVE
// ...x xxxx
//    0 0000 - ?????????? ??????????
//    0 0001 - interface
//    0 0010 - end point
//    0 0011 - other
//    reserved
//
// bmRequestType
#define REQUEST_HOSTTODEVICE	0x00
#define REQUEST_DEVICETOHOST	0x80
#define REQUEST_DIRECTION( RequestType )		( 0x80 & (RequestType) )

#define REQUEST_STANDARD		0x00
#define REQUEST_CLASS			0x20
#define REQUEST_VENDOR			0x40
#define REQUEST_TYPE( RequestType )				( 0x60 & (RequestType) )

#define REQUEST_DEVICE			0x00
#define REQUEST_INTERFACE		0x01
#define REQUEST_ENDPOINT		0x02
#define REQUEST_OTHER			0x03
#define REQUEST_RECIPIENT( RequestType )		( 0x1F & (RequestType) )


#define REQUEST_DEVICETOHOST_CLASS_INTERFACE    (REQUEST_DEVICETOHOST | REQUEST_CLASS    | REQUEST_INTERFACE)
#define REQUEST_HOSTTODEVICE_CLASS_INTERFACE    (REQUEST_HOSTTODEVICE | REQUEST_CLASS    | REQUEST_INTERFACE)
#define REQUEST_DEVICETOHOST_STANDARD_INTERFACE (REQUEST_DEVICETOHOST | REQUEST_STANDARD | REQUEST_INTERFACE)
#define REQUEST_HOSTTODEVICE_STANDARD_DEVICE	(REQUEST_HOSTTODEVICE | REQUEST_STANDARD    | REQUEST_DEVICE)



//	Descriptors
#define USB_DEVICE_DESC_SIZE 18
#define USB_CONFIGUARTION_DESC_SIZE 9
#define USB_INTERFACE_DESC_SIZE 9
#define USB_ENDPOINT_DESC_SIZE 7

#define USB_DEVICE_DESCRIPTOR_TYPE             1
#define USB_CONFIGURATION_DESCRIPTOR_TYPE      2
#define USB_STRING_DESCRIPTOR_TYPE             3
#define USB_INTERFACE_DESCRIPTOR_TYPE          4
#define USB_ENDPOINT_DESCRIPTOR_TYPE           5


// usb_20.pdf Table 9.6 Standard Feature Selectors
#define DEVICE_REMOTE_WAKEUP                   1
#define ENDPOINT_HALT                          2
#define TEST_MODE                              3

// usb_20.pdf Figure 9-4. Information Returned by a GetStatus() Request to a Device
#define FEATURE_SELFPOWERED_ENABLED     (1 << 0)
#define FEATURE_REMOTE_WAKEUP_ENABLED   (1 << 1)

#define USB_DEVICE_CLASS_COMMUNICATIONS        0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE       0x03
#define USB_DEVICE_CLASS_STORAGE               0x08
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC       0xFF

#define USB_CONFIG_POWERED_MASK                0x40
#define USB_CONFIG_BUS_POWERED                 0x80
#define USB_CONFIG_SELF_POWERED                0xC0
#define USB_CONFIG_REMOTE_WAKEUP               0x20

// bMaxPower in Configuration Descriptor
#define USB_CONFIG_POWER_MA(mA)                ((mA)/2)
#ifndef USB_CONFIG_POWER
#define USB_CONFIG_POWER                      (500)
#endif

// bEndpointAddress in Endpoint Descriptor
#define USB_ENDPOINT_OUT                       0x00
#define USB_ENDPOINT_IN						   0x80


#define USB_ENDPOINT_TYPE_MASK                 0x03
#define USB_ENDPOINT_TYPE_CONTROL              0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS          0x01
#define USB_ENDPOINT_TYPE_BULK                 0x02
#define USB_ENDPOINT_TYPE_INTERRUPT            0x03

#define TOBYTES(x) ((x) & 0xFF),(((x) >> 8) & 0xFF)


/*****************************************************************************************************************************/
/*                   STD  DESCRIPTORS                                                                                        */
/*****************************************************************************************************************************/

//	Device
typedef struct _DEVICE_DESCRIPTOR {
	char len;				// 18
	char dtype;			// 1 USB_DEVICE_DESCRIPTOR_TYPE
	short usbVersion;		// 0x200 or 0x210
	char	deviceClass;
	char	deviceSubClass;
	char	deviceProtocol;
	char	packetSize0;	// Packet 0
	short	idVendor;
	short	idProduct;
	short	deviceVersion;	// 0x100
	char	iManufacturer;
	char	iProduct;
	char	iSerialNumber;
	char	bNumConfigurations;
} DEVICE_DESCRIPTOR, *PDEVICE_DESCRIPTOR;

//	Config
typedef struct _CONFIG_DESCRIPTOR{
	char	len;			// 9
	char	dtype;			// 2
	short clen;			// total length
	char	numInterfaces;
	char	config;
	char	iconfig;
	char	attributes;
	char	maxPower;
} CONFIG_DESCRIPTOR, *PCONFIG_DESCRIPTOR;

//	String

//	Interface
typedef struct _INTERFACE_DESCRIPTOR
{
	char len;		// 9
	char dtype;	    // 4
	char number;
	char alternate;
	char numEndpoints;
	char interfaceClass;
	char interfaceSubClass;
	char protocol;
	char iInterface;
} INTERFACE_DESCRIPTOR, *PINTERFACE_DESCRIPTOR;

//	Endpoint
typedef struct _ENDPOINT_DESCRIPTOR
{
	char len;		// 7
	char dtype;	// 5
	char addr;
	char attr;
	short packetSize;
	char interval;
} ENDPOINT_DESCRIPTOR, *PENDPOINT_DESCRIPTOR;

// Interface Association Descriptor
// Used to bind 2 interfaces together in CDC compostite device
typedef struct _IAD_DESCRIPTOR
{
	char len;				// 8
	char dtype;			    // 11
	char firstInterface;
	char interfaceCount;
	char functionClass;
	char funtionSubClass;
	char functionProtocol;
	char iInterface;
} IAD_DESCRIPTOR, *PIAD_DESCRIPTOR;

#define D_DEVICE(_class,_subClass,_proto,_packetSize0,_vid,_pid,_version,_im,_ip,_is,_configs) \
{ 18, 1, USB_VERSION, _class,_subClass,_proto,_packetSize0,_vid,_pid,_version,_im,_ip,_is,_configs }

#define D_CONFIG(_totalLength,_interfaces) \
{ 9, 2, _totalLength,_interfaces, 1, 0, USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USB_CONFIG_POWER_MA(USB_CONFIG_POWER) }

#define D_INTERFACE(_n,_numEndpoints,_class,_subClass,_protocol) \
{ 9, 4, _n, 0, _numEndpoints, _class,_subClass, _protocol, 0 }

#define D_ENDPOINT(_direction, _addr, _attr, _packetSize, _interval) \
{ 7, 5, (_addr) | (_direction), _attr, _packetSize, _interval }

#define D_IAD(_firstInterface, _count, _class, _subClass, _protocol) \
{ 8, 11, _firstInterface, _count, _class, _subClass, _protocol, 0 }


volatile static struct{
	char feature;
	char address;
	char configuration = 1;
	char status;
	
} USB_STATE;

static char wdtcsr_save;

/*****************************************************************************************************************************/
/*                    CDC ACM DESCRIPTORS                                                                                    */
/*****************************************************************************************************************************/


//	Class requests

#define CDC_SET_LINE_CODING			0x20
#define CDC_GET_LINE_CODING			0x21
#define CDC_SET_CONTROL_LINE_STATE	0x22
#define CDC_SEND_BREAK				0x23

#define CDC_V1_10                               0x0110
#define CDC_COMMUNICATION_INTERFACE_CLASS       0x02

#define CDC_CALL_MANAGEMENT                     0x01
#define CDC_ABSTRACT_CONTROL_MODEL              0x02
#define CDC_HEADER                              0x00
#define CDC_ABSTRACT_CONTROL_MANAGEMENT         0x02
#define CDC_UNION                               0x06
#define CDC_CS_INTERFACE                        0x24
#define CDC_CS_ENDPOINT                         0x25
#define CDC_DATA_INTERFACE_CLASS                0x0A


//	CDC CS interface descriptor
typedef struct _CDCCS_INTERFACE_DESCRIPTOR
{
	char len;		// 5
	char dtype;	// 0x24
	char subtype;
	char d0;
	char d1;
} CDCCS_INTERFACE_DESCRIPTOR, *PCDCCS_INTERFACE_DESCRIPTOR;

typedef struct _CDCCS4_INTERFACE_DESCRIPTOR
{
	char len;		// 4
	char dtype;	// 0x24
	char subtype;
	char d0;
} CDCCS4_INTERFACE_DESCRIPTOR, *PCDCCS4_INTERFACE_DESCRIPTOR;

typedef struct _CM_FUNCTIONAL_DESCRIPTOR
{
	char	len;
	char 	dtype;		// 0x24
	char 	subtype;	// 1
	char 	bmCapabilities;
	char 	bDataInterface;
} CM_FUNCTIONAL_DESCRIPTOR, *PCM_FUNCTIONAL_DESCRIPTOR;

typedef struct _ACM_FUNCTIONAL_DESCRIPTOR
{
	char	len;
	char 	dtype;		// 0x24
	char 	subtype;	// 1
	char 	bmCapabilities;
} ACM_FUNCTIONAL_DESCRIPTOR, *PACM_FUNCTIONAL_DESCRIPTOR;



#define D_CDCCS(_subtype,_d0,_d1)	{ 5, 0x24, _subtype, _d0, _d1 }
#define D_CDCCS4(_subtype,_d0)		{ 4, 0x24, _subtype, _d0 }

typedef struct
{
	int32_t	dwDTERate;		// Baudrate in bits per second.
	char	bCharFormat;	// Number of stop bit. (0 = 1 stop bit, 1 = 1.5 stop bits, 2 = 2 stop bits)
	char 	bParityType;	// Parity type. (0 = None, 1 = Odd, 2 = Even, 3 = Mark, 4 = Space)
	char 	bDataBits;		// NUmber of data bits(5, 6, 7, 8 or 16)

	char	lineState; // ??
} LineInfo;


/*****************************************************************************************************************************/
/*                    DEVICE CONFIGURATION:                                                                                  */
/*****************************************************************************************************************************/

static volatile LineInfo _usbLineInfo = { 57600, 0x00, 0x00, 0x00, 0x00 };
static volatile unsigned short _cdcBreakValue;


#define USBCFG_INTERFACE_COUNT 3




// CDC interface:
#define CDC_ACM_INTERFACE	0	// CDC ACM (Abstract Communication Model)
#define CDC_DATA_INTERFACE	1	// CDC Data

#define CDC_ENDPOINT_ACM 1
#define CDC_ENDPOINT_DATA_OUT 2
#define CDC_ENDPOINT_DATA_IN 3


#define USB_MSC_CLASS 0x08    //
#define USB_MSC_SUBCLASS 0x06 // ???
#define USB_MSC_PROTOCOL 0x50 //BULC-ONLY Transport

#define USB_MSC_INTERFACE 2

#define MSC_ENDPOINT_OUT 4
#define MSC_ENDPOINT_IN  5


typedef struct
{
	//	IAD
	IAD_DESCRIPTOR					iad1;	// Only needed on compound device

	//	Control
	INTERFACE_DESCRIPTOR			cif;	//
	CDCCS_INTERFACE_DESCRIPTOR		header;
	CM_FUNCTIONAL_DESCRIPTOR		callManagement;			// Call Management
	ACM_FUNCTIONAL_DESCRIPTOR		controlManagement;		// ACM
	CDCCS_INTERFACE_DESCRIPTOR		functionalDescriptor;	// CDC_UNION
	ENDPOINT_DESCRIPTOR				cifin;

	//	Data
	INTERFACE_DESCRIPTOR			dif;
	ENDPOINT_DESCRIPTOR				in;
	ENDPOINT_DESCRIPTOR				out;

	IAD_DESCRIPTOR					iad2;

	INTERFACE_DESCRIPTOR            mscif;
	ENDPOINT_DESCRIPTOR             mscout;
	ENDPOINT_DESCRIPTOR				mscin;
} CDCMSCDescriptor;

static const CDCMSCDescriptor ConfigurationDescriptorPlus =
{
// first interface, int-face count, 
	D_IAD(0, 2, CDC_COMMUNICATION_INTERFACE_CLASS, CDC_ABSTRACT_CONTROL_MODEL, 0),

	//	CDC communication interface
	D_INTERFACE(CDC_ACM_INTERFACE, 1, CDC_COMMUNICATION_INTERFACE_CLASS, CDC_ABSTRACT_CONTROL_MODEL, 0),
	D_CDCCS(CDC_HEADER, 0x10, 0x01),								// Header (1.10 bcd)
	D_CDCCS(CDC_CALL_MANAGEMENT, 1, 1),							// Device handles call management (not)
	D_CDCCS4(CDC_ABSTRACT_CONTROL_MANAGEMENT, 6),				// SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
	D_CDCCS(CDC_UNION, CDC_ACM_INTERFACE, CDC_DATA_INTERFACE),	// Communication interface is master, data interface is slave 0
	D_ENDPOINT(USB_ENDPOINT_IN, CDC_ENDPOINT_ACM, USB_ENDPOINT_TYPE_INTERRUPT, 0x10, 0x40),

	//	CDC data interface
	D_INTERFACE(CDC_DATA_INTERFACE, 2, CDC_DATA_INTERFACE_CLASS, 0, 0),
	D_ENDPOINT(USB_ENDPOINT_OUT, CDC_ENDPOINT_DATA_OUT, USB_ENDPOINT_TYPE_BULK, 64, 0),
	D_ENDPOINT(USB_ENDPOINT_IN,  CDC_ENDPOINT_DATA_IN,  USB_ENDPOINT_TYPE_BULK, 64, 0),


	D_IAD(2, 1, USB_MSC_CLASS, USB_MSC_SUBCLASS, USB_MSC_PROTOCOL),
	D_INTERFACE(USB_MSC_INTERFACE, 2, USB_MSC_CLASS, USB_MSC_SUBCLASS, USB_MSC_PROTOCOL),
	D_ENDPOINT(USB_ENDPOINT_OUT, MSC_ENDPOINT_OUT, USB_ENDPOINT_TYPE_BULK, 64, 0),
	D_ENDPOINT(USB_ENDPOINT_IN,  MSC_ENDPOINT_IN,  USB_ENDPOINT_TYPE_BULK, 64, 0)
};


static const CONFIG_DESCRIPTOR ConfigurationDescriptor =
D_CONFIG(
sizeof(ConfigurationDescriptorPlus) + sizeof(CONFIG_DESCRIPTOR),
USBCFG_INTERFACE_COUNT
)
;

static const DEVICE_DESCRIPTOR  DeviceDescriptor =
{
	0x12,   // length
	0x01,   // type
	0x200,  // usbVersion
	0xEF,   // deviceClass
	0x02,   // deviceSubClass
	0x01,   // deviceProtocol
	64,     // packetSize0
	0x1f1f, //vid
	0xf1f1, //pid
	0x100,  // deviceVersion
	1,		// iManufacture TODO
	2,		// iProduct TODO
	3,      // iSerialNUmber TODO
	1		//bNumConfiguration
};

static const short STRING_LANGUAGE[2] = {
	(3<<8) | (2+2),
	0x0409	// English
};


static const char STRINGS_SET[][16] = {
	{ 3 }, // count
	{5, 'd', 'a', 'r', 'm', 'a', 0},     // Manufacture
	{4, 't', 'e', 's', 't', 0},  // Product
	{3, 'M', 'S', 'D', 0}   // Serial
};

static const char _initEndpoints[] =
{
	0,                      // Control Endpoint
	
	USB_EP_TYPE_INTERRUPT_IN,   // CDC_ENDPOINT_ACM
	USB_EP_TYPE_BULK_OUT,       // CDC_ENDPOINT_OUT
	USB_EP_TYPE_BULK_IN,        // CDC_ENDPOINT_IN
	USB_EP_TYPE_BULK_OUT,		// MSC_ENDPOINT_OUT
	USB_EP_TYPE_BULK_IN,		// MSC_ENDPOINT_IN

	0
};

/*****************************************************************************************************************************/
/*                    USB CORE CONTROL FUNCTIONS                                                                             */
/*****************************************************************************************************************************/

bool usb_core_init();

bool SendDescriptor(USBSetup* Setup);

bool ClassControlRequest(USBSetup* Setup);

bool ReceivedSetupInterrupt();
void ClearSetupInterrupt();

/************************************************************************/
/*                   END POINT CONTROL                                  */
/************************************************************************/

bool InitializeEndpoint(char Index, char Type, char Size);

void SetEndpoint(char Index);

void ReadData(volatile char* Data, char Count);
void SendBytes(const uint8_t ep, char* Ptr, const uint8_t Count);
void SendByte(char Byte);

void WaitInToken();
void CleanIN();

void WaitOutToken();
void ClearOUT();

// IN - true, OUT - false
bool WaitINOUT();


static short _cmark;
static short _cend;
void InitControlTransfer(const short End);
bool SendControlTransfer(const char Data);
bool USBSendControlTransfer(const char Flags, const char* Data, short Length);

bool USBRecvControlTransfer(char* dist, short size);




bool SendUsb(const char epid, const char* data, short len);

bool CDC_Send_Data(const char* data, short length);
bool CDC_Send_Byte(uchar data);


/*****************************************************************************************************************************/
/*                    USB MSC BBB (Bulk-Only)                                                                             */
/*****************************************************************************************************************************/


#define USB_MSC_BBB_RESET 0xFF
#define USB_MSC_BBB_LUN   0xFE

typedef struct
{
	uint32_t dCBWSignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bCBWFlags;
	// Bit 7 Direction, device shall ignore this bit, if the dCBWDataTransfeLength field is zero, otherwise: 0 h->d, 1 d->h
	// Bit 6 - obsolete. Bit 5..0 - reserved.

	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
} USB_MSD_CBW;

typedef struct
{
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} USB_MSD_CSW;


//==============================================================================================
//  Mandatory SCSI commands and common optional SCSI commands
// for USB mass-storage devices that comply with SBC-2 or SBC-3.
//==============================================================================================

	//request for device information struct 
	//Мы будем отвечать только на запрос вида: 12 00 00 00 24 00 (EVPD и CMDDT равны 0), иначе будем отвечать ошибкой в CSW.
#define INQUIRY                       (unsigned char) 0x12
#define READ_FORMAT_CAPACITY          (unsigned char) 0x23 // todo ...

// uint8_t capacity[8] = {
//         0x00, 0x00, 0x0F, 0xFF, //Addr last blocks = 2M/512 - 1
//         0x00, 0x00, 0x02, 0x00      //Size blocks = 512 bytes
// };
#define READ_CAPACITY                 (unsigned char) 0x25

	
#define READ_10                       (unsigned char) 0x28


#define WRITE_10                      (unsigned char) 0x2a 

	//Если хост принял CSW с полем bCSWStatus = 1, он может послать команду REQUEST_SENSE, чтобы запросить пояснительные данные (SENSE DATA).
// 	uint8_t sense_data[18] = {
// 		0x70,       //VALID = 1, RESRONSE_CODE = 0x70
// 		0x00,
// 		0x05,       //S_ILLEGAL_REQUEST
// 		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// 	};
#define REQUEST_SENSE                 (unsigned char) 0x03 

// 	uint8_t mode_sense_6[4] = {
// 		0x03, 0x00, 0x00, 0x00,
// 	};
#define MODE_SENSE                    (unsigned char) 0x1a 

#define TEST_UNIT_READY               (unsigned char) 0x00 
#define VERIFY                        (unsigned char) 0x2F // todo
#define PREVENT_ALLOW_MEDIUM_REMOVAL  (unsigned char) 0x1e // todo

//==============================================================================================
//The Inquiry Responsev (device information struct)
//==============================================================================================
typedef struct
{
	unsigned char Peripheral; // direction access
	unsigned char Removble;
	unsigned char Version;    // protocol version, 02 - SPC-2
	unsigned char Response_Data_Format;
	unsigned char AdditionalLength;
	unsigned char Sccstp;
	unsigned char bqueetc;
	unsigned char CmdQue;
	unsigned char vendorID[8];
	unsigned char productID[16];
	unsigned char productRev[4];
} CInquiryResponse;

const static CInquiryResponse inq_resp =
{
	0x00,                                                             // direct access block device, conne
	0x80,                                                             // device is removable
	0x02,                                                             // SPC-2 compliance
	0x02,                                                             // response data format
	0x1F,                                                             // response has 20h + 4 bytes
	0x00, 0x00, 0x00,                                                 // additional fields, none set
	'A','T','m','e','g','a',' ','0',                                  // 8 -byte T10-assigned Vendor ID
	'V','-','U','S','B',' ','S','D','-','A','d','a','p','t','e','r',  // 16-byte product identification
	'0','0','0','8'                                                   // 4-byte product revision level
};

static uint8_t sense_data[18] = {
	0x70,       //VALID = 1, RESRONSE_CODE = 0x70
	0x00,
	0x05,       //S_ILLEGAL_REQUEST
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8_t capacity[8] = {
	0x00, 0x3F, 0xFF, 0xFF, //Addr last blocks = 2G/512 - 1
	0x00, 0x00, 0x02, 0x00      //Size blocks = 512 bytes
};

static const unsigned char usb_format_capacity[] =
{
	0x00, 0x00, 0x00, 0x08,
	0x00, 0xF2, 0x38, 0x00,
	0x02,
	0x00, 0x02, 0x00
};

static uint8_t mode_sense[4] = {
	0x03, 0x00, 0x00, 0x00
};

void SCSI_Execute();


#endif /* AVRUSBCORE_H_ */