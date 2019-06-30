
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

// The specific GPIO being used must be setup and replaced thru
// this code.  The GPIO of 240 is in the path of most the sys dirs
// and in the export write.
//
// Figuring out the exact GPIO was not totally obvious when there
// were multiple GPIOs in the system. One way to do is to go into
// the gpiochips in /sys/class/gpio and view the label as it should
// reflect the address of the GPIO in the system. The name of the
// the chip appears to be the 1st GPIO of the controller.
//
// The export causes the gpio422 dir to appear in /sys/class/gpio.
// Then the direction and value can be changed by writing to them.

// The performance of this is pretty good, using a nfs mount,
// running on open source linux, on the ML507 reference system,
// the GPIO can be toggled about every 4 usec.

// The Ultra96 uses a ZynqMP, so there are 78 GPIO signals for device pins
// and 288 GPIO signals for PS and PL pins through the EMIO interface.
//
// to caculate the export base for a GPIO depends on if it is a device pin or
// through the EMIO interface. for Device Pins (HD_GPIO_15 for example):
// HD_GPIO_15 == EMIO[15] == GPIO_BASE+78+15 == 431 so echo 431>export
//
// and for PL/PS pins (MIO40_PS_GPIO1_3 for example):
// GPIO_BASE + 40 + 3 == 338+40+3 == 381
// MIO40 == GPIO_BASE + 40 == 338 + 40 == 378
//
// The following commands from the console setup the GPIO to be
// exported, set the direction of it to an output and write a 1
// to the GPIO.
//
// bash> echo 378 > /sys/class/gpio/export
// bash> echo out > /sys/class/gpio/gpio378/direction
// bash> echo 1 > /sys/class/gpio/gpio378/value
// if sysfs is not mounted on your system, the you need to mount it
// bash> mount -t sysfs sysfs /sys

// the following bash script to toggle the gpio is also handy for
// testing
//
// while [ 1 ]; do
//  echo 1 > /sys/class/gpio/gpio422/value
//  echo 0 > /sys/class/gpio/gpio422/value
// done

// to compile this, use the following command
// gcc gpio.c -o gpio

// The kernel needs the following configuration to make this work.
//
// CONFIG_GPIO_SYSFS=y
// CONFIG_SYSFS=y
// CONFIG_EXPERIMENTAL=y
// CONFIG_GPIO_XILINX=y

int main(int argc, char *argv[])
{
    int thegpio, write_test =0, i, val, valuefd, exportfd, directionfd;
    int close_when_done = 1;
    char buf[80];

    thegpio = 342;
    while((i=getopt(argc,argv,"cwv:r:")) != -1 )
        switch(i) {
            case 'c':
		close_when_done = 0;
                printf(">> don't close when done\n");
                break;
            case 'w':
		write_test = 1;
                break;
            case 'v':
                sscanf(optarg,"%d",&val);
                printf(">> write %d\n",val);
                break;
            case 'r':
                sscanf(optarg,"%d",&thegpio);
                printf(">> use gpio %d\n",thegpio);
                break;
            default:
               fprintf (stderr, ">> nknown option character `\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
            }

    sprintf(buf,"%d",thegpio);
    printf("GPIO test using %s...\n",buf);

    // The GPIO has to be exported to be able to see it
    // in sysfs

    exportfd = open("/sys/class/gpio/export", O_WRONLY);
    if (exportfd < 0) {
        printf("Cannot open GPIO to export it\n");
        exit(1);
        }

    i = write(exportfd, buf, strlen(buf));
    close(exportfd);

    printf("GPIO exported...(i=%d)\n",i);

    // Update the direction of the GPIO to be an output

    sprintf(buf,"/sys/class/gpio/gpio%d/direction",thegpio);
    printf("opening %s\n",buf);
    directionfd = open(buf, O_RDWR);
    if (directionfd < 0) {
        printf("Cannot open GPIO direction to set...\n");
        exit(1);
        }
    else
        printf("Opened GPIO direction to set...\n");

    if( write_test )
        i=write(directionfd, "out", 4);
    else
        i=write(directionfd, "in", 3);

    close(directionfd);

    printf("GPIO direction set (i=%d)\n",i);

    // Get the GPIO value ready to be toggled

    sprintf(buf,"/sys/class/gpio/gpio%d/value",thegpio);
    valuefd = open(buf, O_RDWR);
    if (valuefd < 0) {
        printf("Cannot open GPIO value\n");
        exit(1);
        }
    if( write_test )
        write(valuefd, &val, 1);
    else{
        read(valuefd, &val, 1);
        printf("read %d\n",val);
        }

    if( close_when_done ) {
        printf("sleep for 20 seconds then close...\n");
        sleep(20);

        sprintf(buf,"%d",thegpio);
        exportfd = open("/sys/class/gpio/unexport", O_WRONLY);
        if (exportfd < 0) {
            printf("Unable to open unexport gpio\n");
            }

        i = write(exportfd, buf, strlen(buf));
        close(exportfd);
        }
}



