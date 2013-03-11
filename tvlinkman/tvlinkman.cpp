// 包含头文件
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "tinyxml.h"

static void GetAppBasePath(char *path)
{
    int i;
    GetModuleFileNameA(NULL, path, MAX_PATH);
    i = (int)strlen(path);
    while (--i) if (path[i] == '\\') { path[++i] = '\0'; break; }
}

#define CHECK_WAIT_TIMEOUT 20000
#define READ_BUF_SIZE      4096
static BOOL check_video_url(const char *url)
{
    // create pipe
    DWORD  retv   = 0;
    BOOL   bIsOK  = FALSE;
    HANDLE hPipeR = NULL;
    HANDLE hPipeW = NULL;
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength        = sizeof(sa);
    sa.bInheritHandle = TRUE;
    retv = CreatePipe(&hPipeR, &hPipeW, &sa, 0);
    if (!retv) goto done;

    // ffprobe path & cmdline
    char ffprobe[MAX_PATH] = {0};
    char cmdline[MAX_PATH] = {0};
    // ffprobe exe path
    GetAppBasePath(ffprobe);
    strcat(ffprobe, "ffmpeg\\ffprobe.exe");
    // ffprobe cmd line
    strcpy(cmdline, " -loglevel info ");
    strcat(cmdline, url);

    // create process
    PROCESS_INFORMATION pinfo = {0};
    STARTUPINFOA        sinfo = {0};
    sinfo.cb         = sizeof(sinfo);
    sinfo.dwFlags    = STARTF_USESTDHANDLES;
    sinfo.hStdOutput = hPipeW;
    sinfo.hStdError  = hPipeW;
    retv = CreateProcessA(ffprobe, cmdline, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &sinfo, &pinfo);
    if (!retv) goto done;

    // wait for process
    retv = WaitForSingleObject(pinfo.hProcess, CHECK_WAIT_TIMEOUT);
    if (retv == WAIT_TIMEOUT)
    {
        TerminateProcess(pinfo.hProcess, 0);
        goto done;
    }

    // read ffprobe output
    char  szReadBuf[READ_BUF_SIZE] = {0};
    DWORD dwReadNum = 0;
    retv = ReadFile(hPipeR, szReadBuf, READ_BUF_SIZE, &dwReadNum, NULL);
    if (retv && !strstr(szReadBuf, "No such file or directory")
        && !strstr(szReadBuf, "Invalid data found when processing input")
        && !strstr(szReadBuf, "Input/output error"))
    {
        bIsOK = TRUE;
    }

done:
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);
    CloseHandle(hPipeR);
    CloseHandle(hPipeW);
    return bIsOK;
}

int main(int argc, char *argv[])
{
#if 1
    bool retv = false;
    char strXmlIn  [MAX_PATH];
    char strXmlOut0[MAX_PATH];
    char strXmlOut1[MAX_PATH];
    GetAppBasePath(strXmlIn  );
    GetAppBasePath(strXmlOut0);
    GetAppBasePath(strXmlOut1);
    strcat(strXmlIn  , "androidtv.xml");
    strcat(strXmlOut0, "androidtv-ok.xml");
    strcat(strXmlOut1, "androidtv-ng.xml");

    TiXmlDocument *pDoc = new TiXmlDocument();
    if (NULL == pDoc) return false;
    retv = pDoc->LoadFile(strXmlIn);
    if (!retv) return false;

    HANDLE hConsole = NULL;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    TiXmlElement   *pEle0 = pDoc->RootElement();
    TiXmlElement   *pEle1 = NULL;
    TiXmlElement   *pEle2 = NULL;
    TiXmlElement   *pEle3 = NULL;
    TiXmlElement   *pEle4 = NULL;
    TiXmlAttribute *pAttr = NULL;
    const char *strName   = NULL;
    const char *strValue  = NULL;
    if (pEle0)
    {
        strName = pEle0->Value();
        if (strcmp(strName, "response") == 0)
        {
            for (pEle1=pEle0->FirstChildElement(); pEle1; pEle1=pEle1->NextSiblingElement())
            {
                strName = pEle1->Value();
                if (strcmp(strName, "attributes") == 0)
                {
                    for (pEle2=pEle1->FirstChildElement(); pEle2; pEle2=pEle2->NextSiblingElement())
                    {
                        strName = pEle2->Value();
                        if (strcmp(strName, "liveType") == 0)
                        {
                            for (pEle3=pEle2->FirstChildElement(); pEle3; pEle3=pEle3->NextSiblingElement())
                            {
                                strName = pEle3->Value();
                                if (strcmp(strName, "channel") == 0)
                                {
                                    for (pEle4=pEle3->FirstChildElement(); pEle4; pEle4=pEle4->NextSiblingElement())
                                    {
                                        strName = pEle4->Value();
                                        if (strcmp(strName, "addressInfo") == 0)
                                        {
                                            for (pAttr=pEle4->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                                            {
                                                strName  = pAttr->Name();
                                                strValue = pAttr->Value();
                                                if (strcmp(strName, "url") == 0)
                                                {
                                                    if (check_video_url(strValue))
                                                    {
                                                        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN|FOREGROUND_INTENSITY);
                                                        printf("[OK    ] ");
                                                        SetConsoleTextAttribute(hConsole, 0x7);
                                                    }
                                                    else
                                                    {
                                                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED|FOREGROUND_INTENSITY);
                                                        printf("[FAILED] ");
                                                        SetConsoleTextAttribute(hConsole, 0x7);
                                                    }
                                                    printf("%s\n", strValue);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#endif

    printf("\ndone!\n");
    getch();
    return 0;
}
