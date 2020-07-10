#ifndef QPROFILE_H
#define QPROFILE_H

#ifndef REM
#define REM(param)
#endif



#include "Common/Common.h"
#include <stdint.h>

#if defined(_MSC_VER)
    #include <intrin.h>
    #define GET_TIMER_VALUE(val)    val = __rdtsc()
#else
    #include <x86intrin.h>
    #define GET_TIMER_VALUE(val)    val = __rdtsc()
#endif

//#define QPROFILE_REMOVE

void SYS_COMMON_API GetCPUFreq(int64_t & v);

#define GET_TIMER_FREQ(val)        GetCPUFreq(val)

#define QPROFILE_NAMELEN 80

//usage: QProfile::SortBy = ONE of the following values
enum QProfile_Sorting {
    QProfile_Sort_None,        // No sort
    QProfile_Sort_Time,        // sorting by total time
    QProfile_Sort_PureTime,    // sorting by "pure time", i.e. except time in subfunctions
    QProfile_Sort_TimePerCall, // sorting by average time per call
    QProfile_Sort_Count        // sorting by count
};


//usage: QProfile::Output = combination of the flags below
enum QProfile_OutputFlags {
    QProfile_Out_None = 0,            // no output at all
    QProfile_Out_File = 1,            // output to file
    QProfile_Out_Consol = 2,        // output to consol
    QProfile_Out_DebugWindow = 4,    // output to VC debug window
    QProfile_Out_All = 7,            // output to file, consol, debug window
    QProfile_Out_File_Append = 8,    // when writing to file - append new data
    QProfile_Out_Add_Source = 16,    // add source line info to debug window
    QProfile_Out_DrawBar = 32        // add a bar after each line
};

//usage: QProfile::OutputFilter = combination of the following flags
//aim: to exclude data, you do not need for your analysis.
enum QProfile_OutputFilters {
    QProfile_OutputFilters_None = 0,
    QProfile_OutputFilters_Time = 1,
    QProfile_OutputFilters_PureTime = 2,
    QProfile_OutputFilters_TimePerCall = 4,
    QProfile_OutputFilters_Count = 8
};



REM("============QProfile======================")
    REM("============QProfile======================")
    REM("============QProfile======================")

    extern int64_t          g_timerFrequency;


class SYS_COMMON_API QProfile {
    friend class QProfileStarter;

public:

    static        void            PrintSummary();    //you can call this function directly
private:
    static        char *            PrintBar(double val,double max,int lenght);
    static        bool            Out(const char * string,bool last=false,const char * debug_only=nullptr);

    double        ElapsedInMS() const;
    void        PrintReport(int level = 0);
    void        Add(const int64_t& add, const int64_t& in_children);
    bool        operator>(QProfile& to_compare);
    QProfile *    FindNextChild(QProfile * find_after);

    bool                DeleteAfterReport;    //this object is constructed by new and should be deleted
    int64_t              Elapsed;            //time in function
    int64_t              TimeInChildren;        //time in children
    int64_t              LastStart;            //last start time
    int64_t              MaxTime;            //max time per call
    int64_t              MinTime;            //min time per call
    int64_t            Counter;            //count of calls
    QProfile *            Next;                //pointer to next class
    int                    Running;            //Start() was called
    int                    AutoStarterActive;    //
    QProfile *            FirstParentFunction;//Pointer to the parent function if it is UNIQUE!
    bool                ReportPrinted;        //this function report is already printed
    const char *        FileName;            //name of the source file this objects was constructed
    int                    LineNumber;            //line of the source file this objects was constructed
public:
    const char * Name;
    //char                Name[QPROFILE_NAMELEN]; //name of the function
    static        char            StrBuffer[QPROFILE_NAMELEN];

public:
    QProfile(const char * name,bool delete_after_report=false, const char * file_name="", int line_num=0);
    ~QProfile();
    void        Start();
    void        Stop();
    void        Reset();
    bool        IsRunning() const { return Running!=0; }
    static void PrintMemorySummary();
};

SYS_COMMON_API int  QPAddMemoryStatus(const char * tag);
SYS_COMMON_API double  QPMemoryDiffent(const char * tag1, const char * tag2);

extern QProfile            QProfile_Program;



REM("============Start=========================")
    inline    void    QProfile::Start(){
        if (!Running){
            GET_TIMER_VALUE(LastStart);}
        Running++;};

        REM("============Stop==========================")
            inline    void    QProfile::Stop(){
                if (!Running) return;
                Counter++;
                Running--;
                if (Running){ return;}
                int64_t    now;
                GET_TIMER_VALUE(now);
                Elapsed += now-LastStart;
        };

        inline    void    QProfile::Add(const int64_t& add, const int64_t& in_children)
        {
            Elapsed += add;
            TimeInChildren += in_children;
            Counter++;
        };

        inline double    QProfile::ElapsedInMS() const
        {
            return (double) Elapsed/ g_timerFrequency*1000;
        }








        REM("============QProfile_Starter==============")
            REM("============QProfile_Starter==============")
            REM("============QProfile_Starter==============")
        class SYS_COMMON_API QProfileStarter {
            QProfile *            Profile;
            QProfileStarter *    Parent;
            int64_t              StartTime;
            int64_t              TimeInChildren;
        public:

            QProfileStarter(QProfile * profile);

            ~QProfileStarter();

        private:
            QProfileStarter *    RecursiveCallFrom();
        };



#ifdef QPROFILE_REMOVE
#define QP_MEM(name)

#define QP_SUMMARY()

#define    QP_FUN(name)
#define    QP_FUN1(name,param)
#define    QP_FUN2(name,param,param2)
#define    QP_FUN3(name,param,param2,param3)

#define    QP_MT_FUN(name)
#define    QP_MT_FUN1(name,param)
#define    QP_MT_FUN2(name,param,param2)
#define    QP_MT_FUN3(name,param,param2,param3)

#define    QP_NEW_FUN1(name,param)
#define    QP_NEW_FUN2(name,param,param2)

#define    QPROFILE_DECLARE(var,name)
#define    QPROFILE_DECLARE1(var,name,param)
#define    QPROFILE_DECLARE2(var,name,param,param2)
#define    QPROFILE_DECLARE3(var,name,param,param2,param3)

#define    QPROFILE_MT_DECLARE(var,name)
#define    QPROFILE_MT_DECLARE1(var,name,param)
#define    QPROFILE_MT_DECLARE2(var,name,param,param2)
#define    QPROFILE_MT_DECLARE3(var,name,param,param2,param3)

#else //not QPROFILE_REMOVE
#define QP_SUMMARY() QProfile::PrintSummary()

#define QP_MEM(name) {static int __memStatus = QPAddMemoryStatus(name);}

        REM("============QPROFILE_DECLARE==============")
#define    QPROFILE_DECLARE(var,name)    \
    static QProfile var(name,false,__FILE__,__LINE__);

#define    QPROFILE_DECLARE1(var,name,param) \
    static QProfile var(QProfile::StrBuffer+\
    !sprintf(QProfile::StrBuffer,name,param),false,__FILE__,__LINE__);
#define    QPROFILE_DECLARE2(var,name,param,param2) \
    static QProfile var(QProfile::StrBuffer+\
    !sprintf(QProfile::StrBuffer,name,param,param2),false,__FILE__,__LINE__);
#define    QPROFILE_DECLARE3(var,name,param,param2,param3) \
    static QProfile var(QProfile::StrBuffer+\
    !sprintf(QProfile::StrBuffer,name,param,param2,param3),false,__FILE__,__LINE__);


            REM("============QPROFILE_MT_DECLARE===========")
#define    QPROFILE_MT_DECLARE(var,name) \
    _declspec(thread) static QProfile * var=NULL; \
    if (!var) var = new QProfile(name,true,__FILE__,__LINE__);
#define    QPROFILE_MT_DECLARE1(var,name,param) \
    _declspec(thread) static QProfile * var=NULL; \
    if (!var){ var = new QProfile(name,true,__FILE__,__LINE__); \
    sprintf(var->Name,name,param);}
#define    QPROFILE_MT_DECLARE2(var,name,param,param2) \
    _declspec(thread) static QProfile * var=NULL; \
    if (!var){ var = new QProfile(name,true,__FILE__,__LINE__); \
    sprintf(var->Name,name,param,param2);}
#define    QPROFILE_MT_DECLARE3(var,name,param,param2,param3) \
    _declspec(thread) static QProfile * var=NULL; \
    if (!var){ var = new QProfile(name,true,__FILE__,__LINE__); \
    sprintf(var->Name,name,param,param2,param3);}



            REM("============QPROFILE_FUN==========")

#define    QP_FUN(name)    \
    QPROFILE_DECLARE(static_profile,name) \
    QProfileStarter static_profile_auto(&static_profile);
#define    QP_FUN1(name,param) \
    QPROFILE_DECLARE1(static_profile1,name,param) \
    QProfileStarter static_profile_auto1(&static_profile1);
#define    QP_FUN2(name,param,param2) \
    QPROFILE_DECLARE2(static_profile2,name,param,param2) \
    QProfileStarter static_profile_auto2(&static_profile2);
#define    QP_FUN3(name,param,param2,param3) \
    QPROFILE_DECLARE3(static_profile3,name,param,param2,param3) \
    QProfileStarter static_profile_auto3(&static_profile3);



            REM("============QPROFILE_MT_FUN===============")
#define    QP_MT_FUN(name)    \
    QPROFILE_MT_DECLARE(profile_ptr,name) \
    QProfileStarter profile_ptr_auto(profile_ptr);
#define    QP_MT_FUN1(name,param) \
    QPROFILE_MT_DECLARE1(profile_ptr,name,param) \
    QProfileStarter profile_ptr_auto(profile_ptr);
#define    QP_MT_FUN2(name,param,param2) \
    QPROFILE_MT_DECLARE2(profile_ptr,name,param,param2) \
    QProfileStarter profile_ptr_auto(profile_ptr);
#define    QP_MT_FUN3(name,param,param2,param3) \
    QPROFILE_MT_DECLARE3(profile_ptr,name,param,param2,param3) \
    QProfileStarter profile_ptr_auto(profile_ptr);

            REM("============QPROFILE_NEW_FUN1=============")
#define    QP_NEW_FUN1(name,param) \
    QProfile * profile_ptr= new QProfile(name,true,__FILE__,__LINE__); \
    sprintf(profile_ptr->Name,name,param);\
    QProfileStarter profile_ptr_auto(profile_ptr);

#define    QP_NEW_FUN2(name,param,param2) \
    QProfile * profile_ptr= new QProfile(name,true,__FILE__,__LINE__); \
    sprintf(profile_ptr->Name,name,param,param2);\
    QProfileStarter profile_ptr_auto(profile_ptr);

#endif //QPROFILE_REMOVE


#endif
