#include "zte_download.h"
#include "misc.h"

const stSTAT m_DownloadImageTable[] =
{
    {STAT_TERMINATE, STAT_TERMINATE},          // 0
    {STAT_TERMINATE, STAT_TERMINATE},          // 1
    {STAT_TERMINATE, STAT_TERMINATE},          // 2
    {STAT_TERMINATE, STAT_TERMINATE},          // 3
    {STAT_TERMINATE, STAT_TERMINATE},          // 4
    {STAT_EXECUTE_CODE, STAT_TERMINATE},       // 5
    {STAT_ERASE_SEG_ST_ADDR, STAT_TERMINATE},  // 6
    {STAT_TERMINATE, STAT_TERMINATE},          // 7
    {STAT_TERMINATE, STAT_TERMINATE},          // 8
    {STAT_PROG_DEV_END_ADDR, STAT_TERMINATE},  // 9
    {STAT_SET_PACKET_SIZE, STAT_TERMINATE},    // 10
    {STAT_DATAPACKET, STAT_TERMINATE},         // 11
    {STAT_TERMINATE, STAT_TERMINATE},          // 12
    {STAT_ERASE_SEG_END_ADDR, STAT_TERMINATE}, // 13
    {STAT_PROG_DEV_ST_ADDR, STAT_TERMINATE},   // 14
    {STAT_TERMINATE, STAT_TERMINATE},          // 15
    {STAT_TERMINATE, STAT_TERMINATE},          // 16
    {STAT_TERMINATE, STAT_TERMINATE}           // 17
};

const stSTAT m_DownloadRAMImageTable[] =
{
    {STAT_TERMINATE, STAT_TERMINATE},         // 0
    {STAT_TERMINATE, STAT_TERMINATE},         // 1
    {STAT_TERMINATE, STAT_TERMINATE},         // 2
    {STAT_TERMINATE, STAT_TERMINATE},         // 3
    {STAT_TERMINATE, STAT_TERMINATE},         // 4
    {STAT_EXECUTE_CODE, STAT_TERMINATE},      // 5
    {STAT_PROG_DEV_ST_ADDR, STAT_TERMINATE},  // 6
    {STAT_TERMINATE, STAT_TERMINATE},         // 7
    {STAT_TERMINATE, STAT_TERMINATE},         // 8
    {STAT_PROG_DEV_END_ADDR, STAT_TERMINATE}, // 9
    {STAT_SET_PACKET_SIZE, STAT_TERMINATE},   // 10
    {STAT_DATAPACKET, STAT_TERMINATE},        // 11
    {STAT_TERMINATE, STAT_TERMINATE},         // 12
    {STAT_TERMINATE, STAT_TERMINATE},         // 13
    {STAT_TERMINATE, STAT_TERMINATE},         // 14
    {STAT_TERMINATE, STAT_TERMINATE},         // 15
    {STAT_TERMINATE, STAT_TERMINATE},         // 16
    {STAT_TERMINATE, STAT_TERMINATE}          // 17
};

FILE *pf = NULL;
char g_filename[MAXFILEPATH] = {0};
DWORD m_dwPartitionSize = 0;
DWORD m_dwBootStage1StartAddress = 0x00082000; //Tloader download address
DWORD m_dwBootStage1Size = 0;         //Tloader size
DWORD m_dwBootStage2StartAddress = 0; //boot download address
DWORD m_dwBootStage2Size = 0;       //boot size
DWORD m_iFileNum = 0;
BOOL m_bCRCCheck = FALSE;
const stSTAT *m_pProtocolTable = NULL;
stBinMasterInfo m_stMasterInfo;
stFileItem FileItem[MAXFILENUM];
int g_DownLoadFileNum = 0;

static BOOL FileAnalysis();
static BOOL EraseNVRW();
static BOOL DoDownload(enDownloadType DownloadType, BOOL bEableCRCCheck);

void DumpHex(char *buf, int size)
{
    if (buf == NULL || size == 0) {
        return;
    }
#if 0
    int i = 0;
    for(i=0; i<size; i++) {
        printf("%02X,", (BYTE)buf[i]);
        if (((i+1)%16) == 0) {
            printf("\n");
        }
    }
    printf("\n");
#endif
}

BOOL ExecuteEraseNVRW(char *softwarepath)
{
    strcpy(g_filename, softwarepath);

    if (!EraseNVRW())
    {
        LogInfo("FAIL\r\n");
        return FALSE;
    }
    LogInfo("OK\r\n");

    return TRUE;

}

BOOL PrimaryDoDownload(char *softwarepath)
{
    strcpy(g_filename, softwarepath);

    if (!DoDownload(DOWNLOAD_TYPE_IMAGE, FALSE))
    {
        LogInfo("Image FAIL\r\n");
        return FALSE;
    }

    LogInfo("Image OK\r\n");

    return TRUE;
}

BOOL Initialize()
{
    m_pProtocolTable = NULL;
    return TRUE;
}

BOOL UnInitialize()
{
    return TRUE;
}

static void GetFileList(enDownloadType DownloadType)
{
    switch ((WORD)DownloadType)
    {
        case DOWNLOAD_TYPE_IMAGE:
            if (FileAnalysis())
            {
                LogInfo("FileAnalysis OK\r\n");
            }
            break;
        default:;
    }
}

static BOOL FileAnalysis()
{
    int fileIndex = 0;
    BOOL bRet = FALSE;
    stBinImageHead BinFileHeader;
    DWORD i = 0;

    if (access(g_filename, 0) != 0)
    {
        LogInfo("%s is not found.\r\n", g_filename);
        return FALSE;
    }

    pf = fopen(g_filename, "rb");
    if (pf == NULL)
    {
        LogInfo("Open file %s failed.\r\n", g_filename);
    }
    else
    {
        LogInfo("Open file OK, g_filename: %s\r\n", g_filename);
    }

    if (m_iFileNum <= 0)
    {
        LogInfo("file num error m_iFileNum:%d\r\n", m_iFileNum);
        return bRet;
    }
    fseek(pf, sizeof(m_stMasterInfo), 0);

    for (i = 0; i < m_iFileNum; i++)
    {
        if (fread((BYTE *)&BinFileHeader, 1, sizeof(stBinImageHead), pf) != sizeof(stBinImageHead))
        {
            LogInfo("Read file FAIL\r\n");
            bRet = FALSE;
        }
        if (memcmp(BinFileHeader.PartitionName, "NVR", 3) != 0)
        {
            LogInfo("not NVR file, BinFileHeader.PartitionName:%s\r\n", BinFileHeader.PartitionName);
            fileIndex++;
            g_DownLoadFileNum++;
        }
        else
        {
            LogInfo("NVR file, BinFileHeader.PartitionName:%s\r\n", BinFileHeader.PartitionName);
            continue;
        }
    //shijiaxing 2024-0926 modify for MTC0356-1582
#if __BYTE_ORDER != __LITTLE_ENDIAN  //from little to big-endian
        DWORD change = 0;
        change = BinFileHeader.iFileLength;
        BinFileHeader.iFileLength = BSWAP_32(change);
        change = BinFileHeader.iPartitionOffset;
        BinFileHeader.iPartitionOffset = BSWAP_32(change);
        change = BinFileHeader.iFileOffset;
        BinFileHeader.iFileOffset = BSWAP_32(change);
#endif

        FileItem[fileIndex].bDownload = TRUE;
        FileItem[fileIndex].strFileName = g_filename;
        FileItem[fileIndex].bUseRelativePath = 0;
        FileItem[fileIndex].bReadFromFile = 1;
        FileItem[fileIndex].bAutoDeleteImageCache = FALSE;
        FileItem[fileIndex].pImageCache = NULL;
        FileItem[fileIndex].dwOffset = BinFileHeader.iFileOffset;
        strcpy(FileItem[fileIndex].PartitionName, (char *)BinFileHeader.PartitionName);
        FileItem[fileIndex].dwLength = BinFileHeader.iFileLength;
        FileItem[fileIndex].nPartitionOffset = BinFileHeader.iPartitionOffset;
        DumpHex((char *)BinFileHeader.PartitionType, 16);
        strcpy(FileItem[fileIndex].PartitionType, (char *)BinFileHeader.PartitionType);
        FileItem[fileIndex].nType = nand;

        LogInfo("FileItem[%d].bDownload:%d\r\n", fileIndex, FileItem[fileIndex].bDownload);
        if (FileItem[fileIndex].strFileName != NULL)
        {
            LogInfo("FileItem[%d].strFileName:%s\r\n", fileIndex, FileItem[fileIndex].strFileName);
        }
        else
        {
            LogInfo("FileItem[%d].strFileName ptr is NULL\r\n", fileIndex);
        }
        LogInfo("FileItem[%d].bUseRelativePath:%d\r\n", fileIndex, FileItem[fileIndex].bUseRelativePath);
        LogInfo("FileItem[%d].bReadFromFile:%d\r\n", fileIndex, FileItem[fileIndex].bReadFromFile);

        LogInfo("FileItem[%d].bAutoDeleteImageCache:%d\r\n", fileIndex, FileItem[fileIndex].bAutoDeleteImageCache);

        if (FileItem[fileIndex].pImageCache != NULL)
        {
            LogInfo("FileItem[%d].pImageCache:%s\r\n", fileIndex, FileItem[fileIndex].pImageCache);
        }
        else
        {
            LogInfo("FileItem[%d].pImageCache ptr is NULL\r\n", fileIndex);
        }
        LogInfo("FileItem[%d].dwOffset:%d\r\n", fileIndex, FileItem[fileIndex].dwOffset);
        LogInfo("FileItem[%d].PartitionName:%s\r\n", fileIndex, FileItem[fileIndex].PartitionName);
        LogInfo("FileItem[%d].dwLength:%d\r\n", fileIndex, FileItem[fileIndex].dwLength);
        LogInfo("FileItem[%d].nPartitionOffset:%d\r\n", fileIndex, FileItem[fileIndex].nPartitionOffset);
        LogInfo("FileItem[%d].PartitionType:%s\r\n", fileIndex, FileItem[fileIndex].PartitionType);
        LogInfo("FileItem[%d].nType:%d\r\n", fileIndex, FileItem[fileIndex].nType);
    }
    bRet = TRUE;
    if (pf != NULL)
    {
        fclose(pf);
        pf = NULL;
        LogInfo("fclose(pf)\r\n");
    }
    return bRet;
}

static void CollectDownloadImage(enDownloadType DownloadType)
{
    GetFileList(DownloadType);
    LogInfo("OK\r\n");
}

enSTAT GotoNextState(enSTAT enCurrentStat, BOOL bRet)
{
    enSTAT NextState = STAT_TERMINATE;
    int nDim = 0;

    if (NULL == m_pProtocolTable)
    {
        LogInfo("NULL == m_pProtocolTable\r\n");
        return NextState;
    }

    if (m_DownloadImageTable == m_pProtocolTable)
    {
        nDim = sizeof(m_DownloadImageTable) / sizeof(m_DownloadImageTable[0]);
    }
    else if (m_DownloadRAMImageTable == m_pProtocolTable)
    {
        nDim = sizeof(m_DownloadRAMImageTable) / sizeof(m_DownloadRAMImageTable[0]);
    }
    else
    {
        nDim = 0;
    }

    if ((int)enCurrentStat >= nDim)
    {
        if (m_DownloadImageTable == m_pProtocolTable)
        {
            LogInfo("m_pProtocolTable -> m_DownloadImageTable\r\n");
        }
        else if (m_DownloadRAMImageTable == m_pProtocolTable)
        {
            LogInfo("m_pProtocolTable -> m_DownloadRAMImageTable\r\n");
        }
        else
        {
            LogInfo("error state\r\n");
        }
        return NextState;
    }

    if (TRUE == bRet)
    {
        NextState = m_pProtocolTable[enCurrentStat].mNextTRUE;
    }
    else
    {
        NextState = m_pProtocolTable[enCurrentStat].mNextFALSE;
    }

    return NextState;
}

BOOL DownloadOneImage(stDownloadConfig CurrentConfig)
{
    enSTAT State = CurrentConfig.mStartState;
    BOOL bRet = FALSE;
    BOOL bExit = FALSE;
    ULONG ulAckCRCValue = 0;
    DWORD dwCurrentSize = 0;
    BYTE AckBuffer[255] = {0};
    BYTE cmdbuf[255] = {0};
    BYTE *WriteBuffer = NULL;
    DWORD dwPacketSize = 0;
    DWORD dwDDRPacketSize = 0;
    BYTE *pBuffer = CurrentConfig.mCurrentFile.pImageCache;
    DWORD dwFileSize = 0;

    if (access(g_filename, 0) != 0)
    {
        LogInfo("software file path is error.\r\n");
        return FALSE;
    }
    pf = fopen(g_filename, "rb");
    if (pf == NULL)
    {
        LogInfo("Open file FAIL, g_filename: %s\r\n", g_filename);
        return FALSE;
    }
    LogInfo("Open %s OK\r\n", g_filename);

    fseek(pf, CurrentConfig.mCurrentFile.dwOffset, 0);
    while (!bExit)
    {
        switch (State)
        {
        case STAT_SET_PACKET_SIZE:
        {
            memset(AckBuffer, 0, sizeof(AckBuffer));
            LogInfo("STAT_SET_PACKET_SIZE\r\n");

            memset(cmdbuf, 0, 255);
            sprintf((char *)cmdbuf, "compat_write %s %08x %0x", CurrentConfig.mCurrentFile.PartitionName,
                        CurrentConfig.mCurrentFile.nPartitionOffset, CurrentConfig.mCurrentFile.dwLength);
            LogInfo("cmdbuf: %s\r\n", cmdbuf);
            dwFileSize = CurrentConfig.mCurrentFile.dwLength;
            bRet = SendData(cmdbuf, strlen((const char *)cmdbuf) + 1, 20, 1);
            if (bRet)
            {
                //AckBuffer = (BYTE *)ReadDataExtraFuncB(14, 10 , 30);
                memset(AckBuffer, 0, sizeof(AckBuffer));
                bRet = ReadData(AckBuffer, 14, 10, 4 * 60);
            }
            LogInfo("AckBuffer: 0x%x\r\n", AckBuffer[0]);

            //m_pTransmit->AddAction(ACTION_SEND, cmdbuf,strlen((const char*)cmdbuf)+1, 20);
            //m_pTransmit->AddAction(ACTION_RECEIVE, AckBuffer, 14, 10, 400); //13:data 00200000
            //bRet = m_pTransmit->ExecuteAction();
            if ((AckBuffer[0] == 0x00) && (AckBuffer[1] == 0X44))
            {
                int indexextra = 0;
                for (indexextra = 0; indexextra < 14; indexextra++)
                {
                    AckBuffer[indexextra] = AckBuffer[indexextra + 1];
                }
            }
            if (bRet && (!memcmp(AckBuffer, "DATA", 4)))
            {
                LogInfo("STAT_SET_PACKET_SIZE OK\r\n");
                dwDDRPacketSize = strtoul((char *)AckBuffer + 4, 0, 16);
                if (dwDDRPacketSize > CurrentConfig.mCurrentFile.dwLength)
                {
                    LogInfo("return data length greater than file total length, failed, dwDDRPacketSize:%d, CurrentConfig.mCurrentFile.dwLength:%d\r\n",
                    dwDDRPacketSize, CurrentConfig.mCurrentFile.dwLength);

                    State = GotoNextState(State, FALSE);
                    bRet = FALSE;
                }
                else
                {
                    LogInfo("return data length  <= file total length, dwDDRPacketSize:%d, CurrentConfig.mCurrentFile.dwLength:%d\r\n", dwDDRPacketSize, CurrentConfig.mCurrentFile.dwLength);
                    dwCurrentSize = dwDDRPacketSize;
                    State = GotoNextState(State, TRUE);
                }
            }
            else
            {
                LogInfo("STAT_SET_PACKET_SIZE failed\r\n");
                State = GotoNextState(State, FALSE);
                bRet = FALSE;
            }
        }
        break;
        case STAT_DATAPACKET:
        {
            dwFileSize -= dwCurrentSize;
            if (dwCurrentSize >= CurrentConfig.mPacketSize)
            {
                dwPacketSize = CurrentConfig.mPacketSize;
            }
            else
            {
                dwPacketSize = dwCurrentSize;
            }

            if (WriteBuffer != NULL) {
                free(WriteBuffer);
                WriteBuffer = NULL;
            }
            WriteBuffer = (BYTE *)malloc(dwPacketSize);
            if (WriteBuffer == NULL) {
                LogInfo("allocate WriteBuffer failed.\r\n");
                bExit = TRUE;
                bRet = FALSE;
                break;
            }

            if (dwCurrentSize > 0)
            {
                while (1)
                {
                    memset(WriteBuffer, 0, dwPacketSize);
                    if (CurrentConfig.mCurrentFile.bReadFromFile)
                    {
                        if (dwCurrentSize > dwPacketSize)
                        {
                            // dwPacketSize = dwPacketSize;
                        }
                        else
                        {
                            dwPacketSize = dwCurrentSize;
                            if ((dwPacketSize % 512) == 0)
                            {
                                dwPacketSize -= 8;
                                LogInfo("STAT_DATAPACKET: split packet\r\n");
                            }
                            else
                            {
                                usleep(2 * 1000);
                            }
                        }
                        if (fread(&WriteBuffer[0], 1, dwPacketSize, pf) != dwPacketSize)
                        {
                            LogInfo("read packte failed\r\n");
                            State = GotoNextState(State, FALSE);
                            if (WriteBuffer != NULL)
                            {
                                free(WriteBuffer);
                                WriteBuffer = NULL;
                            }
                            bRet = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        if (CurrentConfig.mCurrentFile.pImageCache != NULL)
                        {
                            memcpy(&WriteBuffer[0], pBuffer, dwPacketSize);
                            pBuffer += dwPacketSize;
                        }
                    }

                    bRet = SendData(WriteBuffer, dwPacketSize, 20, 1);
                    if (bRet == TRUE)
                    {
                        // LogInfo("STAT_DATAPACKET: send one packet success\r\n");
                    }
                    else
                    {
                        LogInfo("STAT_DATAPACKET: send one packet FAIL!\r\n");
                        State = GotoNextState(State, FALSE);
                        bRet = FALSE;
                        break;
                    }
                    dwCurrentSize -= dwPacketSize;
                    if (dwCurrentSize == 0)
                    {
                        LogInfo("STAT_DATAPACKET: dwCurrentSize == 0\r\n");
                        if (dwFileSize > 0)
                        {
                            //AckBuffer = (BYTE *)ReadDataExtraFuncB(14, 10 , 30);
                            memset(AckBuffer, 0, sizeof(AckBuffer));
                            bRet = ReadData(AckBuffer, 14, 10, 4*60);
                            LogInfo("AckBuffer:%x\r\n", AckBuffer[0]);
                            if ((AckBuffer[0] == 0x00) && (AckBuffer[1] == 0X44))
                            {
                                int indexextra = 0;
                                for (indexextra = 0; indexextra < 14; indexextra++)
                                {
                                    AckBuffer[indexextra] = AckBuffer[indexextra + 1];
                                }
                            }
                            if (bRet && (!memcmp(AckBuffer, "DATA", 4)))
                            {
                                LogInfo("STAT_SET_PACKET_SIZE OK\r\n");

                                dwDDRPacketSize = strtoul((char *)AckBuffer + 4, 0, 16);
                                if (dwDDRPacketSize > CurrentConfig.mCurrentFile.dwLength)
                                {
                                    LogInfo("return data length greater than file total length, failed, dwDDRPacketSize:%d, CurrentConfig.mCurrentFile.dwLength:%d\r\n",
                                                dwDDRPacketSize, CurrentConfig.mCurrentFile.dwLength);
                                    State = GotoNextState(State, FALSE);
                                    bRet = FALSE;
                                }
                                else
                                {
                                    LogInfo("return data length <= file total length, dwDDRPacketSize:%d, CurrentConfig.mCurrentFile.dwLength:%d\r\n",
                                            dwDDRPacketSize, CurrentConfig.mCurrentFile.dwLength);
                                    dwCurrentSize = dwDDRPacketSize;
                                }
                            }
                            else
                            {
                                LogInfo("STAT_SET_PACKET_SIZE failed\r\n");
                                State = GotoNextState(State, FALSE);
                                bRet = FALSE;
                            }
                        }
                        else
                        {
                            if (m_bCRCCheck)
                            {
                                bRet = ReadData((BYTE *)&ulAckCRCValue, 4, 30, 4*60);
                                LogInfo("CurrentConfig.mCurrentFile.ulCRCValue:%ld ulAckCRCValue:%ld\r\n", CurrentConfig.mCurrentFile.ulCRCValue, ulAckCRCValue);
                                if (bRet && (CurrentConfig.mCurrentFile.ulCRCValue == ulAckCRCValue))
                                {
                                    LogInfo("Image data send success!CurrentConfig.mCurrentFile.ulCRCValue:%ld ulAckCRCValue:%ld\r\n", CurrentConfig.mCurrentFile.ulCRCValue, ulAckCRCValue);
                                    bRet = TRUE;
                                    State = GotoNextState(State, TRUE);
                                }
                                else
                                {
                                    LogInfo("Image data send failed!\r\n");
                                    bRet = FALSE;
                                    State = GotoNextState(State, FALSE);
                                }
                            }
                            else
                            {
                                LogInfo("No CRCCheck\r\n");
                                memset(AckBuffer, 0, sizeof(AckBuffer));
                                bRet = ReadData(AckBuffer, 5, 10, 4*60);
                                LogInfo("AckBuffer: %s\r\n", AckBuffer);

                                //bRet = ReadData(AckBuffer, 5,30, 8000);
                                //m_pTransmit->AddAction(ACTION_RECEIVE, AckBuffer, 5,30, 8000);
                                //bRet = m_pTransmit->ExecuteAction();
                                if (bRet && (!memcmp(AckBuffer, "OKAY", 4)))
                                {
                                    LogInfo("Image data send success!\r\n");
                                    bRet = TRUE;
                                    State = GotoNextState(State, TRUE);
                                }
                                else
                                {
                                    LogInfo("Image data send failed!\r\n");
                                    State = GotoNextState(State, FALSE);
                                    bRet = FALSE;
                                }
                            }
                        }
                        break;
                    }
                }
            }

            if (WriteBuffer != NULL)
            {
                free(WriteBuffer);
                WriteBuffer = NULL;
            }
        }
        break;
        case STAT_EXECUTE_CODE:
        {
            LogInfo("EXECUTE_CODE IN mbExecuteCode:%d, PartitionType:%s\r\n", CurrentConfig.mbExecuteCode, CurrentConfig.mCurrentFile.PartitionType);
            if ((TRUE == CurrentConfig.mbExecuteCode) && (memcmp(CurrentConfig.mCurrentFile.PartitionType, "zftl", 3) == 0))
            {
                LogInfo("start EXECUTE_CODE\r\n");
                memset(cmdbuf, 0, sizeof(cmdbuf));
                memcpy(cmdbuf, ("reboot"), strlen(("reboot")));
                bRet = SendData(cmdbuf, strlen((const char *)cmdbuf) + 1, 10, 1);
                if (bRet)
                {
                    memset(AckBuffer, 0, sizeof(AckBuffer));
                    bRet = ReadData(AckBuffer, 14, 10, 10);
                    LogInfo("AckBuffer: %s\r\n", AckBuffer);
                }
                //m_pTransmit->AddAction(ACTION_SEND, cmdbuf, strlen((const char*)cmdbuf), 20);
                // m_pTransmit->AddAction(ACTION_RECEIVE, AckBuffer, 15, 10);
                // bRet = m_pTransmit->ExecuteAction();
                if (bRet && !memcmp(AckBuffer, "OKAY REBOOT", 11))
                {
                    LogInfo("EXECUTE_CODE on doing\r\n");
                }
                else
                {
                    LogInfo("EXECUTE_CODE failed\r\n");
                    bRet = FALSE;
                }
            }
            else
            {
                bRet = TRUE;
            }
            State = GotoNextState(State, TRUE);
        }
        break;
        default:
            bExit = TRUE;
            break;
        }
    }

    if (pf != NULL)
    {
        fclose(pf);
        pf = NULL;
        LogInfo("fclose(pf) \r\n");
    }

    if (WriteBuffer != NULL) {
        free(WriteBuffer);
        WriteBuffer = NULL;
    }

    return bRet;
}

BOOL DownloadImage(stFileItem ImageInfo, BOOL bExecuteCode)
{
    BOOL bRet = FALSE;
    stDownloadConfig CurrentConfig;
    CurrentConfig.mCurrentFile = ImageInfo;

    switch (ImageInfo.nType)
    {
    case nand:
        LogInfo("case nand\r\n");

        m_pProtocolTable = m_DownloadImageTable;
        CurrentConfig.mPacketSize = PACKET_SIZE_FLASH;
        break;
    case fs:
        LogInfo("case fs\r\n");
        m_pProtocolTable = m_DownloadImageTable;
        CurrentConfig.mPacketSize = PACKET_SIZE_FLASH;
        break;
    case zftl:
        LogInfo("case zftl\r\n");
        ImageInfo.nType = zftl;
        m_pProtocolTable = m_DownloadImageTable;
        CurrentConfig.mPacketSize = PACKET_SIZE_FLASH;
        break;
    case ddr:
        LogInfo("case ddr\r\n");
        ImageInfo.nType = ddr;
        m_pProtocolTable = m_DownloadRAMImageTable;
        CurrentConfig.mPacketSize = PACKET_SIZE_IMAGE;
        CurrentConfig.mCurrentFile.PartitionName[0] = 'd';
        CurrentConfig.mCurrentFile.PartitionName[1] = 'd';
        CurrentConfig.mCurrentFile.PartitionName[2] = 'r';
        CurrentConfig.mCurrentFile.PartitionName[3] = '\0';
        break;
    case 6:
        LogInfo("case 6\r\n");
        ImageInfo.nType = 0x06;
        m_pProtocolTable = m_DownloadRAMImageTable;
        CurrentConfig.mPacketSize = PACKET_SIZE_IMAGE;
        CurrentConfig.mCurrentFile.PartitionName[0] = 'z';
        CurrentConfig.mCurrentFile.PartitionName[1] = 'l';
        CurrentConfig.mCurrentFile.PartitionName[2] = 'o';
        CurrentConfig.mCurrentFile.PartitionName[3] = 'a';
        CurrentConfig.mCurrentFile.PartitionName[4] = 'd';
        CurrentConfig.mCurrentFile.PartitionName[5] = 'e';
        CurrentConfig.mCurrentFile.PartitionName[6] = 'r';
        CurrentConfig.mCurrentFile.PartitionName[7] = '\0';
        break;
    default:
    {
        LogInfo("not right type\r\n");
        return TRUE;
    }
    break;
    }

    if (ImageInfo.bReadFromFile)
    {
        if (access(g_filename, 0) != 0)
        {
            LogInfo("software file path is error.\r\n");
            return FALSE;
        }
        pf = fopen(g_filename, "rb");
        if (pf == NULL)
        {
            LogInfo("Open file FAIL, g_filename: %s\r\n", g_filename);
        }
    }
    CurrentConfig.mStartState = STAT_SET_PACKET_SIZE;
    CurrentConfig.mbExecuteCode = bExecuteCode;
    bRet = DownloadOneImage(CurrentConfig);
    if (ImageInfo.bReadFromFile)
    {
        if (pf != NULL)
        {
            fclose(pf);
            pf = NULL;
            LogInfo("fclose(pf)\r\n");
        }
    }
    return bRet;
}

BOOL ExchangeImage()
{
    BOOL bRet = FALSE;
    BOOL bExecuteCode = FALSE;
    int fileIndex = 0;

    LogInfo("g_DownLoadFileNum:%d\r\n", g_DownLoadFileNum);
    if (g_DownLoadFileNum == 0)
    {
        LogInfo("g_DownLoadFileNum is error.\r\n");
        return bRet;
    }

    for (fileIndex = 1; fileIndex <= g_DownLoadFileNum; fileIndex++)
    {
        LogInfo("CYF start ExchangeImage:%d \r\n", fileIndex);

        LogInfo("FileItem[%d].bDownload:%d\r\n", fileIndex, FileItem[fileIndex].bDownload);
        LogInfo("FileItem[%d].strFileName:%s\r\n", fileIndex, FileItem[fileIndex].strFileName);
        LogInfo("FileItem[%d].bUseRelativePath:%d\r\n", fileIndex, FileItem[fileIndex].bUseRelativePath);
        LogInfo("FileItem[%d].bReadFromFile:%d\r\n", fileIndex, FileItem[fileIndex].bReadFromFile);
        LogInfo("FileItem[%d].bAutoDeleteImageCache:%d\r\n", fileIndex, FileItem[fileIndex].bAutoDeleteImageCache);
        LogInfo("FileItem[%d].pImageCache:%s\r\n", fileIndex, FileItem[fileIndex].pImageCache);
        LogInfo("FileItem[%d].dwOffset:%d\r\n", fileIndex, FileItem[fileIndex].dwOffset);
        LogInfo("FileItem[%d].PartitionName:%s\r\n", fileIndex, FileItem[fileIndex].PartitionName);
        LogInfo("FileItem[%d].dwLength:%d\r\n", fileIndex, FileItem[fileIndex].dwLength);
        LogInfo("FileItem[%d].nPartitionOffset:%d\r\n", fileIndex, FileItem[fileIndex].nPartitionOffset);
        LogInfo("FileItem[%d].PartitionType:%s\r\n", fileIndex, FileItem[fileIndex].PartitionType);
        LogInfo("FileItem[%d].nType:%d\r\n", fileIndex, FileItem[fileIndex].nType);
        LogInfo("bDownload FileItem[fileIndex].bDownload:%d\r\n", FileItem[fileIndex].bDownload);

        if (FileItem[fileIndex].bDownload)
        {
            if (fileIndex == g_DownLoadFileNum)
            {
                LogInfo("bExecuteCode == TRUE, fileIndex:%d\r\n", fileIndex);
                bExecuteCode = FALSE;
            }
            else
            {
                LogInfo("bExecuteCode == FALSE, fileIndex:%d\r\n", fileIndex);
            }
            LogInfo("bDownload true \r\n");
            bRet = DownloadImage(FileItem[fileIndex], bExecuteCode);
            usleep(10 * 1000);
        }
        else
        {
            LogInfo("bDownload false\r\n");
        }

        if (bRet)
        {
            LogInfo("fileIndex:%d  OK\r\n", fileIndex);
        }
        else
        {
            LogInfo("fileIndex:%d  FAIL\r\n", fileIndex);
            break;
        }
    }

    return bRet;
}

static BOOL DoDownload(enDownloadType DownloadType, BOOL bEableCRCCheck)
{
    BOOL bRet = FALSE;
    BOOL bReturn = FALSE;
    BOOL bExit = FALSE;
    enum STAT_ITEM {
        STAT_INIT,
        STAT_COLLECT_UPLOAD_NV_INFO,
        STAT_COLLECT_DOWNLOAD_IMAGE_INFO,
        STAT_EXCHANGE_IMAGE,
        STAT_START_AMT,
        STAT_UNINIT,
        STAT_EXIT
    };
    int nState = STAT_INIT;

    LogInfo("start\r\n");
    while (!bExit)
    {
        switch (nState)
        {
        case STAT_INIT:
            bRet = Initialize();
            if (bRet)
            {
                LogInfo("STAT_INIT OK\r\n");
                nState = STAT_COLLECT_UPLOAD_NV_INFO;
            }
            else
            {
                LogInfo("STAT_INIT FAIL\r\n");
                nState = STAT_EXIT;
            }
            break;

        case STAT_COLLECT_UPLOAD_NV_INFO:
            if (bEableCRCCheck)
            {
                nState = STAT_CRC_ON;
            }
            else if (m_bCRCCheck)
            {
                nState = STAT_CRC_OFF;
            }
            else
            {
                nState = STAT_COLLECT_DOWNLOAD_IMAGE_INFO;
            }
            break;

        case STAT_CRC_ON:
        {
            BYTE cmdbuf[255] = {0};
            BYTE AckBuffer[255] = {0};
            memset(cmdbuf, 0, sizeof(cmdbuf));
            memset(AckBuffer, 0, sizeof(AckBuffer));
            memcpy(&cmdbuf[0], "CRC ON", 6);
            bRet = SendData(cmdbuf, strlen((const char *)cmdbuf) + 1, 10, 1);
            if (bRet)
            {
                bRet = ReadData(AckBuffer, 5, 10, 30);
            }
            //m_pTransmit->AddAction(ACTION_SEND, cmdbuf,strlen((const char*)cmdbuf)+1, 20);
            //m_pTransmit->AddAction(ACTION_RECEIVE, AckBuffer, 5, 10, 400); //4:OKAY
            //bRet = m_pTransmit->ExecuteAction();
            if (bRet && (!memcmp(AckBuffer, "OKAY", 4)))
            {
                LogInfo("CRC open OK\r\n");
                m_bCRCCheck = TRUE;
                nState = STAT_COLLECT_DOWNLOAD_IMAGE_INFO;
            }
            else
            {
                LogInfo("CRC open FAIL..\r\n");
                m_bCRCCheck = FALSE;
                nState = STAT_COLLECT_DOWNLOAD_IMAGE_INFO;
            }
        }
        break;
        case STAT_CRC_OFF:
        {
            BYTE cmdbuf[255] = {0};
            BYTE AckBuffer[255] = {0};
            memset(cmdbuf, 0, sizeof(cmdbuf));
            memset(AckBuffer, 0, sizeof(AckBuffer));
            memcpy(&cmdbuf[0], "CRC OFF", 7);
            bRet = SendData(cmdbuf, strlen((const char *)cmdbuf) + 1, 10, 1);
            if (bRet)
            {
                bRet = ReadData(AckBuffer, 5, 10, 30);
            }

            //m_pTransmit->AddAction(ACTION_SEND, cmdbuf,strlen((const char*)cmdbuf)+1, 20);
            //m_pTransmit->AddAction(ACTION_RECEIVE, AckBuffer, 5, 10, 400); //4:OKAY
            //bRet = m_pTransmit->ExecuteAction();
            if (bRet && (!memcmp(AckBuffer, "OKAY", 4)))
            {
                LogInfo("CRC close OK\r\n");
                m_bCRCCheck = FALSE;
                nState = STAT_COLLECT_DOWNLOAD_IMAGE_INFO;
            }
            else
            {
                LogInfo("CRC close FAIL\r\n");
                m_bCRCCheck = TRUE;
                nState = STAT_COLLECT_DOWNLOAD_IMAGE_INFO;
            }
        }
        break;

        case STAT_COLLECT_DOWNLOAD_IMAGE_INFO:
            LogInfo("Collect image\r\n");
            CollectDownloadImage(DownloadType);
            nState = STAT_EXCHANGE_IMAGE;
            break;

        case STAT_EXCHANGE_IMAGE:

            LogInfo("ExchangeImage\r\n");
            bRet = ExchangeImage();
            if (bRet)
            {
                nState = STAT_START_AMT;
            }
            else
            {
                nState = STAT_UNINIT;
            }
            break;

        case STAT_START_AMT:
            bReturn = TRUE;
            nState = STAT_UNINIT;
            break;

        case STAT_UNINIT:
            bRet = UnInitialize();
            if (bRet)
            {
            }
            else
            {
                bReturn = FALSE;
            }
            nState = STAT_EXIT;
            break;

        default:
            bExit = TRUE;
        }
    }

    return bReturn;
}

BOOL UnPreInitBoot()
{
    return TRUE;
}

BOOL DownloadOneBoot(DWORD dwBootStartAddress, DWORD dwBootSize, DWORD dwPacketSize, DWORD dwOffset, BOOL SYNC)
{
    enSTAT State = STAT_SYNC;
    BOOL bRet = FALSE;
    BOOL bExit = FALSE;
    DWORD dwCurrentSize = 0;
    DWORD dwPacketCount = 0;

    if (access(g_filename, 0) != 0)
    {
        LogInfo("software file path is error.\r\n");
        return FALSE;
    }
    pf = fopen(g_filename, "rb");
    if (pf == NULL)
    {
        LogInfo("fopen %s failed.\r\n", g_filename);
        return FALSE;
    }
    if (SYNC == FALSE)
    {
        State = STAT_START_BYTE;
    }

    LogInfo("dwOffset:%d, dwPacketSize:%d\r\n", dwOffset, dwPacketSize);
    while (!bExit)
    {
        switch (State)
        {
        case STAT_SYNC:
        {
            BYTE nCommand = 0;
            BYTE nAck = 0;

            nCommand = CMD_SYNC_TBOOT;
            bRet = SendData(&nCommand, 1, 10, 20);
            if (bRet)
            {
                bRet = ReadData(&nAck, 1, 20, 20);
            }
            sleep(10);

            if (SYNC == FALSE)
            {
                LogInfo("not need STAT_SYNC 0X5A\r\n");
            }
            State = STAT_START_BYTE;
            if (bRet && (CMD_SYNC_BYTE_ACK_BOOTROM == nAck))
            {
                LogInfo("Send STAT_SYNC OK\r\n");
                State = STAT_START_BYTE;
            }
            else
            {
                LogInfo("Send STAT_SYNC FAIL\r\n");
                bExit = TRUE;
                bRet = FALSE;
            }
        }
        break;
        case STAT_START_BYTE:
        {
            State = STAT_NO_MODIFY_REG;
        }
        break;
        case STAT_NO_MODIFY_REG:
        {
            BYTE nCommand = CMD_SEND_DATA_BOOTROM;

            bRet = SendData(&nCommand, 1, 10, 1);
            if (bRet)
            {
                LogInfo("Send CMD_SEND_DATA_BOOTROM OK\r\n");
                State = STAT_ADDRESS;
            }
            else
            {
                LogInfo("Send CMD_SEND_DATA_BOOTROM FAIL\r\n");
                State = STAT_TERMINATE;
            }
        }
        break;
        case STAT_ADDRESS:
        {
            DWORD dwStartAddress = ExchangeEndian(dwBootStartAddress);

            bRet = SendData((BYTE *)&dwStartAddress, sizeof(DWORD), 10, 1);
            if (bRet)
            {
                LogInfo("Send dwStartAddress OK\r\n");
                State = STAT_DATA_LEN;
            }
            else
            {
                LogInfo("Send dwStartAddress FAIL\r\n");
                State = STAT_TERMINATE;
            }
        }
        break;
        case STAT_DATA_LEN:
        {
            DWORD dwSize = ExchangeEndian(dwBootSize);
            BYTE nAck = 0;

            bRet = SendData((BYTE *)&dwSize, sizeof(DWORD), 20, 1);
            if (bRet)
            {
                bRet = ReadData(&nAck, 1, 20, 20);
                if (bRet && (CMD_SEND_ADDRESS_LEN_ACK_BOOTROM == nAck))
                {
                    LogInfo("Set address and length OK\r\n");
                    if (NULL == pf)
                    {
                        LogInfo("pf not exsist\r\n");
                        State = STAT_TERMINATE;
                    }
                    else
                    {
                        fseek(pf, dwOffset, 0);
                        dwCurrentSize = dwBootSize;
                        LogInfo("dwOffset:%d,  dwBootSize:%d\r\n", dwOffset, dwBootSize);
                        State = STAT_DATAPACKET;
                        LogInfo("start send BootFile\r\n");
                    }
                }
                else
                {
                    LogInfo("Get ADDRESS_LEN_ACK FAIL!bRet=%d,nAck=%d\r\n", bRet, nAck);
                    State = STAT_TERMINATE;
                }
            }
            else
            {
                LogInfo("Send dwBootSize FAIL\r\n");
                State = STAT_TERMINATE;
            }
        }
        break;
        case STAT_DATAPACKET:
        {
            BYTE WriteBuffer[PACKET_SIZE_BOOT] = {0};
            BYTE nAck = 0;
            DWORD dwSendSize = 0;

            if (dwPacketSize > PACKET_SIZE_BOOT)
            {
                LogInfo("dwPacketSize>PACKET_SIZE_BOOT too big\r\n");
                bRet = FALSE;
                State = STAT_TERMINATE;
            }
            else
            {
                LogInfo("dwPacketSize:%d   PACKET_SIZE_BOOT:%d\r\n", dwPacketSize, PACKET_SIZE_BOOT);
            }

            memset(WriteBuffer, 0, sizeof(WriteBuffer));
            if (dwCurrentSize >= dwPacketSize)
            {
                LogInfo("dwCurrentSize >= dwPacketSize\r\n");
                if (fread(&WriteBuffer[0], 1, dwPacketSize, pf) != dwPacketSize)
                {
                    LogInfo("Read bootfile FAIL\r\n");
                    bRet = FALSE;
                    State = STAT_TERMINATE;
                    break;
                }
                LogInfo("dwPacketSize:%d\r\n", dwPacketSize);
                bRet = SendData(WriteBuffer, dwPacketSize, 10, 1);
                LogInfo("dwPacketCount:%d, bRet:%d\r\n", dwPacketCount, bRet);
                if (bRet)
                {
                    LogInfo("send bootfile one packet OK\r\n");
                    dwPacketCount++;
                    dwCurrentSize -= dwPacketSize;
                    if (0x00 == dwCurrentSize)
                    {
                        bRet = ReadData(&nAck, 1, 20, 20);
                        if (bRet && (CMD_SEND_DATA_ACK_BOOTROM == nAck))
                        {
                            State = STAT_EXECUTE_CODE;
                        }
                        else
                        {
                            LogInfo("Get send bootfile ACK FAIL!\r\n");
                            bRet = FALSE;
                            State = STAT_TERMINATE;
                        }
                    }
                }
                else
                {
                    LogInfo("send bootfile one packet FAIL\r\n");
                    bRet = FALSE;
                    State = STAT_TERMINATE;
                }
            }
            else if (dwCurrentSize > 0)
            {
                if ((dwCurrentSize % 512) == 0)
                {
                    dwSendSize = dwCurrentSize - 8;
                    LogInfo("STAT_DATAPACKET, split packet\r\n");
                }
                else
                {
                    dwSendSize = dwCurrentSize;
                    usleep(2 * 1000);
                }

                LogInfo("dwCurrentSize > 0 but <packetsize\r\n");
                if (fread(&WriteBuffer[0], 1, dwSendSize, pf) != dwSendSize)
                {
                    LogInfo("Read bootfile FAIL\r\n");
                    bRet = FALSE;
                    State = STAT_TERMINATE;
                    break;
                }

                bRet = SendData(WriteBuffer, dwSendSize, 10, 1);
                LogInfo("dwPacketCount:%d,dwSendSize:%d, dwCurrentSize:%d\r\n", dwPacketCount, dwSendSize, dwCurrentSize);
                //m_pTransmit->AddAction(ACTION_SEND, WriteBuffer, dwCurrentSize, 10);
                //bRet = m_pTransmit->ExecuteAction();
                if (bRet)
                {
                    dwPacketCount++;
                    dwCurrentSize -= dwSendSize;
                    if (dwCurrentSize == 0)
                    {
                        nAck = 0;
                        bRet = ReadData(&nAck, 1, 20, 20);
                        LogInfo("nAck = %02x,bRet:%d\r\n", nAck, bRet);
                        if (bRet && (CMD_SEND_DATA_ACK_BOOTROM == nAck))
                        {
                            State = STAT_EXECUTE_CODE;
                            LogInfo("send bootfile  last packet OK\r\n");
                        }
                        else
                        {
                            LogInfo("Get send bootfile ACK FAIL!\r\n");
                            bRet = FALSE;
                            State = STAT_TERMINATE;
                        }
                    }
                }
                else
                {
                    LogInfo("send bootfile one packet FAIL\r\n");
                    bRet = FALSE;
                    State = STAT_TERMINATE;
                }
            }
            else
            {
                bRet = TRUE;
                State = STAT_EXECUTE_CODE;
            }
        }
        break;
        case STAT_EXECUTE_CODE:
        {
            unsigned char buf[5];
            BYTE nAck = 0;
            DWORD dwStartAddress = ExchangeEndian(dwBootStartAddress);
            buf[0] = CMD_STARTUP_BOOTROM;

            memcpy(&buf[1], &dwStartAddress, sizeof(DWORD));
            bRet = SendData(buf, sizeof(buf), 10, 1);
            //m_pTransmit->AddAction(ACTION_SEND, buf, sizeof(buf));
            //bRet = m_pTransmit->ExecuteAction();
            if (bRet)
            {
                bRet = ReadData(&nAck, 1, 20, 20);
                if (bRet && (CMD_STARTUP_ACK_BOOTROM == nAck))
                {
                    LogInfo("start boot OK\r\n");
                    State = STAT_TERMINATE;
                    bRet = TRUE;
                }
                else
                {
                    LogInfo("Get start boot ACK FAIL\r\n");
                    State = STAT_TERMINATE;
                    bRet = FALSE;
                }
            }
            else
            {
                LogInfo("send boot start command FAIL\r\n");
                State = STAT_TERMINATE;
                bRet = FALSE;
            }
        }
        break;
        default:
            bExit = TRUE;
            break;
        }
    }

    if (pf != NULL)
    {
        fclose(pf);
        pf = NULL;
    }
    return bRet;
}

BOOL PreInitBoot()
{
    unsigned char header[64] = {0};
    stBinMasterInfo MasterHead;
    int num = 0;

    m_dwBootStage1StartAddress = 0x00082000;

#if 0
    if(s_downmode == 0)
        m_dwBootStage2StartAddress = 0x27ef0000;
    else
        m_dwBootStage2StartAddress = 0x23df0000;
#endif

    LogInfo("g_filename: %s\r\n", g_filename);

    if (access(g_filename, 0) != 0)
    {
        LogInfo("software file path is error.\r\n");
        return FALSE;
    }

    pf = fopen(g_filename, "rb");
    if (pf == NULL)
    {
        LogInfo("Open file FAIL, g_filename: %s\r\n", g_filename);
        return FALSE;
    }

    num = fread((BYTE *)&MasterHead, 1, sizeof(MasterHead), pf);
    LogInfo("fread num:%d, sizeof(MasterHead):%ld\r\n", num, sizeof(MasterHead));
    DumpHex((char *)&MasterHead, sizeof(MasterHead));
    if (num == 0)
    {
        LogInfo("Read file FAIL, g_filename: %s\r\n", g_filename);
        return FALSE;
    }

    //shijiaxing 2024-0926 modify for MTC0356-1582
#if __BYTE_ORDER != __LITTLE_ENDIAN  //from little to big-endian
    DWORD change = 0;
    change = MasterHead.nTotalFileNum;
    MasterHead.nTotalFileNum = BSWAP_32(change);
    change = MasterHead.nFlashType;
    MasterHead.nFlashType = BSWAP_32(change);
    change = MasterHead.nNVCoalition;
    MasterHead.nNVCoalition = BSWAP_32(change);
    change = MasterHead.iFileSize;
    MasterHead.iFileSize = BSWAP_32(change);
    change = MasterHead.iCkeckSum;
    MasterHead.iCkeckSum = BSWAP_32(change);
    change = MasterHead.iImageStructOffset;
    MasterHead.iImageStructOffset = BSWAP_32(change);
    change = MasterHead.tloaderOffset;
    MasterHead.tloaderOffset = BSWAP_32(change);
    change = MasterHead.tloaderLength;
    MasterHead.tloaderLength = BSWAP_32(change);
    change = MasterHead.tBootOffset;
    MasterHead.tBootOffset = BSWAP_32(change);
    change = MasterHead.tBootLength;
    MasterHead.tBootLength = BSWAP_32(change);
    change = MasterHead.PartitionOffset;
    MasterHead.PartitionOffset = BSWAP_32(change);
    change = MasterHead.PartitionLength;
    MasterHead.PartitionLength = BSWAP_32(change);
#endif

    m_iFileNum = MasterHead.nTotalFileNum;
    m_stMasterInfo.nTotalFileNum = MasterHead.nTotalFileNum;

    m_stMasterInfo.nFlashType = MasterHead.nFlashType;
    m_stMasterInfo.nNVCoalition = MasterHead.nNVCoalition;
    memcpy(m_stMasterInfo.chVersionIN, MasterHead.chVersionIN, sizeof(MasterHead.chVersionIN));
    memcpy(m_stMasterInfo.chVersionOUT, MasterHead.chVersionOUT, sizeof(MasterHead.chVersionOUT));
    m_stMasterInfo.iFileSize = MasterHead.iFileSize;
    m_stMasterInfo.iImageStructOffset = MasterHead.iImageStructOffset;
    m_stMasterInfo.PartitionLength = MasterHead.PartitionLength;
    m_stMasterInfo.PartitionOffset = MasterHead.PartitionOffset;

    m_stMasterInfo.tloaderLength = MasterHead.tloaderLength;
    m_stMasterInfo.tloaderOffset = MasterHead.tloaderOffset;
    m_stMasterInfo.tBootLength = MasterHead.tBootLength - 64;
    m_stMasterInfo.tBootOffset = MasterHead.tBootOffset + 64;

    m_dwBootStage2Size = m_stMasterInfo.tBootLength;
    m_dwBootStage1Size = m_stMasterInfo.tloaderLength;
    m_dwPartitionSize = m_stMasterInfo.PartitionLength;

    LogInfo("m_iFileNum:%d\r\n", m_iFileNum);
    LogInfo("m_stMasterInfo.nTotalFileNum:%d\r\n", m_stMasterInfo.nTotalFileNum);
    LogInfo("m_stMasterInfo.nFlashType:%d\r\n", m_stMasterInfo.nFlashType);
    LogInfo("m_stMasterInfo.nNVCoalition:%d\r\n", m_stMasterInfo.nNVCoalition);
    LogInfo("m_stMasterInfo.chVersionIN:%s\r\n", m_stMasterInfo.chVersionIN);
    LogInfo("m_stMasterInfo.chVersionOUT:%s\r\n", m_stMasterInfo.chVersionOUT);
    LogInfo("m_stMasterInfo.iFileSize:%d\r\n", m_stMasterInfo.iFileSize);
    LogInfo("m_stMasterInfo.iImageStructOffset:%d\r\n", m_stMasterInfo.iImageStructOffset);
    LogInfo("m_stMasterInfo.PartitionLength:%d\r\n", m_stMasterInfo.PartitionLength);
    LogInfo("m_stMasterInfo.PartitionOffset:%d\r\n", m_stMasterInfo.PartitionOffset);
    LogInfo("m_stMasterInfo.tloaderLength:%d\r\n", m_stMasterInfo.tloaderLength);
    LogInfo("m_stMasterInfo.tloaderOffset:%d\r\n", m_stMasterInfo.tloaderOffset);
    LogInfo("m_stMasterInfo.tBootLength:%d\r\n", m_stMasterInfo.tBootLength);
    LogInfo("m_stMasterInfo.tBootOffset:%d\r\n", m_stMasterInfo.tBootOffset);
    LogInfo("m_dwBootStage2Size:%d\r\n", m_dwBootStage2Size);
    LogInfo("m_dwBootStage1Size:%d\r\n", m_dwBootStage1Size);
    LogInfo("m_dwPartitionSize:%d\r\n", m_dwPartitionSize);

    //start: Lucien add for automatic get m_dwBootStage2StartAddress
    fseek(pf,MasterHead.tBootOffset,0);
    if(fread(header,1,sizeof(header),pf)!= sizeof(header)) {
        LogInfo("read tboot failed %s\r\n", g_filename);
        return FALSE;
    }
    /**
     * U-Boot Image header use big-Endian, after below process, for big-Endian CPUs, could be big-Endian represent,
     * for little-Endian CPUs, could be little-Endian representation.
     * when used in DownloadOneBoot(), exchanged to big-Endian, then SendData()
     */
    m_dwBootStage2StartAddress = (header[20]<<24 | header[21]<<16 | header[22]<<8 | header[23]);
    LogInfo("m_dwBootStage2StartAddress: 0x%x\r\n", m_dwBootStage2StartAddress);
    //end: Lucien add for automatic get m_dwBootStage2StartAddress

    if (pf != NULL)
    {
        fclose(pf);
        pf = NULL;
    }
    return TRUE;
}

static BOOL DownloadPartition(DWORD dwPartitionSize, DWORD dwOffset)
{
    enParpSTAT State = STAT_SYNC_PARTIITON;
    BOOL bRet = FALSE;
    BOOL bExit = FALSE;
    BYTE AckBuffer[255] = {0};
    BYTE *WriteBuffer = NULL;

    if (access(g_filename, 0) != 0)
    {
        LogInfo("software file path is error.\r\n");
        return FALSE;
    }
    pf = fopen(g_filename, "rb");
    if (pf == NULL)
    {
        LogInfo("Open file FAIL,g_filename: %s\r\n", g_filename);
        return FALSE;
    }

    while (!bExit)
    {
        switch (State)
        {
        case STAT_SYNC_PARTIITON:
        {
            BYTE nCommand = 0;
            BYTE nAck = 0;

            LogInfo("STAT_SYNC_PARTIITON\r\n");
            nCommand = CMD_SYNC_TBOOT;
            bRet = SendData(&nCommand, 1, 10, 1);
            if (bRet)
            {
                bRet = ReadData(&nAck, 1, 10, 10);
            }

            //m_pTransmit->AddAction(ACTION_SEND, &nCommand, 1, 10);
            //m_pTransmit->AddAction(ACTION_RECEIVE, &nAck, 1, 10,10);
            //bRet = m_pTransmit->ExecuteAction();
            if (bRet && (CMD_SYNC_ACK_TBOOT == nAck))
            {
                LogInfo("STAT_SYNC_PARTIITON OK\r\n");
                State = STAT_SET_PARTITION_CMD;
            }
            else
            {
                LogInfo("STAT_SYNC_PARTIITON FAIL\r\n");
                State = STAT_SYNC_PARTIITON;
                int count = 0;
                count++;
                if (count > 20)
                {
                    State = STAT_EXIT;
                    bExit = TRUE;
                    bRet = FALSE;
                }
            }
            usleep(10 * 1000);
        }
        break;
        case STAT_SET_PARTITION_CMD:
        {
            BYTE CommandHead[] = "set partitions 1000\0";
            BYTE CommandBuf[200] = {0};

            memset(AckBuffer, 0, sizeof(AckBuffer));
            memcpy(CommandBuf, CommandHead, sizeof(CommandHead));

            LogInfo("STAT_SET_PARTITION_CMD strlen((char*)CommandBuf)):%ld\r\n", strlen((char *)CommandBuf));
            bRet = SendData(CommandBuf, strlen((char *)CommandBuf) + 1, 10, 1);
            if (bRet)
            {
                // AckBuffer = (BYTE *)ReadDataExtraFuncB(17,10,20);
                bRet = ReadData(AckBuffer, 17, 10, 20);
            }
            LogInfo("STAT_SET_PARTITION_CMD AckBuffer:%s\r\n", AckBuffer);

            //m_pTransmit->AddAction(ACTION_SEND,CommandBuf,strlen((char*)CommandBuf)+1,10,1);
            //m_pTransmit->AddAction(ACTION_RECEIVE,AckBuffer,17,10,20);
            //bRet = m_pTransmit->ExecuteAction();
            //if (bRet&&(AckBuffer[0] == 'O'))

            if (bRet && (!memcmp(AckBuffer, "OKAY RECV_TABLES", 16)))
            {
                LogInfo("STAT_SET_PARTITION_CMD set partitions OK\r\n");
                State = STAT_SEND_PARTIITON;
            }
            else
            {
                LogInfo("STAT_SET_PARTITION_CMD set partitions FAIL\r\n");
                State = STAT_EXIT;

                bExit = TRUE;
                bRet = FALSE;
            }
        }
        break;
        case STAT_SEND_PARTIITON:
        {
            char *strfail1 = "FAIL INVALID_PARTITION_TABLE";
            char *strfail2 = "FAIL ACCEPTABLE_PARTITION_CHANGE";
            char *strfail3 = "FAIL UNACCEPTABLE_PARTITION_CHANGE";

            if (WriteBuffer != NULL) {
                free(WriteBuffer);
                WriteBuffer = NULL;
            }
            WriteBuffer = (BYTE *)malloc(dwPartitionSize);
            if (WriteBuffer == NULL) {
                LogInfo("allocate WriteBuffer failed.\r\n");
                bExit = TRUE;
                bRet = FALSE;
                break;
            }

            fseek(pf, dwOffset, 0);
            if (fread(WriteBuffer, 1, dwPartitionSize, pf) != dwPartitionSize)
            //fread(WriteBuffer, dwPartitionSize) != dwPartitionSize)
            {
                LogInfo("STAT_SEND_PARTIITON read partition data FAIL\r\n");
                bRet = FALSE;
                break;
            }

            int dlPTindex = 0;
            int dlTime = dwPartitionSize / 1000;

            LogInfo("STAT_SEND_PARTIITON start !\r\n");
            for (dlPTindex = 0; dlPTindex < dlTime; dlPTindex++)
            {
                bRet = SendData(WriteBuffer + 1000 * dlPTindex, 1000, 10, 1);
                LogInfo("STAT_SEND_PARTIITON dlPTindex:%d\r\n", dlPTindex);
            }
            if (dwPartitionSize % 1000 != 0)
                bRet = SendData(WriteBuffer + 1000 * dlTime, dwPartitionSize % 1000, 10, 1);

            LogInfo("STAT_SEND_PARTIITON SendData\r\n");
            if (bRet)
            {
                usleep(10 * 1000);
                memset(AckBuffer, 0, sizeof(AckBuffer));
                bRet = ReadData(AckBuffer, strlen(strfail3) + 1, 10, 5);
            }
            LogInfo("STAT_SEND_PARTIITON ReadDataExtraFuncB\r\n");
            //m_pTransmit->AddAction(ACTION_SEND, WriteBuffer, dwPartitionSize, 10,1);
            //m_pTransmit->AddAction(ACTION_RECEIVE,AckBuffer,strfail3.GetLength()+1,50,20);
            //bRet = m_pTransmit->ExecuteAction();

            // char buf[100];
            // memset(buf, 0, 100);
            // strcpy(buf, (char *)AckBuffer);

            if (AckBuffer[0] == 0)
            {
                LogInfo("STAT_SEND_PARTIITON read error !\r\n");
                bExit = TRUE;
                bRet = FALSE;
                State = STAT_EXIT;
                break;
            }

            if (!memcmp(AckBuffer, "OKAY", 4))
            {
                LogInfo("STAT_SEND_PARTIITON send partition OK\r\n");
                State = STAT_EXIT;
                bRet = TRUE;
                break;
            }

            if (!memcmp(AckBuffer, strfail1, strlen(strfail1)))
            {
                LogInfo("STAT_SEND_PARTIITON  partition has some problem..\r\n");
                bExit = TRUE;
                bRet = FALSE;
                State = STAT_EXIT;
                break;
            }

            if (!memcmp(AckBuffer, strfail2, strlen(strfail2)))
            {
                BYTE WriteBufferErase[200] = "erase auto";

                //memset(AckBuffer,0,150);
                LogInfo("FAIL ACCEPTABLE_PARTITION_CHANGE\r\n");
                bRet = SendData(WriteBufferErase, 11, 10, 1);
                if (bRet)
                {
                    bRet = ReadData(AckBuffer, 5, 20, 4*60);
                }

                //m_pTransmit->AddAction(ACTION_SEND, WriteBufferErase, 11, 10,1);
                //m_pTransmit->AddAction(ACTION_RECEIVE,AckBuffer,5,20,500);
                //bRet = m_pTransmit->ExecuteAction();
                // memset(buf, 0, 100);
                // strcpy(buf, (char *)AckBuffer);
                if (bRet && (!memcmp(AckBuffer, "OKAY", 5)))
                {
                    if (WriteBuffer != NULL)
                    {
                        free(WriteBuffer);
                        WriteBuffer = NULL;
                    }

                    LogInfo("Erase Partition OK\r\n");
                    usleep(100 * 1000);
                    bExit = TRUE;

                    bRet = TRUE;
                    State = STAT_EXIT;
                    break;
                }
                else
                {
                    if (WriteBuffer != NULL)
                    {
                        free(WriteBuffer);
                        WriteBuffer = NULL;
                    }
                    LogInfo("Erase Partition FAIL\r\n");
                    bExit = TRUE;
                    bRet = FALSE;
                    State = STAT_EXIT;
                    break;
                }
            }

            if (!memcmp(AckBuffer, strfail3, strlen(strfail3)))
            {
                LogInfo("Partition change, stop download\r\n");
                State = STAT_EXIT;
                bExit = TRUE;
                bRet = FALSE;
                State = STAT_EXIT;
                break;
            }
            else
            {
                LogInfo("Partition other failed, stop download\r\n");
                State = STAT_EXIT;
                bExit = TRUE;
                bRet = FALSE;
                State = STAT_EXIT;
                break;
            }
        }
        break;
        default:
            bExit = TRUE;
        }
    }
    if (pf != NULL)
    {
        fclose(pf);
        pf = NULL;
    }

    if (WriteBuffer != NULL) {
        free(WriteBuffer);
        WriteBuffer = NULL;
    }

    return bRet;
}

static BOOL EraseNVRW()
{
    BOOL bRet = FALSE;
    char AckBuffer[255] = {0};
    BYTE WriteBufferErase[200] = "erase nvrw";

    LogInfo("start\r\n");
    bRet = SendData(WriteBufferErase, 11, 10, 1);
    usleep(1000 * 1000);
    if (bRet)
    {
        bRet = ReadData((BYTE *)AckBuffer, 4, 20, 30);
    }

    if (bRet && (!memcmp(AckBuffer, "OKAY", 5)))
    {
        LogInfo("Erase OK\r\n");
        usleep(100 * 1000);
        return TRUE;
    }
    LogInfo("Erase FAIL\r\n");

    return FALSE;
}

//download tloader, tboot, partition
BOOL DoDownloadBootForDL(char *softwarepath)
{
    double duration = 0;
    clock_t start;
    clock_t finish;
    BOOL bRet = FALSE;
    BOOL bTotalRet = TRUE;
    BOOL bExit = FALSE;
    enum {
        STAT_INIT,
        STAT_DOWNLOAD_BOOT1,
        STAT_DOWNLOAD_BOOT2,
        STAT_DOWNLOAD_PARTITION,
        STAT_UNINIT,
        STAT_EXIT
    };
    int nState = STAT_INIT;
    int nWhile = 0;

    LogInfo("start\r\n");
    strcpy(g_filename, (char *)softwarepath);

    while (!bExit)
    {
        LogInfo("nWhile:%d\r\n", nWhile);
        nWhile++;

        switch (nState)
        {
        case STAT_INIT:
            bRet = PreInitBoot();
            if (bRet)
            {
                LogInfo("PreInitBoot OK\r\n");
                nState = STAT_DOWNLOAD_BOOT1;
            }
            else
            {
                LogInfo("PreInitBoot FAIL\r\n");
                bTotalRet = FALSE;
                nState = STAT_EXIT;
            }
            break;
            /*********************add zxw 20121009 for 296301 begin************************************/
        case STAT_DOWNLOAD_BOOT1:
            start = clock();
            LogInfo("start download BOOT1\r\n");
            bRet = DownloadOneBoot(m_dwBootStage1StartAddress, m_dwBootStage1Size, 4000, m_stMasterInfo.tloaderOffset, TRUE);

            finish = clock();
            duration = (double)(finish - start) / CLOCKS_PER_SEC;
            LogInfo("download BOOT1 used time:%f\r\n", duration);
            if (bRet)
            {
                LogInfo("download BOOT1 OK\r\n");
                nState = STAT_DOWNLOAD_BOOT2;
            }
            else
            {
                LogInfo("download BOOT1 FAIL\r\n");
                bTotalRet = FALSE;
                nState = STAT_UNINIT;
            }
            break;
        case STAT_DOWNLOAD_BOOT2:
            start = clock();
            LogInfo("start download BOOT2\r\n");

            bRet = DownloadOneBoot(m_dwBootStage2StartAddress, m_dwBootStage2Size, 4000, m_stMasterInfo.tBootOffset, TRUE);
            finish = clock();
            duration = (double)(finish - start) / CLOCKS_PER_SEC;
            LogInfo("download BOOT2 used time:%f\r\n", duration);

            if (bRet)
            {
                LogInfo("download BOOT2 OK\r\n");
                nState = STAT_DOWNLOAD_PARTITION;
            }
            else
            {
                LogInfo("download BOOT2 FAIL\r\n");
                nState = STAT_UNINIT;
                bTotalRet = FALSE;
            }
            break;
            /*********************add zxw 20121009 for 296301 end************************************/
        case STAT_DOWNLOAD_PARTITION:
            LogInfo("STAT_DOWNLOAD_PARTITION\r\n");
            bRet = DownloadPartition(m_dwPartitionSize, m_stMasterInfo.PartitionOffset);

            if (bRet)
            {
                LogInfo("DownloadPartition : download OK\r\n");
            }
            else
            {
                LogInfo("DownloadPartition : download FAIL\r\n");
                bTotalRet = FALSE;
            }
            nState = STAT_UNINIT;
            break;
        case STAT_UNINIT:
            bRet = UnPreInitBoot();
            if (bRet)
            {
                LogInfo("UnPreInitBoot OK\r\n");
            }
            else
            {
                LogInfo("UnPreInitBoot FAIL\r\n");
                bTotalRet = FALSE;
            }
            nState = STAT_EXIT;
            break;

        default:
            bExit = TRUE;
            break;
        }
    }

    return bTotalRet;
}

void DownLoad_LogInfo(const char *filename, const char *func_name, const int line, const char *fmt, ...)
{
    char tmp[2048] = {0};
    va_list arglist;

    va_start(arglist, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, arglist);
    va_end(arglist);

    if (filename) {}

    printf("[%s:%d]:%s\n", func_name, line, tmp);
}

