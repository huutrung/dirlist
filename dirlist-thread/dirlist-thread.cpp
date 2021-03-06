#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>
#pragma comment(lib, "User32.lib")

typedef struct MyData {
	TCHAR* pathOfDirectory;
	int idepth;
} MYDATA, *PMYDATA;

void DisplayErrorBox(LPTSTR lpszFunction);

DWORD WINAPI ListFilesInDirectory(LPVOID lpParam)
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

	// Extract data from lpParam here
	PMYDATA param = (PMYDATA)lpParam;

	// Store pathToDirectory to a new variable
	StringCchCopy(szDir, MAX_PATH, param->pathOfDirectory);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) 
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
	} 

	// List all the files in the directory with some info about them.

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			TCHAR* curDir;
			curDir = new TCHAR [MAX_PATH];
			StringCchCopy(curDir, MAX_PATH, param->pathOfDirectory);
			StringCchCat(curDir, MAX_PATH, TEXT("\\"));
			StringCchCat(curDir, MAX_PATH, ffd.cFileName);

			if (_tcscmp(ffd.cFileName, _T(".")) && _tcscmp(ffd.cFileName, _T("..")))
			{
				_tprintf(TEXT("%*s%s  DIR\n"), param->idepth * 4, "", ffd.cFileName);

				//Need to modify with thread create
				PMYDATA childDirectory = (PMYDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					sizeof(MYDATA));;
				childDirectory->pathOfDirectory = curDir;
				childDirectory->idepth = param->idepth + 1;
				DWORD   dwThreadId;
				HANDLE hThread = CreateThread( 
					NULL,                   // default security attributes
					0,                      // use default stack size  
					ListFilesInDirectory,       // thread function name
					childDirectory,          // argument to thread function 
					0,                      // use default creation flags 
					&dwThreadId);   // returns the thread identifier
				
				// Check the return value for success.
				// If CreateThread fails, terminate execution. 
				// This will automatically clean up threads and memory. 

				if (hThread == NULL) 
				{
					DisplayErrorBox(TEXT("CreateThread"));
					ExitProcess(3);
				}

				WaitForSingleObject(hThread, INFINITE);
				CloseHandle(hThread);
			}
			delete curDir;
		}
		else
		{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			_tprintf(TEXT("%*s%s   %ld bytes\n"), param->idepth*4, "", ffd.cFileName, filesize.QuadPart);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) 
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
	}

	FindClose(hFind);
}

int _tmain(int argc, TCHAR *argv[])
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);

	// If the directory is not specified as a command-line argument,
	// print usage.

	if(argc != 2)
	{
		_tprintf(TEXT("\nUsage: %s <directory name>\n"), argv[0]);
		return (-1);
	}

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	StringCchLength(argv[1], MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
	{
		_tprintf(TEXT("\nDirectory path is too long.\n"));
		return (-1);
	}

	_tprintf(TEXT("\nTarget directory is %s\n\n"), argv[1]);

	PMYDATA rootDirectory = (PMYDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(MYDATA));

	rootDirectory->pathOfDirectory = argv[1];
	rootDirectory->idepth = 0;
	DWORD   dwThreadId;
	HANDLE hThread = CreateThread( 
		NULL,                   // default security attributes
		0,                      // use default stack size  
		ListFilesInDirectory,       // thread function name
		rootDirectory,          // argument to thread function 
		0,                      // use default creation flags 
		&dwThreadId);   // returns the thread identifier

	// Check the return value for success.
	// If CreateThread fails, terminate execution. 
	// This will automatically clean up threads and memory. 

	if (hThread == NULL) 
	{
		DisplayErrorBox(TEXT("CreateThread"));
		ExitProcess(3);
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	return 0;
}


void DisplayErrorBox(LPTSTR lpszFunction) 
{ 
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message and clean up

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}