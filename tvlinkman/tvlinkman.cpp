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
    PROCESS_INFORMATION pinfo = {0};
    STARTUPINFOA        sinfo = {0};
    SECURITY_ATTRIBUTES sa    = {0};
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
    bool retv = false;
    char strXmlIn  [MAX_PATH];
    char strXmlOut0[MAX_PATH];
    char strXmlOut1[MAX_PATH];
    FILE *fp = NULL;

    GetAppBasePath(strXmlIn  );
    GetAppBasePath(strXmlOut0);
    GetAppBasePath(strXmlOut1);
    strcat(strXmlIn  , "androidtv.xml");
    strcat(strXmlOut0, "androidtv-ok.xml");
    strcat(strXmlOut1, "androidtv-ng.ini");
    fp = fopen(strXmlOut1, "w");

    TiXmlDocument *pDoc0 = new TiXmlDocument();
    if (!pDoc0) return false;
    retv = pDoc0->LoadFile(strXmlIn);
    if (!retv) return false;

    HANDLE hConsole = NULL;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    TiXmlElement   *pEle0 = pDoc0->RootElement();
    TiXmlElement   *pEle1 = NULL;
    TiXmlElement   *pEle2 = NULL;
    TiXmlElement   *pEle3 = NULL;
    TiXmlElement   *pEleN = NULL;
    TiXmlAttribute *pAttr = NULL;
    BOOL            bIsOK = FALSE;
    int             nType = 0;
    char            livetype[MAX_PATH];
    char            channame[MAX_PATH];

    if (!pEle0 || strcmp(pEle0->Value(), "response")) return false;
    for (pEle1=pEle0->FirstChildElement(); pEle1; pEle1=pEle1->NextSiblingElement())
    {
        if (strcmp(pEle1->Value(), "liveType")) continue;
        for (pAttr=pEle1->FirstAttribute(); pAttr; pAttr=pAttr->Next())
        {
            if (strcmp(pAttr->Name(), "name")) continue;
            strcpy(livetype, pAttr->Value());
        }

        for (pEle2=pEle1->FirstChildElement(); pEle2; )
        {
            if (strcmp(pEle2->Value(), "channel")) goto next;

            for (pAttr=pEle2->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if (strcmp(pAttr->Name(), "name")) continue;
                strcpy(channame, pAttr->Value());
            }

            for (pEle3=pEle2->FirstChildElement(); pEle3; pEle3=pEle3->NextSiblingElement())
            {
                if (strcmp(pEle3->Value(), "addressInfo")) continue;

                nType = 0; // get type value, default is 0
                for (pAttr=pEle3->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if (strcmp(pAttr->Name(), "type")) continue;
                    nType = atoi(pAttr->Value());
                }

                // check normal url
                for (pAttr=pEle3->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if (strcmp(pAttr->Name(), "url")) continue;
                    if (nType == 1) bIsOK = TRUE;
                    else bIsOK = check_video_url(pAttr->Value());
                    if (nType == 1)
                    {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY);
                        printf("[OK]");
                        SetConsoleTextAttribute(hConsole, 0x7);
                    }
                    else if (bIsOK)
                    {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN|FOREGROUND_INTENSITY);
                        printf("[OK]");
                        SetConsoleTextAttribute(hConsole, 0x7);
                    }
                    else
                    {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED|FOREGROUND_INTENSITY);
                        printf("[NG]");
                        SetConsoleTextAttribute(hConsole, 0x7);
                        if (fp) fprintf(fp, "%s %s %s\n", livetype, channame, pAttr->Value());
                    }
                    printf("%s %s %s\n", livetype, channame, pAttr->Value());
                }
            }

next:
            pEleN = pEle2;
            pEle2 = pEle2->NextSiblingElement();
            if (!bIsOK) pEle1->RemoveChild(pEleN);
        }
    }

    pDoc0->SaveFile(strXmlOut0);
    if (fp) fclose(fp);
    printf("\ndone!\n");
    getch();
    return 0;
}
