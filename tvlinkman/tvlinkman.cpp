// 包含头文件
#include <stdio.h>
#include "tinyxml.h"

int main(void)
{
    TiXmlDocument *pDoc = new TiXmlDocument();
    if (NULL == pDoc)
    {
        return false;
    }
    pDoc->LoadFile("test.xml");

    TiXmlNode* pXmlFirst = pDoc->FirstChild();     
    if (NULL != pXmlFirst)
    {
        TiXmlDeclaration* pXmlDec = pXmlFirst->ToDeclaration();
        if (NULL != pXmlDec)
        {
            const char *strVersion    = pXmlDec->Version();
            const char *strStandalone = pXmlDec->Standalone();
            const char *strEncoding   = pXmlDec->Encoding();
            printf("%s, %s, %s\n", strVersion, strStandalone, strEncoding);
        }
    }

    system("pause");
    return 0;
}
