#ifndef NTB_LOG_H
#define NTB_LOG_H
#include "common.h"
#include "P7_Trace.h"
#include "P7_Telemetry.h"

#define  NTB_LOG_TRACE P7_TRACE_LEVEL_TRACE
#define  NTB_LOG_DEBUG P7_TRACE_LEVEL_DEBUG
#define  NTB_LOG_INFO P7_TRACE_LEVEL_INFO
#define  NTB_LOG_WARN P7_TRACE_LEVEL_WARNING
#define  NTB_LOG_ERR P7_TRACE_LEVEL_ERROR
#define  NTB_LOG_CRIT P7_TRACE_LEVEL_CRITICAL


namespace ntb
{
enum LogSink{
    SINK_COMMON,
    LogSinksSize
};


#ifdef NTB_DEBUG_LOGGING
void logDebugNoMacro(tUINT16 i_wLine, const char *i_pFile, const char *i_pFunction, const tXCHAR *i_pFormat ...);
#define ntbLogDebug(...) ntb::logDebugNoMacro(__LINE__, __FILE__ ,__FUNCTION__, __VA_ARGS__);
#else
#define ntbLogDebug(...){}
#endif

#ifdef NTB_ENABLE_LOGGING
void logNoMacro(ulong log_sink, uint level,tUINT16 i_wLine, const char *i_pFile, const char *i_pFunction, const tXCHAR *i_pFormat ...);
#define ntbLog(log_sink,level,...) ntb::logNoMacro(log_sink,level, __LINE__, __FILE__ ,__FUNCTION__, __VA_ARGS__);
#else
#define ntbLog(...){}
#endif



void p7Release(uint log_sink=0, IP7_Telemetry *l_pTelemetry=nullptr, IP7_Client *l_pClient=nullptr);
bool p7Init(std::string_view wName);
bool p7RegThread(uint log_sink,std::string_view str)noexcept;
void p7UnregThread(uint log_sink);

}//namespace end
#endif // NTB_LOG_H
