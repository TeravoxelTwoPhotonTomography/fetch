//
// Log to QT4 
// ----------
//
#include "common.h"
#include <QtGlobal>

void _qt_crit(const char* msg) {qCritical(msg);}
void _qt_dbg (const char* msg) {qDebug(msg);}
void _qt_warn(const char* msg) {qWarning(msg);}

void Reporting_Setup_Log_To_Qt(void)
{ Register_New_Error_Reporting_Callback(_qt_crit);
  Register_New_Debug_Reporting_Callback(_qt_dbg);
  Register_New_Warning_Reporting_Callback(_qt_warn);
}

