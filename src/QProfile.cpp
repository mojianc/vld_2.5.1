/*
This file is distributed "as is", e.g. there are no warranties
and obligations and you could use it in your applications on your
own risk. Although your comments and questions are welcome.

Source:            QProfile.cpp
Author:            (c) Dan Kozub, 1999
URL   :            http://DanKozub.com
Email :            mail@DanKozub.com
Last Modified:    Feb 7,1999
Version:        1.3
*/
#pragma warning(disable:4996 4554)
#include "QProfile.h"
#include <stdio.h>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <cinttypes>
typedef unsigned long       DWORD;
typedef unsigned short      WORD;
typedef const char*         LPCSTR;
#endif


#define QPROFILE_MIN_MAX

#ifdef _WIN32
static _declspec(thread) QProfileStarter * __s_LastActive = NULL;
#else
static __thread QProfileStarter * __s_LastActive = NULL;
#endif

//static
static        QProfile *        ChainHead = NULL;        //the pointer to the first class in the chain
int64_t          g_timerFrequency;        //frequency of the timer
static        int64_t          ProgramTime;    //total time of the programm
static        bool            StopProfiling = false;  //stops profiling
static        int                MaxFileNameLen; //is used to store max length of the source file

static        int                Output = QProfile_Out_All|QProfile_Out_File_Append|QProfile_Out_Add_Source;

static        const char *    OutFile ="QProfile.txt";;        // by default = "QProfile.txt"
static        int                OutFileMaxSize = 100*1024; // append output until file size < OutFileMaxSize
static        int                OutputFilter = QProfile_OutputFilters_Time;    // excludes some data e.g. time per call

char            QProfile::StrBuffer[QPROFILE_NAMELEN];



//comment above line to exclude min-max info collection
//this can speed up profiling a bit

// Initializing static variables

                        // by default time in ms is excluded from report
static QProfile_Sorting    SortBy = QProfile_Sort_None; //QProfile_Sort_PureTime;

// This is the main object to measure total time
#ifndef QPROFILE_REMOVE
QProfile            QProfile_Program("Total time");
#endif



//("============QProfile======================")
//("============QProfile======================")
//("============QProfile======================")
//("============QProfile======================")
QProfile::QProfile(const char * name,bool delete_after_report,
                   const char * file_name, int line_num)
{
    //if (name) ::lstrcpynA(Name,name,QPROFILE_NAMELEN);
    Name = name;
    //else Name[0]=0;
    DeleteAfterReport = delete_after_report;
    Elapsed = 0;
    LastStart = 0;
    TimeInChildren = 0;
    Counter = 0;
    Running = 0;
    Next = ChainHead;
    ChainHead = this;
    AutoStarterActive = 0;
#ifndef QPROFILE_REMOVE
    if (!QProfile_Program.IsRunning())     QProfile_Program.Start();
#endif
    FirstParentFunction = NULL;
    ReportPrinted = false;
#ifdef QPROFILE_MIN_MAX
    MaxTime=0;
    MinTime=0x7FFFFFFFFFFFFFFF;
#endif
    FileName=file_name;
    int fileNameLen = (int)strlen(file_name);
    if (fileNameLen > MaxFileNameLen)  MaxFileNameLen = fileNameLen;
    LineNumber=line_num;
};


//("============Reset=========================")
void QProfile::Reset(){
Elapsed = 0;
LastStart = 0;
Counter = 0;
Running = 0;
#ifdef QPROFILE_MIN_MAX
    MaxTime=0;
    MinTime=0x7FFFFFFFFFFFFFFF;
#endif
};


//("============PrintSummary==================")
void    QProfile::PrintSummary()
{
    static bool SummaryAlreadyPrinted = false;
    if (SummaryAlreadyPrinted) return;
    else SummaryAlreadyPrinted = true;
    //Summary should be printed only once
    StopProfiling=true;
#ifndef QPROFILE_REMOVE
    QProfile_Program.Stop();
    GET_TIMER_FREQ(g_timerFrequency);

    ProgramTime = QProfile_Program.Elapsed;

#endif

    char buff[255];
    Out("\r\n----------------- Profiling  results -----------------\r\n");
#ifdef _WIN32
    SYSTEMTIME time;GetLocalTime(&time);
    sprintf(buff,"Date: %02d.%02d.%02d, Time: %02d:%02d.%02d\r\n",
        time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute,time.wSecond);
    Out(buff);
#endif
    bool bContinue(true);
    while(bContinue)
    {
        QProfile * max = ChainHead;
        QProfile * cur = ChainHead;
        while(cur){
            if ((*max>*cur)==false)
                max=cur;
            cur=cur->Next;}
        if (max->ReportPrinted) bContinue=false;
        else max->PrintReport();
    }
    //Out("\r\n------------------------------------------------------\r\n",false,"");
    //let's go through all objects once more to delete some
    QProfile * cur = ChainHead;
    while(cur){
        QProfile * next = cur->Next;
        if (cur->DeleteAfterReport) delete cur;
        cur = next;
    }

    PrintMemorySummary();
    Out("------------------------------------------------------\r\n",true,"");
    return;
};

//("============PrintReport===================")
void    QProfile::PrintReport(int level){
    if (ReportPrinted) return; ReportPrinted = true;
    char buff[255];
    char buff2[255];
    double elapsed =(double)Elapsed/g_timerFrequency*1000;
    double share = (double)Elapsed/ProgramTime*100;
    //double no_children =(double)(Elapsed-TimeInChildren)/g_timerFrequency*1000;
    double no_children_share = (double)(Elapsed-TimeInChildren)/ProgramTime*100;
    static bool first_line = true;
    if (first_line){
        Out("------------------------------------------------------\r\n",false,"");
        Out("|-Child|Total ",false,"");
        if ((OutputFilter&QProfile_OutputFilters_Time)==0)
            Out("|Time (ms) ");
        if ((OutputFilter&QProfile_OutputFilters_Count)==0)
            Out("|  Hits  ");
        if ((OutputFilter&QProfile_OutputFilters_TimePerCall)==0)
            Out("|Time/call ");
#ifdef QPROFILE_MIN_MAX
        Out("|   MIN   |   MAX   ");
#endif
        Out("| Function    \r\n");
        Out("------------------------------------------------------\r\n",false,"");}
    if (no_children_share == share){
        if (SortBy == QProfile_Sort_Time)
            sprintf(buff,"|      |%6.2lf",no_children_share);
        else
            sprintf(buff,"|%6.2lf|      ",no_children_share);
    }else
        sprintf(buff,"|%6.2lf|%6.2lf",no_children_share,share);

    if (FileName) {
        sprintf(buff2,"%-*.*s(%3d) :",MaxFileNameLen,MaxFileNameLen,FileName,(WORD)LineNumber);
    }
    Out(buff,false,buff2);
    if ((OutputFilter&QProfile_OutputFilters_Time)==0){
        sprintf(buff,"|%10.2lf",elapsed);
        Out(buff);}
    if ((OutputFilter&QProfile_OutputFilters_Count)==0){
#ifdef _MSC_VER
        sprintf(buff,"|%8I64d",Counter);
#else
        sprintf(buff,"|%8" PRId64,Counter);
#endif
        Out(buff);}
    if ((OutputFilter&QProfile_OutputFilters_TimePerCall)==0){
        sprintf(buff,"|%12.5lf",elapsed/Counter);
        Out(buff);}
    Out("|");
#ifdef QPROFILE_MIN_MAX
    if (MinTime==0x7FFFFFFFFFFFFFFF) MinTime = 0;
    sprintf(buff,"%9.3lf|%9.3lf|",
        (double)MinTime/g_timerFrequency*1000,
        (double)MaxTime/g_timerFrequency*1000);
    Out(buff);
#endif
    for(int l=0;l<level;l++) Out("  ");
    Out(Name);

    int len = 50 - (int)strlen(Name); for(int i = 0; i< len; i++) buff[i]='-';
    if(len > 0){buff[len] = 0; Out(buff);}

    if ((DWORD)FirstParentFunction>1){
        sprintf(buff,"|%3.1lf%% of %s",(double)Elapsed/FirstParentFunction->Elapsed*100, FirstParentFunction->Name);
        Out(buff);
    }

    Out("\r\n");
    if (Output&QProfile_Out_DrawBar){
        double bar_share = no_children_share;
        if (SortBy == QProfile_Sort_Time) bar_share = share;
        Out(PrintBar(bar_share,100,80),false,"");
        Out("\r\n");}
    first_line = false;
    if (SortBy == QProfile_Sort_Time){
        QProfile * child = FindNextChild(NULL);
        while(child){
            child->PrintReport(level +1);
            child = FindNextChild(child);}
    }
    return;
};

//("============PrintBar======================")
char *        QProfile::PrintBar(double val,double max,int length){
    static char buff[256];
    if (length > 255) length =255;
    double to_print = (double)length*val/max;
    int to_print_int = (int)to_print;
    int i;
    for(i=0;i<length;i++){
        /*while(true){*/
            if (i==to_print_int){ buff[i]='#';/* break;*/}
            if (i==to_print_int*10){ buff[i]='#';/*break;*/}
            if (i<to_print_int){ buff[i]='>';/* break;*/}
            if (i<to_print_int*10){ buff[i]='=';/*break;*/}
            buff[i]='.';
            /*break;}*/
    }
    buff[i]=0;
    return buff;
};

//("============Out===========================")
bool QProfile::Out(LPCSTR string,bool last,LPCSTR debug_only)
{
//#ifdef _RPT0
//    if (Output&QProfile_Out_DebugWindow){
//        if (debug_only && (Output&QProfile_Out_Add_Source)){
//            if (*debug_only==0) {
//                for(int space=0;space<MaxFileNameLen+7;space++)
//                    _RPT0(_CRT_WARN," ");}
//            else
//                _RPT0(_CRT_WARN,debug_only);
//        }
//        _RPT0(_CRT_WARN,string);
//        if (last) _RPT0(_CRT_WARN,"\r\n");
//    }
//#endif
    if (debug_only)
    {
        //todo
    }
    if (Output&QProfile_Out_Consol){
        printf("%s", string);
        if (last) printf("\r\n");
    }

#ifdef _WIN32
    if ((Output&QProfile_Out_File)==0){ return true;}
    static HANDLE Handle = NULL;
    if (Handle==NULL){

        char szPath[2*_MAX_PATH];
        GetModuleFileNameA(NULL, szPath, _countof(szPath));
        char * p = strrchr(szPath, '\\')+1;
        strcpy(p, OutFile);
        Handle =::CreateFileA(szPath,GENERIC_WRITE,0,0L,OPEN_ALWAYS,0,0);
        int sz= ::GetFileSize(Handle,NULL);
        if ((Output&QProfile_Out_File_Append) && sz < OutFileMaxSize) ::SetFilePointer(Handle,0,NULL,FILE_END);
        else ::SetEndOfFile(Handle);
    };
    DWORD written = 0;
    WriteFile(Handle,string,static_cast<DWORD>(strlen(string)),&written,NULL);
    //BOOL ok=::WriteFile(Handle,string,strlen(string),&written,NULL);
    if (last) ::CloseHandle(Handle);
#endif
    return true;
};

//("============FindNextChild=================")
QProfile *QProfile::FindNextChild(QProfile * find_after)
{
    QProfile * cur = ChainHead;
    if (find_after) cur =  find_after->Next;
    while(cur){
        if (cur->FirstParentFunction == this) return cur;
        cur=cur->Next;
    }
    return NULL;
};


//("============operator>=====================")
bool QProfile::operator>(QProfile& to_compare)
{
    if (to_compare.ReportPrinted) return true;
    if (ReportPrinted) return false;
    if (SortBy == QProfile_Sort_Time){
        if (to_compare.FirstParentFunction!=FirstParentFunction)
            return (DWORD)FirstParentFunction<=1;
    }
    switch(SortBy){
    case QProfile_Sort_Time:
        if (Elapsed > to_compare.Elapsed) return true;
        else return false;
    case QProfile_Sort_PureTime:
        if (Elapsed-TimeInChildren >
            to_compare.Elapsed - to_compare.TimeInChildren) return true;
        else return false;
    case QProfile_Sort_Count:
        if (Counter > to_compare.Counter) return true;
        else return false;
    case QProfile_Sort_TimePerCall:
        if ((double)Elapsed/Counter >
            (double)to_compare.Elapsed/to_compare.Counter ) return true;
        else return false;
    default: return false;
    }
};





//("============QProfileStarter===============")
//("============QProfileStarter===============")
//("============QProfileStarter===============")

//("============RecursiveCallFrom=============")
QProfileStarter *    QProfileStarter::RecursiveCallFrom()
{
    QProfileStarter * cur = Parent;
    while(cur){
        if (cur->Profile==Profile) return cur;
        cur=cur->Parent;
    }
    return NULL;
}

//("============QProfileStarter===============")
QProfileStarter::~QProfileStarter(){
    if (StopProfiling) return;
    int64_t       now;
    GET_TIMER_VALUE(now);
    int64_t       elapsed = now-StartTime;
    __s_LastActive = Parent;
    bool maybe_recursive = (--Profile->AutoStarterActive) > 0;
#ifdef QPROFILE_MIN_MAX
    if (elapsed>Profile->MaxTime)
        Profile->MaxTime=elapsed;
    if (elapsed<Profile->MinTime)
        Profile->MinTime=elapsed;
#endif
    if (!Parent){
        Profile->Add(elapsed,TimeInChildren); return;
    }
    // checking calling function to be unique caller
    if (!Profile->FirstParentFunction){
        Profile->FirstParentFunction = Parent->Profile;}
    else{
        if (Profile->FirstParentFunction!=Parent->Profile)
            Profile->FirstParentFunction = (QProfile*)(DWORD)1;}


    QProfileStarter * recursive = NULL;
    if (maybe_recursive)
        recursive=RecursiveCallFrom();

    if (!recursive){
        Profile->Add(elapsed,TimeInChildren);
        Parent->TimeInChildren+=elapsed;
    }else {
        Profile->Counter++;
        if (recursive==Parent){
            Parent->TimeInChildren+=TimeInChildren;}
        else {
            Parent->TimeInChildren+=elapsed;
            //time in recursive call should be excluded
            recursive->TimeInChildren -= elapsed-TimeInChildren;
        }
    }
    return;
}


QProfileStarter::QProfileStarter( QProfile * profile )
{
    Profile=profile;
    Parent = __s_LastActive;
    __s_LastActive = this;
    TimeInChildren = 0;
    Profile->AutoStarterActive++;
    GET_TIMER_VALUE(StartTime);
}


void GetCPUFreq(int64_t & v)
{
#ifdef _WIN32
    DWORD BufSize = _MAX_PATH;
    DWORD dwMHz = _MAX_PATH;
    HKEY hKey;

    // open the key where the proc speed is hidden:

    long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0,
        KEY_READ,
        &hKey);

    if(lError != ERROR_SUCCESS)
    {// if the key is not found, tell the user why:
        v = 0;
        return;
    }

    // query the key:

    RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE) &dwMHz, &BufSize);

    // convert the DWORD to a CString:
    v = dwMHz * 1000000;
#endif // _WIN32
}

const int __maxMemoryStatus = 1024;
static double __memoryUseds[__maxMemoryStatus];
static const char * __memoryTags[__maxMemoryStatus];
static int __countMemoryStatus = 0;

 int QPAddMemoryStatus( const char * tag )
 {
#ifdef _WIN32
     if(__countMemoryStatus>__maxMemoryStatus){
         //assert(false && "status number overflow");
         return 0;
     }
     PROCESS_MEMORY_COUNTERS pmc;
     GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
     __memoryUseds[__countMemoryStatus] = (double)pmc.PagefileUsage/(1024.0*1024.0);
     __memoryTags[__countMemoryStatus] = tag;
     __countMemoryStatus++;
#endif
     return 1;
 }

double QPMemoryDiffent( const char * tag1, const char * tag2 )
{
    int i;
    for(i = 0; i < __countMemoryStatus; i++){
        if(!strcmp(__memoryTags[i],tag1)){
            break;
        }
    }

    int i1 = i;

    for(i = 0; i < __countMemoryStatus; i++){
        if(!strcmp(__memoryTags[i],tag2)){
            break;
        }
    }

    int i2 = i;

    if((i1 < __countMemoryStatus) && (i2< __countMemoryStatus)){
        return __memoryUseds[i1] - __memoryUseds[i2];
    }else{
        return 0.0;
    }

}


void QProfile::PrintMemorySummary()
{
    if(__countMemoryStatus > 0){
        Out("----------------- memory  status -----------------\r\n");
        Out("|used/M|Inc/M | time Tag ---------------------------------\r\n");
    }

    char buff[255];
    double lastmem = 0.0;
    for(int i=0; i < __countMemoryStatus; i++){
        sprintf(buff,"|%6.2lf|%6.2lf| %s\r\n", __memoryUseds[i], __memoryUseds[i] - lastmem, __memoryTags[i]);
        Out(buff);
        lastmem = __memoryUseds[i];
    }
}


REM("============~QProfile=====================")
 QProfile::~QProfile()
{
    if (ChainHead && ChainHead->Next) {
        PrintSummary();
    }
};
