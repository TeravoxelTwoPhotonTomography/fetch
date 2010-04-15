#pragma once
#include "stdafx.h"
#include "device-task.h"
#include "device-scanner.h"

DeviceTask* Scanner_Create_Task_Video(void);
DeviceTask* Scanner_Create_Task_Line_Scan(void);

void DeviceTask_Scanner_Video_Write_Waveforms( Scanner *scanner );
