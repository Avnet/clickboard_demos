#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"

#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

void print_pal_error(VL53L0X_Error Status)
{
    char buf[VL53L0X_MAX_STRING_LENGTH];
    VL53L0X_GetPalErrorString(Status, buf);
    printf("API Status: %i : %s\n", Status, buf);
}

void print_range_status(VL53L0X_RangingMeasurementData_t* pRangingMeasurementData)
{
    char buf[VL53L0X_MAX_STRING_LENGTH];
    uint8_t RangeStatus;

    RangeStatus = pRangingMeasurementData->RangeStatus;

    VL53L0X_GetRangeStatusString(RangeStatus, buf);
    printf(" Range Status: %i : %s, ", RangeStatus, buf);
}

VL53L0X_Error WaitMeasurementDataReady(VL53L0X_DEV Dev) 
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t       NewDatReady=0;
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
    uint32_t StopCompleted=0;
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

    if( (Status=VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE )
        return Status;
    
    if( (Status=VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE)
        return Status;

    if( (Status = VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE)
        return Status;

    if( (Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING)) != VL53L0X_ERROR_NONE)
        return Status;
    
    if( (Status = VL53L0X_StartMeasurement(pMyDevice)) != VL53L0X_ERROR_NONE)
        return Status;

    for(int i=0; i<secs && (Status == VL53L0X_ERROR_NONE); i++){
        if( (Status = WaitMeasurementDataReady(pMyDevice)) == VL53L0X_ERROR_NONE) {
            Status = VL53L0X_GetRangingMeasurementData(pMyDevice, pRangingMeasurementData);
            printf("measurement (%d): %6d (mm)\n", i+1, pRangingMeasurementData->RangeMilliMeter);
            VL53L0X_ClearInterruptMask(pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
            VL53L0X_PollingDelay(pMyDevice);
            sleep(1);
            } 
        }

    if(Status != VL53L0X_ERROR_NONE)
        return Status;

    if( (Status = VL53L0X_StopMeasurement(pMyDevice)) != VL53L0X_ERROR_NONE)
        return Status;

    if( (Status = WaitStopCompleted(pMyDevice)) != VL53L0X_ERROR_NONE)
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

    if( (stat=VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE ) 
        return stat;
    if( (stat=VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) !=VL53L0X_ERROR_NONE )
        return stat;
    if( (stat=VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) !=VL53L0X_ERROR_NONE )
        return stat;
    if( (stat=VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) !=VL53L0X_ERROR_NONE )
        return stat;
    if( (stat=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) !=VL53L0X_ERROR_NONE )
        return stat;
    if( (stat=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) !=VL53L0X_ERROR_NONE )
        return stat;
    if( (stat=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1)) !=VL53L0X_ERROR_NONE )
        return stat;
    if( (stat=VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, (FixPoint1616_t)(1.5*0.023*65536))) !=VL53L0X_ERROR_NONE )
        return stat;

    for(int i=0; i<secs && (stat == VL53L0X_ERROR_NONE);i++){
        printf("Measurement %d; ",i);
        if( (stat=VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE )
            print_pal_error(stat);
        print_range_status(&RangingMeasurementData);
        VL53L0X_GetLimitCheckCurrent(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent);
        printf(" RANGE IGNORE THRESHOLD: %.6f - ", (float)LimitCheckCurrent/65536.0);
        printf("Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
        sleep(1);
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

    if( (status=VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) != VL53L0X_ERROR_NONE ) // Setup in single ranging mode
        return status;
    if( (status=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.25*65536))) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(18*65536))) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, 200000)) != VL53L0X_ERROR_NONE )
        return status;

    for(int i=0; i<secs && (status == VL53L0X_ERROR_NONE);i++){
        printf("Measurement %d; ",i);
        if( (status=VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE)
            print_pal_error(status);
        print_range_status(&RangingMeasurementData);
        VL53L0X_GetLimitCheckCurrent(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent);
        printf(" RANGE IGNORE THRESHOLD: %.6f - ", (float)LimitCheckCurrent/65536.0);
        printf("Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
        sleep(1);
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

    if( (status=VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) != VL53L0X_ERROR_NONE ) // Setup in single ranging mode
        return status;
    if( (status=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.25*65536))) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(32*65536))) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, 30000)) != VL53L0X_ERROR_NONE )
        return status;
	
    for(int i=0; i<secs && (status == VL53L0X_ERROR_NONE);i++){
        printf("Measurement %d; ",i);
        if( (status=VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE )
            print_pal_error(status);
        print_range_status(&RangingMeasurementData);
        VL53L0X_GetLimitCheckCurrent(pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent);
        printf(" RANGE IGNORE THRESHOLD: %.6f - ", (float)LimitCheckCurrent/65536.0);
        printf("Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
        sleep(1);
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

    if( (status=VL53L0X_StaticInit(pMyDevice)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_PerformRefCalibration(pMyDevice, &VhvSettings, &PhaseCal)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount, &isApertureSpads)) != VL53L0X_ERROR_NONE ) 
        return status;
    if( (status=VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING)) != VL53L0X_ERROR_NONE ) //single ranging mode
        return status;
    if( (status=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckEnable(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.1*65536))) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetLimitCheckValue(pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(60*65536))) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, 33000)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetVcselPulsePeriod(pMyDevice, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18)) != VL53L0X_ERROR_NONE )
        return status;
    if( (status=VL53L0X_SetVcselPulsePeriod(pMyDevice, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14)) != VL53L0X_ERROR_NONE )
        return status;

    for(int i=0; i<secs && (status == VL53L0X_ERROR_NONE);i++){
        printf("Measurement %d; ",i);
        if( (status=VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData)) != VL53L0X_ERROR_NONE )
            print_pal_error(status);
        print_range_status(&RangingMeasurementData);
        printf(" Measured distance: %i\n", RangingMeasurementData.RangeMilliMeter);
        sleep(1);
        }
    return status;
}

void usage (void)
{
    printf(" Select the test for 'lightranger_demo' to run at startup:\n");
    printf(" -s X: Set the number of seconds to run the test\n");
    printf(" -1  : Run the Continuous Ranging test\n");
    printf(" -2  : Run the Single Ranging test\n");
    printf(" -3  : Run the High Accuracy test\n");
    printf(" -4  : Run the High Speed test\n");
    printf(" -5  : Run the Long Range test\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int                  i, test_run =1;//default to Continuous Ranging test
    int                  test_time=30;  //default to 30 seconds
    VL53L0X_Error        Status;
    VL53L0X_Dev_t        MyDevice;
    VL53L0X_Version_t    Version;
    VL53L0X_DeviceInfo_t DeviceInfo;

    while((i=getopt(argc,argv,"12345s:?")) != -1 )
        switch(i) {
           case 's':
               sscanf(optarg,"%d",&test_time);
               printf(">> running for %d seconds ",test_time);
               break;
           case '1':
               printf(">> Run 'Continuous Ranging test'\n");
               test_run=1;
               break;
           case '2':
               printf(">> Run 'Single Ranging test'\n");
               test_run=2;
               break;
           case '3':
               printf(">> Run 'High Accuracy test'\n");
               test_run=3;
               break;
           case '4':
               printf(">> Run 'High Speed test'\n");
               test_run=4;
               break;
           case '5':
               printf(">> Run 'Long Range test'\n");
               test_run=5;
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> nknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     LightRanger Click SW reuse using C example\r\n");
    printf("   **    **    \r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

//
// Initialize the I2C interface
//
    if( VL53L0X_i2c_init(&MyDevice) != VL53L0X_ERROR_NONE)
        exit(1);

    if( (Status=VL53L0X_DataInit(&MyDevice)) != VL53L0X_ERROR_NONE) {
        print_pal_error(Status);
        exit(1);
        }
    
//
// Get version of the VL53L0X API running in the firmware
//
    if( VL53L0X_GetVersion(&Version) != 0 ) 
        exit(1);

    if( VL53L0X_GetDeviceInfo(&MyDevice, &DeviceInfo) != VL53L0X_ERROR_NONE ) 
        exit(1);

    printf("\n\nVL53L0X_GetDeviceInfo:\n");
    printf("Device Name          : %s\n", DeviceInfo.Name);
    printf("Device Type          : %s\n", DeviceInfo.Type);
    printf("Device ID            : %s\n", DeviceInfo.ProductId);
    printf("ProductRevisionMajor : %d\n", DeviceInfo.ProductRevisionMajor);
    printf("ProductRevisionMinor : %d\n\n", DeviceInfo.ProductRevisionMinor);

    switch(test_run) {
        case 1:
            printf(">> Run 'Continuous Ranging test for %d seconds'\n",test_time);
            Status=rangingTest(&MyDevice,test_time);
            break;
        case 2:
            printf(">> Run 'Single Ranging test' for %d seconds\n",test_time);
            Status=SingleRanging(&MyDevice,test_time);
            break;
        case 3:
            printf(">> Run 'High Accuracy test' for %d seconds\n",test_time);
            Status=HighAccuracy(&MyDevice,test_time);
            break;
        case 4:
            printf(">> Run 'High Speed test' for %d seconds\n",test_time);
            Status=HighSpeed(&MyDevice,test_time);
            break;
        case 5:
            printf(">> Run 'Long Range test' for %d seconds\n",test_time);
            Status=LongRange(&MyDevice,test_time);
            break;
        }

    if( Status != VL53L0X_ERROR_NONE) 
        print_pal_error(Status);
    VL53L0X_i2c_close(&MyDevice);
    printf("DONE...\n");
    exit(EXIT_SUCCESS);
}



