// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4349F811_86B2_4D24_B1F9_E02F03A854D4__INCLUDED_)
#define AFX_STDAFX_H__4349F811_86B2_4D24_B1F9_E02F03A854D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef WIN32

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#pragma comment(lib, "pthreadVC1.lib")
#include <windows.h>
#include <Winsock.h>
#pragma comment(lib,"wsock32.lib")
typedef int socklen_t;


#else

typedef int SOCKET;
typedef unsigned char BYTE;
typedef void* HMODULE;
typedef unsigned int UINT;
typedef unsigned short WORD;
typedef short WCHAR;
typedef unsigned long DWORD;
typedef long long ULONGLONG;
typedef long long UINT64;
typedef long long __int64;
typedef int __int32;
typedef long long LONGLONG;
typedef unsigned int BOOL;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;
typedef void* LPVOID;
typedef void* POSITION;
#define INFINITE 0xFFFFFFFF
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>        //for struct ifreq
#include<netinet/in.h>    //互联网地址族   定义数据结构sockaddr_in
#include<arpa/inet.h>   //提供IP地址转换函数

typedef int SOCKET;
typedef unsigned char BYTE;

typedef void* LPVOID;
typedef unsigned int BOOL;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;

#define FALSE 0
#define SOCKET_ERROR (-1)
#define TRUE 1
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define INVALID_SOCKET (SOCKET)(~0)


#endif

#define TryDo(x) try{x;}catch(...){};

#include <pthread.h>

class CMyObject
{
public:
	CMyObject(){};
	virtual ~CMyObject(){};
};


class CMyString : public CMyObject
{
public:
	CMyString();
	virtual ~CMyString();
#ifndef FindCString
	CMyString(const char* p);
#endif
	CMyString(const CMyString& s);
	CMyString& operator = (const char* p);
	CMyString& operator = (const char c);
	CMyString& operator = (CMyString& p);
	CMyString& operator + (const char* p);
	CMyString& operator + (CMyString& s);
	CMyString& operator += (const char c);
	CMyString& operator += (const char* p);
	CMyString& operator += (CMyString& p);
	BOOL operator == (const char* p);
	BOOL operator > (const char* p);
	BOOL operator > (CMyString& p);
	BOOL operator < (const char* p);
	BOOL operator < (CMyString& p);
	BOOL operator >= (const char* p);
	BOOL operator >= (CMyString& p);
	BOOL operator <= (const char* p);
	BOOL operator <= (CMyString& p);
	BOOL operator == (CMyString& p);
	BOOL operator != (const char* p);
	BOOL operator != (CMyString& p);
	operator const char*() const;
	operator char*() const;
	const char* GetChar();
	char GetAt(int i);
	void SetAt (int nIndex, char ch);
	char* SetChar(const char* p);
	CMyString& TrimLeft(char c=' ');
	CMyString& TrimRight(char c=' ');
	int GetLength();
	char* Mid(int nPos, int nCount=-1);
	char* Left(int nPos);
	char* Right(int nCount);
	int Find(char c, int nStartPos=0);
	int Find(LPCSTR s, int nStartPos=0);
	int ReverseFind(char c, int nCount=1);
	CMyString& MakeLower();
	CMyString& MakeUpper();
	int CompareNoCase(const char* p);
	int Compare(const char* p);
	char* GetBuffer(int nLenNew);
	char* InsertStr(const char* pAdd);
	char* AppendStr(const char* pAdd);
	char* InsertStr(CMyString& pAdd);
	char* AppendStr(CMyString& pAdd);
	BOOL IsEmpty();
	BOOL IsNotEmpty();
	BOOL Equal(const char* szMsg);
	const char* DoubleText(const char* fmt, double d);
	const char* IntText(const char* fmt, int d);
	//const char* IntText(const char* fmt, __int64 d);
	//由数据精度获得浮点数的字符串值
	const char* GetDoubleText2(double d, CMyString& strDotCount);
	int Replace(char c1, char c2);	
	int Replace(LPCSTR s1, LPCSTR s2);	
	int ReplaceOnce(LPCSTR s1, LPCSTR s2);	
protected:
	char* AddStr(const char* pAdd,
		bool bAddHead
		);
	char* m_pStr;
	char* m_pStrPart;
	int m_nLenS;
};

class CMyMutex : public CMyObject//这个类不提供直接外部使用
{
public:
	CMyMutex();
	virtual ~CMyMutex();
	BOOL Lock(DWORD dwWaitMS=INFINITE);
	void Unlock();
protected:
	
	pthread_mutex_t m_lock;	
	pthread_mutexattr_t at;
};

class CMyPtrListBase : public CMyObject
{
public:
	CMyPtrListBase();
	virtual ~CMyPtrListBase();
	LPVOID m_pValue;
	CMyPtrListBase* m_pPrev;
	CMyPtrListBase* m_pNext;
protected:
};


#define ThreadPollNum 6

class CMyThreadPoll : public CMyObject
{
public:
	CMyThreadPoll();
	virtual ~CMyThreadPoll();
	static void* ThreadB(void* l);
	BOOL AddTask(SOCKET s);	
	static void StartThreadPollWebSvr();
	static void MoveToHead(CMyPtrListBase* pTh);
	static BOOL AllocWebSvrThread(SOCKET s);
	
	static CMyMutex m_mutexList;
	static CMyThreadPoll* m_pPoll;

	SOCKET m_s;
	pthread_t m_tida;	
	CMyPtrListBase* m_pPLB;
	// 	pthread_mutex_t m_mutex;	
	// 	pthread_cond_t m_cond;	
};

int readSK(int s, void *buf, int nSize);
int closeSK(int s);
int writeSK(int s, const void * buf, int nSize);
void LoopFeedback(SOCKET connfd, char szLastDir[]);

LPCSTR MyGetPrivateProfileString(LPCSTR strApp, LPCSTR strKey, LPCSTR strDefault, LPCSTR strIni, int nMaxLineSize=256);
int MyGetPrivateProfileInt(LPCSTR strApp, LPCSTR strKey, int nDefault, LPCSTR strIni);
void MyWritePrivateProfileInt(LPCSTR strApp, LPCSTR strKey, int nVal, LPCSTR strIni);
void MyWritePrivateProfileString(LPCSTR strApp, LPCSTR strKey, LPCSTR sVal, LPCSTR strIni, int nMaxLineSize=256);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4349F811_86B2_4D24_B1F9_E02F03A854D4__INCLUDED_)
