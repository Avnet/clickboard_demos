#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/i2c.h>

#include "../Hardware/mt3620/inc/hw/mt3620.h"

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"

// This C application for the MT3620 Reference Development Board (Azure Sphere)


void sleep_1sec(void)
{
	const struct timespec sleepTime = { 1, 0 };
	nanosleep(&sleepTime, NULL);
}


void print_pal_error(VL53L0X_Error Status, int line)
{
	char buf[VL53L0X_MAX_STRING_LENGTH];
	VL53L0X_GetPalErrorString(Status, buf);
	Log_Debug("API Status (%d): %i : %s\n", line, Status, buf);
}

void print_range_status(VL53L0X_RangingMeasurementData_t* pRangingMeasurementData)
{
	char buf[VL53L0X_MAX_STRING_LENGTH];
	uint8_t RangeStatus;

	RangeStatus = pRangingMeasurementData->RangeStatus;

	VL53L0X_GetRangeStatusString(RangeStatus, buf);
	Log_Debug(" Range Status: %i : %s, ", RangeStatus, buf);
}

VL53L0X_Error WaitMeasurementDataReady(VL53L0X_DEV Dev)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	uint8_t       NewDatReady = 0;
	uint32_t      LoopNb;

	LoopNb = 0;
	do {
		Status = VL53L0X_GetMeasurementDataReady(Dev, &NewDatReady);
		if ((NewDatReady == 0x01) || Status != VL53L0X_ERROR_NONE)
			break;
		LoopNb = LoopNb + 1;
		VL53L0X_PollingDelay(Dev);
	} while (LoopNb < VL53L0X_DEFAULT_MAX_LOOP);
	if (LoopNb >= VL53L0X_DEFAULT_MAX_LOOP)
		Status = VL53L0X_ERROR_TIME_OUT;
	return Status;
}

VL53L0X_Error WaitStopCompleted(VL53L0X_DEV Dev)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	uint32_t StopCompleted = 0;
	uint32_t LoopNb;

	LoopNb = 0;
	do {
		Status = VL53L0X_GetStopCompletedStatus(Dev, &StopCompleted);
		if ((StopCompleted == 0x00) || Status != VL53L0X_ERROR_NONE)
			break;
		LoopNb = LoopNb + 1;
		VL53L0X_PollingDelay(Dev);
	} while (LoopNb < VL53L0X_DEFAULT_MAX_LOOP);

	if (LoopNb >= VL53L0X_DEFAULT_MAX_LOOP)
		Status = VL53L0X_ERROR_TIME_OUT;
	return Status;
}


VL53L0X_Error rangingTest(VL53L0X_Dev_t *pMyDevice, int secs)
{
	VL53L0X_RangingMeasurementData_t RangingMeasurementData;
	VL53L0X_RangingMeasurementData_t *pRangingMeasurementData = &RangingMeasurementData;
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	uint32_t refSpadCount;
	uint8_t isApertureSpads;
	uint8_t VhvSettings;
	uint8_t PhaseCal;

	if ((Status = VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE)
		return Status;

	if ((Status = VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE)
		return Status;

	if ((Status = VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE)
		return Status;

	if ((Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING)) != VL53L0X_ERROR_NONE)
		return Status;

	if ((Status = VL53L0X_StartMeasurement(pMyDevice)) != VL53L0X_ERROR_NONE)
		return Status;

	for (int i = 0; i < secs && (Status == VL53L0X_ERROR_NONE); i++) {
		if ((Status = WaitMeasurementDataReady(pMyDevice)) == VL53L0X_ERROR_NONE) {
			Status = VL53L0X_GetRangingMeasurementData(pMyDevice, pRangingMeasurementData);
			Log_Debug("measurement (%d): %6d (mm)\n", i + 1, pRangingMeasurementData->RangeMilliMeter);
			VL53L0X_ClearInterruptMask(pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
			VL53L0X_PollingDelay(pMyDevice);
			sleep_1sec();
		}
	}

	if (Status != VL53L0X_ERROR_NONE)
		return Status;

	if ((Status = VL53L0X_StopMeasurement(pMyDevice)) != VL53L0X_ERROR_NONE)
		return Status;

	if ((Status = WaitStopCompleted(pMyDevice)) != VL53L0X_ERROR_NONE)
		return Status;

	return VL53L0X_ClearInterruptMask(pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
}

VL53L0X_Error SingleRanging(VL53L0X_Dev_t *pMyDevice, int secs)
{
	VL53L0X_RangingMeasurementData_t RangingMeasurementData;
	FixPoint1616_t LimitCheckCurrent;
	VL53L0X_Error  stat;
	uint32_t       refSpadCount;
	uint8_t        isApertureSpads;
	uint8_t        VhvSettings;
	uint8_t        PhaseCal;

	if ((stat = VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE)
		return stat;
	if ((stat = VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE)
		return stat;
	if ((stat = VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE)
		return stat;
	if ((stat = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) != VL53L0X_ERROR_NONE)
		return stat;
	if ((stat = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return stat;
	if ((stat = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return stat;
	if ((stat = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1)) != VL53L0X_ERROR_NONE)
		return stat;
	if ((stat = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, (FixPoint1616_t)(1.5*0.023 * 65536))) != VL53L0X_ERROR_NONE)
		return stat;

	for (int i = 0; i < secs && (stat == VL53L0X_ERROR_NONE); i++) {
		Log_Debug("Measurement %d; ", i);
		if ((stat = VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE)
			print_pal_error(stat,__LINE__);
		print_range_status(&RangingMeasurementData);
		VL53L0X_GetLimitCheckCurrent(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent);
		Log_Debug(" RANGE IGNORE THRESHOLD: %.6f - ", (float)LimitCheckCurrent / 65536.0);
		Log_Debug("Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
		sleep_1sec();
	}
	return stat;
}

VL53L0X_Error HighAccuracy(VL53L0X_Dev_t *pMyDevice, int secs)
{
	VL53L0X_RangingMeasurementData_t RangingMeasurementData;
	FixPoint1616_t LimitCheckCurrent;
	VL53L0X_Error  status;
	uint32_t       refSpadCount;
	uint8_t        isApertureSpads;
	uint8_t        VhvSettings;
	uint8_t        PhaseCal;

	if ((status = VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) != VL53L0X_ERROR_NONE) // Setup in single ranging mode
		return status;
	if ((status = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.25 * 65536))) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(18 * 65536))) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, 200000)) != VL53L0X_ERROR_NONE)
		return status;

	for (int i = 0; i < secs && (status == VL53L0X_ERROR_NONE); i++) {
		Log_Debug("Measurement %d; ", i);
		if ((status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE)
			print_pal_error(status, __LINE__);
		print_range_status(&RangingMeasurementData);
		VL53L0X_GetLimitCheckCurrent(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent);
		Log_Debug(" RANGE IGNORE THRESHOLD: %.6f - ", (float)LimitCheckCurrent / 65536.0);
		Log_Debug("Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
		sleep_1sec();
	}
	return status;
}

VL53L0X_Error HighSpeed(VL53L0X_Dev_t *pMyDevice, int secs)
{
	VL53L0X_RangingMeasurementData_t RangingMeasurementData;
	FixPoint1616_t LimitCheckCurrent;
	VL53L0X_Error  status;
	uint32_t       refSpadCount;
	uint8_t        isApertureSpads;
	uint8_t        VhvSettings;
	uint8_t        PhaseCal;

	if ((status = VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) != VL53L0X_ERROR_NONE) // Setup in single ranging mode
		return status;
	if ((status = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.25 * 65536))) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(32 * 65536))) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, 30000)) != VL53L0X_ERROR_NONE)
		return status;

	for (int i = 0; i < secs && (status == VL53L0X_ERROR_NONE); i++) {
		Log_Debug("Measurement %d; ", i);
		if ((status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE)
			print_pal_error(status, __LINE__);
		print_range_status(&RangingMeasurementData);
		VL53L0X_GetLimitCheckCurrent(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent);
		Log_Debug(" RANGE IGNORE THRESHOLD: %.6f - ", (float)LimitCheckCurrent / 65536.0);
		Log_Debug("Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
		sleep_1sec();
	}
	return status;
}


VL53L0X_Error LongRange(VL53L0X_Dev_t *pMyDevice, int secs)
{
	VL53L0X_RangingMeasurementData_t RangingMeasurementData;
	VL53L0X_Error status;
	uint32_t      refSpadCount;
	uint8_t       isApertureSpads;
	uint8_t       VhvSettings;
	uint8_t       PhaseCal;

	if ((status = VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) != VL53L0X_ERROR_NONE) //single ranging mode
		return status;
	if ((status = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.1 * 65536))) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(60 * 65536))) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, 33000)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetVcselPulsePeriod(pMyDevice, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18)) != VL53L0X_ERROR_NONE)
		return status;
	if ((status = VL53L0X_SetVcselPulsePeriod(pMyDevice, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14)) != VL53L0X_ERROR_NONE)
		return status;

	for (int i = 0; i < secs && (status == VL53L0X_ERROR_NONE); i++) {
		Log_Debug("Measurement %d; ", i);
		if ((status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE)
			print_pal_error(status,__LINE__);
		print_range_status(&RangingMeasurementData);
		Log_Debug(" Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
		sleep_1sec();
	}
	return status;
}

void usage(void)
{
	Log_Debug(" Select the test for 'lightranger_demo' to run at startup:\n");
	Log_Debug(" -s X: Set the number of seconds to run the test\n");
	Log_Debug(" -1  : Run the Continuous Ranging test\n");
	Log_Debug(" -2  : Run the Single Ranging test\n");
	Log_Debug(" -3  : Run the High Accuracy test\n");
	Log_Debug(" -4  : Run the High Speed test\n");
	Log_Debug(" -5  : Run the Long Range test\n");
	Log_Debug(" -?  : Display usage info\n");
}

int main(int argc, char *argv[])
{
	int                  i, test_run = 1;//default to Continuous Ranging test
	int                  test_time = 30;  //default to 30 seconds
	VL53L0X_Error        Status;
	VL53L0X_Dev_t        MyDevice;
	VL53L0X_Version_t    Version;
	VL53L0X_DeviceInfo_t DeviceInfo;

	while ((i = getopt(argc, argv, "12345s:?")) != -1)
		switch (i) {
		case 's':
			sscanf(optarg, "%d", &test_time);
			Log_Debug(">> running for %d seconds ", test_time);
			break;
		case '1':
			Log_Debug(">> Run 'Continuous Ranging test'\n");
			test_run = 1;
			break;
		case '2':
			Log_Debug(">> Run 'Single Ranging test'\n");
			test_run = 2;
			break;
		case '3':
			Log_Debug(">> Run 'High Accuracy test'\n");
			test_run = 3;
			break;
		case '4':
			Log_Debug(">> Run 'High Speed test'\n");
			test_run = 4;
			break;
		case '5':
			Log_Debug(">> Run 'Long Range test'\n");
			test_run = 5;
			break;
		case '?':
			usage();
			exit(EXIT_SUCCESS);
		default:
			Log_Debug(">> nknown option character `\\x%x'.\n", optopt);
			exit(EXIT_FAILURE);
		}

	Log_Debug("\n\n");
	Log_Debug("     ****\r\n");
	Log_Debug("    **  **     LightRanger Click SW reuse using C example\r\n");
	Log_Debug("   **    **    \r\n");
	Log_Debug("  ** ==== **\r\n");
	Log_Debug("\r\n");

	//
	// Initialize the I2C interface
	//
	if (VL53L0X_i2c_init(&MyDevice) != VL53L0X_ERROR_NONE)
		exit(1);

	if ((Status = VL53L0X_DataInit(&MyDevice)) != VL53L0X_ERROR_NONE) {
		print_pal_error(Status,__LINE__);
		exit(1);
	}

	//
	// Get version of the VL53L0X API running in the firmware
	//
	if (VL53L0X_GetVersion(&Version) != 0)
		exit(1);

	if (VL53L0X_GetDeviceInfo(&MyDevice, &DeviceInfo) != VL53L0X_ERROR_NONE)
		exit(1);

	Log_Debug("\n\nVL53L0X_GetDeviceInfo:\n");
	Log_Debug("Device Name          : %s\n", DeviceInfo.Name);
	Log_Debug("Device Type          : %s\n", DeviceInfo.Type);
	Log_Debug("Device ID            : %s\n", DeviceInfo.ProductId);
	Log_Debug("ProductRevisionMajor : %d\n", DeviceInfo.ProductRevisionMajor);
	Log_Debug("ProductRevisionMinor : %d\n\n", DeviceInfo.ProductRevisionMinor);

	switch (test_run) {
	case 1:
		Log_Debug(">> Run 'Continuous Ranging test for %d seconds'\n", test_time);
		Status = rangingTest(&MyDevice, test_time);
		break;
	case 2:
		Log_Debug(">> Run 'Single Ranging test' for %d seconds\n", test_time);
		Status = SingleRanging(&MyDevice, test_time);
		break;
	case 3:
		Log_Debug(">> Run 'High Accuracy test' for %d seconds\n", test_time);
		Status = HighAccuracy(&MyDevice, test_time);
		break;
	case 4:
		Log_Debug(">> Run 'High Speed test' for %d seconds\n", test_time);
		Status = HighSpeed(&MyDevice, test_time);
		break;
	case 5:
		Log_Debug(">> Run 'Long Range test' for %d seconds\n", test_time);
		Status = LongRange(&MyDevice, test_time);
		break;
	}

	if (Status != VL53L0X_ERROR_NONE)
		print_pal_error(Status,__LINE__);
	VL53L0X_i2c_close(&MyDevice);
	Log_Debug("DONE...\n");
	exit(EXIT_SUCCESS);
}


