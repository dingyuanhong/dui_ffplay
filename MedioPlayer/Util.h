#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <Windows.h>

static std::string UTF16_2_ANSI(const std::wstring &strUTF16)
{
	int nANSILenght=WideCharToMultiByte(CP_ACP,0,strUTF16.c_str(),-1,NULL,0,0,0);
	char *pTmp = new char[nANSILenght];
	memset(pTmp,0,nANSILenght);
	int nRet=WideCharToMultiByte(CP_ACP,0,strUTF16.c_str(),-1,pTmp,nANSILenght,0,0);
	std::string strRet = pTmp;
	if(pTmp != NULL) delete pTmp;
	return strRet;
}
static std::wstring ANSI_2_UTF16(const std::string &strANSI)
{
	int nMultyLength = MultiByteToWideChar(CP_ACP,0,strANSI.c_str(),-1,NULL,0);
	wchar_t *pTmp = new wchar_t[nMultyLength + 1];
	memset(pTmp,0,(nMultyLength+1)*sizeof(wchar_t));
	int nRet=MultiByteToWideChar(CP_ACP,0,strANSI.c_str(),-1,pTmp,nMultyLength);
	std::wstring strRet = pTmp;
	if(pTmp != NULL) delete pTmp;
	return strRet;
}
static std::string StrWTA(std::wstring w)
{
	return UTF16_2_ANSI(w);
}

static std::wstring StrATW(std::string w)
{
	return ANSI_2_UTF16(w);
}

#endif