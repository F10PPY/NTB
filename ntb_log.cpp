#include "ntb_log.h"


namespace ntb
{
static  std::array<IP7_Trace *,LogSinksSize>log_sink_arr;
void p7Release(u_int log_sink, IP7_Telemetry *l_pTelemetry, IP7_Client *l_pClient)
{
    if (l_pTelemetry)
    {
        l_pTelemetry->Release();
        l_pTelemetry = nullptr;
    }

    if (log_sink!=0)
    {
        log_sink_arr[log_sink]->Release();
        log_sink_arr[log_sink]=nullptr;
    }

    if (l_pClient)
    {
        l_pClient->Release();
        l_pClient = nullptr;
    }
}
bool p7RegThread(uint log_sink,std::string_view str)noexcept
{
    return log_sink_arr[log_sink]->Register_Thread(TM(str.data()),0);
}
void p7UnregThread(uint log_sink)
{
    log_sink_arr[log_sink]->Unregister_Thread(0);
}
bool p7Init(std::string_view wName)
{
    IP7_Client *pClient=nullptr;

    log_sink_arr[SINK_COMMON]=nullptr;
    IP7_Telemetry *pTelemetry=nullptr;
    pClient = P7_Create_Client(TM("/P7.On=1 /P7.Pool=4096 /P7.Sink=Baical /P7.Files=2 /P7.Addr=192.168.226.1"));
    //pClient = P7_Create_Client(TM("/P7.On=1 /P7.Pool=4096 /P7.Sink=FileTxt /P7.Verb=4"));
    if (nullptr == pClient)
    {
        p7Release(SINK_COMMON,pTelemetry,pClient);
        return false;
    }


    log_sink_arr[SINK_COMMON] = P7_Create_Trace(pClient, TM(wName.data()));
    if (nullptr == log_sink_arr[SINK_COMMON])
    {
        p7Release(SINK_COMMON);
        return false;
    }
    //    stTelemetry_Conf   l_stConf     = {};
    //    l_stConf.pContext              = nullptr;
    //    l_stConf.pEnable_Callback      = nullptr;
    //    l_stConf.pTimestamp_Callback   = nullptr;
    //    l_stConf.qwTimestamp_Frequency = 0ull;
    //    l_stConf.pConnect_Callback     = nullptr;
    //	pTelemetry = P7_Create_Telemetry(pClient, TM("Telemetry channel 1"), &l_stConf);
    //	if (nullptr == pTelemetry)
    //	{
    //		p7Release(nullptr,pTelemetry,nullptr);
    //		return false;
    //	}
    if(!log_sink_arr[SINK_COMMON]->Share("Shared trace"))
    {
        puts("Can't share P7 trace");
        //p7Release(pTrace,pTelemetry,pClient);
        return false;
    }
    //	if(!pTelemetry->Share("Shared telemetry"))
    //	{
    //		puts("Can't share telemetry");
    //		//p7Release(pTrace,pTelemetry,pClient);
    //		return false;
    //	}
   return true;
}

#ifdef NTB_ENABLE_LOGGING
void logNoMacro(ulong log_sink, uint level,tUINT16 i_wLine, const char *i_pFile, const char *i_pFunction, const tXCHAR *i_pFormat ...)
{
    va_list l_pVl;
    va_start(l_pVl, i_pFormat);
    P7_Trace_Embedded(log_sink_arr[log_sink],
                      0,
                      level,
                      nullptr,
                      i_wLine,
                      i_pFile,
                      i_pFunction,
                      &i_pFormat,
                      &l_pVl
                      );
    va_end(l_pVl);
}
#endif
#ifdef NTB_DEBUG_LOGGING
void logDebugNoMacro(tUINT16 i_wLine, const char *i_pFile, const char *i_pFunction, const tXCHAR *i_pFormat ...)
{
    va_list l_pVl;
    va_start(l_pVl, i_pFormat);
    P7_Trace_Embedded(log_sink_arr[SINK_COMMON],
                      0,
                      NTB_LOG_DEBUG,
                      nullptr,
                      i_wLine,
                      i_pFile,
                      i_pFunction,
                      &i_pFormat,
                      &l_pVl
                      );
    va_end(l_pVl);
}
#endif
}//namespace end

