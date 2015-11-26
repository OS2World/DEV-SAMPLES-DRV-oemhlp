
This file contains sample code for PCI functions.  This sample code is
provided "AS IS" and, therefore, assistance with its contents is limited.



/*  Sample Code to access OEMHLP PCI Functions - IOCtl Approach  */



#define OEMHLP_IOCTL        0x80        /* OEM Help IOCtl Category           */
#define PCI_FUNC            0x0b        /* OEM Help PCI Function Code        */
#define PCI_GET_BIOS_INFO   0           /*  PCI Sub-Function Definitions     */
#define PCI_FIND_DEVICE     1
#define PCI_FIND_CLASS_CODE 2
#define PCI_READ_CONFIG     3
#define PCI_WRITE_CONFIG    4

struct PCI_DATA                         /* PCI Complement of POS_Data Struct */
  {
    ULONG  ulIOBase;                /* IO Base Address (IOBAR)           */
    ULONG  ulMemBase;               /* Memory Base Address (HMBAR)       */
    USHORT usIntrReg;               /* Interrupt Register                */
  };

struct PCI_CARD                         /* Data Returned from Find_Device    */
  {                                     /*  PCI DosDevIOCtl Function Call    */
    UCHAR  BusNum;
    UCHAR  DevFuncNum;
  };

typedef struct _PCI_PARM                /* Parameter Packet Definitions for  */
  {                                     /* Input to PCI DosDevIOCtl Function */
   UCHAR PCISubFunc;
   union
     {
      struct
       {
         USHORT DeviceID;
         USHORT VendorID;
         UCHAR  Index;
       }ParmFindDev;
      struct
       {
         UCHAR  BusNum;
         UCHAR  DevFunc;
         UCHAR  ConfigReg;
         UCHAR  Size;
       }ParmReadConfig;
     };
  } PCI_PARM;

typedef struct _PCI_DATA                /* Data Packet Definitions for Info  */
  {                                     /* Returned from PCI DosDevIOCtl     */
   UCHAR bReturn;                       /* Functions                         */
   union
    {
      struct
       {
         UCHAR HWMech;
         UCHAR MajorVer;
         UCHAR MinorVer;
         UCHAR LastBus;
       } DataBiosInfo;
      struct
       {
         UCHAR  BusNum;
         UCHAR  DevFunc;
       }DataFindDev;
      struct
       {
         ULONG  Data;
       }DataReadConfig;
    };
  } PCI_DATA;

  HFILE  hFileHandle;                   /* Handle to OEMHlp Routines         */

/*============================================================================*
*                                                                             *
*  InitForPCI_Use - This routine initializes the driver to access the PCI bus.*
*                                                                             *
*  Input Parameters:                                                          *
*                                                                             *
*     None                                                                    *
*                                                                             *
*  Output Parameters:                                                         *
*                                                                             *
*     pMinorVersion - Pointer to PCI Bus Minor Version # variable             *
*     Return code    - Boolean value indicating if we successfully            *
*                      initialized for PCI access.                            *
*                                                                             *
* Global Variables Accessed:                                                  *
*    hFileHandle                                                              *
*                                                                             *
=============================================================================*/

boolean InitForPCI_Use (USHORT *pMinorVersion)
{

  PCI_PARM   PCIParmPkt;
  PCI_DATA   PCIDataPkt;

  USHORT action,                        /* Action FROM DosOpen Call      */
             rc;                        /* Return Code Value             */

  /* Initialize Value to an Absurd Version #                                 */
  *pMinorVersion = (USHORT)0xff;

  /*  Open OEM Help Functions for PCI BIOS Access                            */
  /*  IF Unavailable - Return to Caller with Negative Return Code            */
  if (rc = DosOpen("OEMHLP$",           /* 'File' to Open                    */
                   &hFileHandle,        /* Handle Returned to us             */
                   &action,             /* Action Taken by DosOpen()         */
                   0L,                  /* File Size (Not Used)              */
                   0,                   /* File Attribute - Read Only        */
                   1,                   /* Open Flag - Open If Exists        */
                   0x40,                /* Open Mode - ???                   */
                   0L))                 /* Extended Attr Buffer (Not Used)   */
      return (FALSE);

  /* Setup IOCTL Request Parm Packet - Query PCI BIOS Info SubFunction       */
  PCIParmPkt.PCISubFunc = PCI_GET_BIOS_INFO;

  rc = DosDevIOCtl ((PVOID)&PCIDataPkt,     /* Data Packet Addr              */
                    (PVOID)&PCIParmPkt,     /* Parameter Packet Addr         */
                    (USHORT)PCI_FUNC,   /* OEMHelp Function (0Bh)        */
                    (USHORT)OEMHLP_IOCTL,  /* OEMHelp Category (80h)     */
                    hFileHandle);           /* Handle to OEMHelp             */

  if ((rc != 0) || (PCIDataPkt.bReturn != 0))
    return (FALSE);

  /* Return the PCI Bus Minor Version #.  If v2.10 (MinorVer == 10) then     */
  /* We can Determine the Slot # for each Card Found                         */
  *pMinorVersion = (USHORT)PCIDataPkt.DataBiosInfo.MinorVer;

   return (TRUE);

} /** END InitForPCI_Use() **/


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

  FindPCICard - This Routine Finds an(other) ARTIC PCI Card.

  Input Parameters:

     CardNumber  - The Card (Index) Number.

  Output Parameters:

     PCICardPtr  - The PCI Card Data is Returned Using this Pointer
                     (BusNum / DevFuncNum)
     Return code - Indicates the Success or Failure of the Call.
                   (86h => Device Not Found)

  Global Variables Accessed:
     hFileHandle

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

USHORT FindPCICard(USHORT Index,
                       struct PCI_CARD *PCICardPtr)
{
  USHORT rc;

  PCI_PARM  PCIParmPkt;
  PCI_DATA  PCIDataPkt;

  /* Setup IOCTL Request Parm Packet                                         */
  /*  - Find (Next) Device SubFunction (01)                                  */
  /*  - ARTIC Card Device ID           (0036h)                               */
  /*  - ARTIC Card Vendor (IBM) ID     (1014h)                               */
  PCIParmPkt.PCISubFunc = PCI_FIND_DEVICE;
  PCIParmPkt.ParmFindDev.DeviceID = (USHORT)ARTIC_DEV_ID;
  PCIParmPkt.ParmFindDev.VendorID = (USHORT)ARTIC_VENDOR_ID;
  PCIParmPkt.ParmFindDev.Index = Index;

  rc = DosDevIOCtl ((PVOID)&PCIDataPkt,     /* Data Packet Addr              */
                    (PVOID)&PCIParmPkt,     /* Parameter Packet Addr         */
                    (USHORT)PCI_FUNC,   /* OEMHelp Function (0Bh)        */
                    (USHORT)OEMHLP_IOCTL, /* OEMHelp Category (80h)      */
                    hFileHandle);           /* Handle to OEMHelp             */

  if ((rc != 0) || (PCIDataPkt.bReturn != 0))
    return (rc);

  /* Get Card Data Returned     */
  PCICardPtr -> BusNum = PCIDataPkt.DataFindDev.BusNum;
  PCICardPtr -> DevFuncNum = PCIDataPkt.DataFindDev.DevFunc;

  return (rc);

} /** END FindPCICard() **/


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*

  GetPCICardData - This Routine Retrieves the Configuration Data for a Card

  Input Parameters:

     PCICardPtr  - Pointer to Device Info (Bus # / DevFunc #) Structure
     CfgRegister - PCI Card Configuration Register to Read
     &pCardData  - Pointer to Var for Returned Data
                       (From the Corresponding Configuration Register)

  Output Parameters:

     CardData    - Data Returned to Caller
     Return code - Indicates the Success or Failure of the Call.

  Global Variables Accessed:
     hFileHandle

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

USHORT GetPCICardData (struct PCI_CARD  *PCICardPtr,
                        UCHAR     CfgRegister,
                        ULONG *pCardData)

{
  USHORT rc;

  PCI_PARM   PCIParmPkt;
  PCI_DATA   PCIDataPkt;

  /* Setup IOCTL Request Parm Packet                                         */
  /*  - Read PCI Config Space SubFunction  (03)                              */
  /*  - Card Bus Number                                                      */
  /*  - Card DevFunc Number                                                  */
  /*  - Configuration Register to Read                                       */
  /*  - Data Size (DWORD)                                                    */
  PCIParmPkt.PCISubFunc = PCI_READ_CONFIG;

  PCIParmPkt.ParmReadConfig.BusNum = PCICardPtr -> BusNum;
  PCIParmPkt.ParmReadConfig.DevFunc = PCICardPtr -> DevFuncNum;
  PCIParmPkt.ParmReadConfig.ConfigReg = CfgRegister;
  PCIParmPkt.ParmReadConfig.Size = (UCHAR)DWORD_DATA;

  rc = DosDevIOCtl ((PVOID)&PCIDataPkt,     /* Data Packet Addr              */
                    (PVOID)&PCIParmPkt,     /* Parameter Packet Addr         */
                    (USHORT)PCI_FUNC,   /* OEMHelp Function (0Bh)        */
                    (USHORT)OEMHLP_IOCTL,  /* OEMHelp Category (80h)     */
                    hFileHandle);           /* Handle to OEMHelp             */

  if ((rc != 0) || (PCIDataPkt.bReturn != 0))
    return (rc);

  /* Get Card Data Returned     */
  *pCardData = PCIDataPkt.DataReadConfig.Data;

  return (rc);

} /** END GetPCICardData() **/

