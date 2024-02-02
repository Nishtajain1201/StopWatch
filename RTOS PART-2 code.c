#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <time.h>

// Define GPIO pins and timer interval
#define GPIO_BUTTON_START_STOP 48
#define GPIO_BUTTON_RESET 49
#define GPIO_LED_RUNNING 117
#define GPIO_LED_PAUSED 115
#define TIMER_INTERVAL_MS 100 // Timer interval in milliseconds

bool stopwatchIsRunning = false; // Indicates if the stopwatch is running
unsigned int timeElapsedInMS = 0; // Elapsed time in milliseconds

// Mutex for thread synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void setGpioAsInput(int gpioPin);
int readGpioValue(int gpioPin);
void writeGpioValue(int gpioPin, int value);
void updateTimerDisplay();
void* stopwatchTimer(void* arg);
void* buttonMonitor(void* arg);

int main()
{
    pthread_t timerThread, buttonThread;

    // Create the timer and button monitor threads
    if (pthread_create(&timerThread, NULL, stopwatchTimer, NULL) != 0)
    {
        perror("Failed to create timer thread");
        return 1;
    }

    if (pthread_create(&buttonThread, NULL, buttonMonitor, NULL) != 0)
    {
        perror("Failed to create button monitor thread");
        return 1;
    }

    updateTimerDisplay(); // Display the initial timer value

    pthread_join(timerThread, NULL);
    pthread_join(buttonThread, NULL);

    return 0;
}

// Function to set the GPIO pin direction to input
void setGpioAsInput(int gpioPin)
{
    char bufferSize[64];
    snprintf(bufferSize, sizeof(bufferSize), "/sys/class/gpio/gpio%d/direction", gpioPin);
    int fd = open(bufferSize, O_WRONLY);
    if (fd != -1)
    {
        write(fd, "in", 2);
        close(fd);
    }
}

// Function to read the GPIO pin value
int readGpioValue(int gpioPin)
{
    char bufferSize[64];
    snprintf(bufferSize, sizeof(bufferSize), "/sys/class/gpio/gpio%d/value", gpioPin);
    int fd = open(bufferSize, O_RDONLY);
    if (fd != -1)
    {
        char value;
        read(fd, &value, 1);
        close(fd);
        return (value == '1') ? 1 : 0;
    }
    return -1; // Return -1 in case of an error
}

// Function to write a value to the GPIO pin
void writeGpioValue(int gpioPin, int value)
{
    char bufferSize[64];
    snprintf(bufferSize, sizeof(bufferSize), "/sys/class/gpio/gpio%d/value", gpioPin);
    int fd = open(bufferSize, O_WRONLY);
    if (fd != -1)
    {
        char val = (value == 0) ? '0' : '1';
        write(fd, &val, 1);
        close(fd);
    }
}

// Function to update and print the timer value
void updateTimerDisplay()
{
    // Convert timeElapsedInMS to seconds and milliseconds
    unsigned int elapsedSeconds = timeElapsedInMS / 1000;
    unsigned int elapsedMilliseconds = timeElapsedInMS % 1000;

    if (stopwatchIsRunning)
    {
        printf("\rElapsed Time: %u.%02u seconds", elapsedSeconds, elapsedMilliseconds / 10);
    }
    else
    {
        printf("\rElapsed Time (Paused): %u.%02u seconds", elapsedSeconds, elapsedMilliseconds / 10);
    }
    fflush(stdout);
}

// Stopwatch timer thread
void* stopwatchTimer(void* arg)
{
    struct timespec interval;
    interval.tv_sec = 0;
    interval.tv_nsec = TIMER_INTERVAL_MS * 1000000;

    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (stopwatchIsRunning)
        {
            timeElapsedInMS += TIMER_INTERVAL_MS;
            writeGpioValue(GPIO_LED_RUNNING, 1);
            writeGpioValue(GPIO_LED_PAUSED, 0);
        }
        else
        {
            writeGpioValue(GPIO_LED_RUNNING, 0);
            writeGpioValue(GPIO_LED_PAUSED, 1);
        }
        pthread_mutex_unlock(&mutex);

        updateTimerDisplay();

        nanosleep(&interval, NULL);
    }
}

// GPIO button monitoring thread
void* buttonMonitor(void* arg)
{
    setGpioAsInput(GPIO_BUTTON_START_STOP);
    setGpioAsInput(GPIO_BUTTON_RESET);

    int startStopPrev = 0;
    int resetPrev = 0;

    while (1)
    {
        int startStopButton = readGpioValue(GPIO_BUTTON_START_STOP);
        int resetButton = readGpioValue(GPIO_BUTTON_RESET);

        pthread_mutex_lock(&mutex);

        // Check if the start/stop button is pressed and toggle the stopwatch state
        if (startStopButton == 1 && startStopPrev == 0)
        {
            stopwatchIsRunning = !stopwatchIsRunning;
        }

        // Check if the reset button is pressed and reset the timer if the stopwatch is running
        if (resetButton == 1 && resetPrev == 0)
        {
            if (stopwatchIsRunning)
            {
                stopwatchIsRunning = false;
            }
            timeElapsedInMS = 0;
        }

        startStopPrev = startStopButton;
        resetPrev = resetButton;

        pthread_mutex_unlock(&mutex);
    }
}
