//
//  testSystem.c
//  yarg
//
//  Created by dlm on 31/10/2025.
//

#include "testSystem.h"

#include "testIntrinsics.h"
#include "object.h"
#include "memory.h"

#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

enum TsExpectedType
{
    TsExpectedRead,
    TsExpectedReadAny,
    TsExpectedWrite,
    TsExpectedWriteAny
};

typedef struct TsExpected {
    int n_;
    int e_;
    struct TsExpectedItem {
        enum TsExpectedType operation_;
        uint32_t address_, value_;
    } *i_;
} TsExpected;

typedef struct TsMemory {
    int n_;
    int e_;
    struct TsMemoryItem {
        uint32_t address_, value_;
    } *i_;
} TsMemory;

typedef struct TsInterruptHandlers {
    int n_;
    int e_;
    struct TsInterruptHandlersItem {
        uint32_t id_;
        void (*isr_)(void);
    } *i_;
} TsInterruptHandlers;

typedef struct TsInterrupts {
    int n_;
    int e_;
    pthread_t *i_;
} TsInterrupts;

typedef struct TestSystem
{
    pthread_mutex_t simulateInterruptsMutex_;
    pthread_cond_t simulateInterrupts_;
    bool simulateInterruptsNow_;
    pthread_mutex_t expectedMutex_;
    TsExpected expected_; // address, value - value == 0 if WriteAny or ReadAny
    pthread_mutex_t memoryMutex_;
    TsMemory memory_; // address, value
    pthread_mutex_t interruptHandlersMutex_;
    TsInterruptHandlers interruptHandlers_; // num, address
    pthread_mutex_t interruptsMutex_;
    TsInterrupts interrupts_; // interrupt id
    pthread_mutex_t logMutex_;
    TsLog log_;
} TestSystem;

static void *simulateInterrupt(void *); // thread entry

static TestSystem *self(void);
static void extend(void *, size_t);

static uint32_t read(uint32_t address);
static void write(uint32_t address, uint32_t value);
static void addInterruptHandler(uint32_t intId, void (*address)(void));
static void removeInterruptHandler(uint32_t intId, void (*address)(void));
static void log(char const *, ...);

struct { char *name; int number_; } lut[] =
{
    {"TIMER_IRQ_0", 0},
    {"TIMER_IRQ_1", 1},
    {"TIMER_IRQ_2", 2},
    {"TIMER_IRQ_3", 3},
    {"PWM_IRQ_WRAP", 4},
    {"USBCTRL_IRQ", 5},
    {"XIP_IRQ", 6},
    {"PIO0_IRQ_0", 7},
    {"PIO0_IRQ_1", 8},
    {"PIO1_IRQ_0", 9},
    {"PIO1_IRQ_1", 10},
    {"DMA_IRQ_0", 11},
    {"DMA_IRQ_1", 12},
    {"IO_IRQ_BANK0", 13},
    {"IO_IRQ_QSPI", 14},
    {"SIO_IRQ_PROC0", 15},
    {"SIO_IRQ_PROC1", 16},
    {"CLOCKS_IRQ", 17},
    {"SPI0_IRQ", 18},
    {"SPI1_IRQ", 19},
    {"UART0_IRQ", 20},
    {"UART1_IRQ", 21},
    {"ADC0_IRQ_FIFO", 22},
    {"I2C0_IRQ", 23},
    {"I2C1_IRQ", 24},
    {"RTC_IRQ", 24},
    {"TIMER0_IRQ_0", 0},
    {"TIMER0_IRQ_1", 1},
    {"TIMER0_IRQ_2", 2},
    {"TIMER0_IRQ_3", 3},
    {"TIMER1_IRQ_0", 4},
    {"TIMER1_IRQ_1", 5},
    {"TIMER1_IRQ_2", 6},
    {"TIMER1_IRQ_3", 7},
    {"PWM_IRQ_WRAP_0", 8},
    {"PWM_IRQ_WRAP_1", 9},
    {"2350_DMA_IRQ_0", 10},
    {"2350_DMA_IRQ_1", 11},
    {"DMA_IRQ_2", 12},
    {"DMA_IRQ_3", 13},
    {"2350_USBCTRL_IRQ", 14},
    {"2350_PIO0_IRQ_0", 15},
    {"2350_PIO0_IRQ_1", 16},
    {"2350_PIO1_IRQ_0", 17},
    {"2350_PIO1_IRQ_1", 18},
    {"PIO2_IRQ_0", 19},
    {"PIO2_IRQ_1", 20},
    {"2350_IO_IRQ_BANK0", 21},
    {"IO_IRQ_BANK0_NS", 22},
    {"2350_IO_IRQ_QSPI", 23},
    {"IO_IRQ_QSPI_NS", 24},
    {"SIO_IRQ_FIFO", 25},
    {"SIO_IRQ_BELL", 26},
    {"SIO_IRQ_FIFO_NS", 27},
    {"SIO_IRQ_BELL_NS", 28},
    {"SIO_IRQ_MTIMECMP", 29},
    {"2350_CLOCKS_IRQ", 30},
    {"2350_SPI0_IRQ", 31},
    {"2350_SPI1_IRQ", 32},
    {"2350_UART0_IRQ", 33},
    {"2350_UART1_IRQ", 34},
    {"ADC_IRQ_FIFO", 35},
    {"2350_I2C0_IRQ", 36},
    {"2350_I2C1_IRQ", 37},
    {"OTP_IRQ", 38},
    {"TRNG_IRQ", 39},
    {"PROC0_IRQ_CTI", 40},
    {"PROC1_IRQ_CTI", 41},
    {"PLL_SYS_IRQ", 42},
    {"PLL_USB_IRQ", 43},
    {"POWMAN_IRQ_POW", 44},
    {"POWMAN_IRQ_TIMER", 45},
    {"SPAREIRQ_IRQ_0", 46},
    {"SPAREIRQ_IRQ_1", 47},
    {"SPAREIRQ_IRQ_2", 48},
    {"SPAREIRQ_IRQ_3", 49},
    {"SPAREIRQ_IRQ_4", 50},
    {"SPAREIRQ_IRQ_5", 51}
};

TestSystem *self(void)
{
    // todo: use    int pthread_once(pthread_once_t *, void (* _Nonnull)(void));
    volatile static TestSystem ts;
    return (TestSystem *)&ts;
}


uint32_t tsRead(uint32_t address)
{
    return read(address);
}

void tsWrite(uint32_t address, uint32_t value)
{
    write(address, value);
}

void tsAddInterruptHandler(uint32_t intId, void (*address)(void))
{
    addInterruptHandler(intId, address);
}

void tsRemoveInterruptHandler(uint32_t intId, void (*address)(void))
{
    removeInterruptHandler(intId, address);
}

static void destroy(void) // never gets called - call from GC?
{
    TsInterrupts *tsi = &self()->interrupts_;
    for (int i = 0; i < tsi->n_; i++)
    {
        pthread_t *th = &tsi->i_[i];
        (void) pthread_detach(*th);
    }
    // todo: free all the collections
}

void *simulateInterrupt(void *idp) // thread entry
{
    uint32_t id = *(uint32_t *)idp;
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->simulateInterruptsMutex_);
        while (!ts->simulateInterruptsNow_)
        {
            pthread_cond_wait(&ts->simulateInterrupts_, &ts->simulateInterruptsMutex_); // unlocks then reaquires simulateInterruptsMutex_
        }
        pthread_mutex_unlock(&ts->simulateInterruptsMutex_);
    }
    
    void (*foundIsr)() = 0;
    {
        pthread_mutex_lock(&ts->interruptHandlersMutex_);
        for (int i = 0; i < ts->interruptHandlers_.n_; i++)
        {
            if (ts->interruptHandlers_.i_[i].id_ == id)
            {
                foundIsr = ts->interruptHandlers_.i_[i].isr_;
                break;
            }
        }
        pthread_mutex_unlock(&ts->interruptHandlersMutex_);
    }
    if (foundIsr != 0)
    {
        (*foundIsr)();
    }
    else
    {
        log("missing irq_add_shared_handler(%u, , );", id);
    }
    return 0;
}

// system under test interface
uint32_t read(uint32_t address)
{
    TestSystem *ts = self();

    bool writtenOrSet = false;
    uint32_t valueWrittenOrSet;
    {
        pthread_mutex_lock(&ts->memoryMutex_);
        for (int i = 0; i < ts->memory_.n_; i++)
        {
            if (ts->memory_.i_[i].address_ == address)
            {
                writtenOrSet = true;
                valueWrittenOrSet = ts->memory_.i_[i].value_;
                break;
            }
        }
        pthread_mutex_unlock(&ts->memoryMutex_);
    }
    if (writtenOrSet)
    {
        bool read = false;
        int i = 0;
        {
            pthread_mutex_lock(&ts->expectedMutex_);
            for (; i < ts->expected_.n_; i++)
            {
                if (ts->expected_.i_[i].operation_ == TsExpectedRead &&
                    ts->expected_.i_[i].address_ == address &&
                    ts->expected_.i_[i].value_ == valueWrittenOrSet)
                {
                    read = true;
                    break;
                }
            }
            if (read)
            {
                for (i++; i < ts->expected_.n_; i++)
                {
                    ts->expected_.i_[i - 1] = ts->expected_.i_[i];
                }
                ts->expected_.n_--;
            }
            pthread_mutex_unlock(&ts->expectedMutex_);
        }
        if (!read)
        {
            i = 0;
            {
                pthread_mutex_lock(&ts->expectedMutex_);
                for (; i < ts->expected_.n_; i++)
                {
                    if (ts->expected_.i_[i].operation_ == TsExpectedReadAny && ts->expected_.i_[i].address_ == address)
                    {
                        read = true;
                        break;
                    }
                }
                if (read)
                {
                    for (i++; i < ts->expected_.n_; i++)
                    {
                        ts->expected_.i_[i - 1] = ts->expected_.i_[i];
                    }
                    ts->expected_.n_--;
                }
                pthread_mutex_unlock(&ts->expectedMutex_);
            }
        }
        if (!read)
        {
            log("missing test_read(0x%06x);", address);
        }
    }
    else
    {
        log("missing poke/test_set 0x%06x", address);
        valueWrittenOrSet = 0xa5a5a5u;
    }
    return valueWrittenOrSet;
}
    
void write(uint32_t address, uint32_t value)
{
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->memoryMutex_);
        int i = 0;
        for (; i < ts->memory_.n_; i++)
        {
            if (ts->memory_.i_[i].address_ == address)
            {
                ts->memory_.i_[i].value_ = value;
                break;
            }
        }
        assert(i == ts->memory_.n_); // create new memory cell
        pthread_mutex_unlock(&ts->memoryMutex_);
    }

    bool written = false;
    int i = 0;
    {
        pthread_mutex_lock(&ts->expectedMutex_);
        for (; i < ts->expected_.n_; i++)
        {
            if (ts->expected_.i_[i].operation_ == TsExpectedWrite &&
                ts->expected_.i_[i].address_ == address &&
                ts->expected_.i_[i].value_ == value)
            {
                written = true;
                break;
            }
        }
        if (written)
        {
            for (i++; i < ts->expected_.n_; i++)
            {
                ts->expected_.i_[i - 1] = ts->expected_.i_[i];
            }
            ts->expected_.n_--;
        }
        pthread_mutex_unlock(&ts->expectedMutex_);
    }
    if (!written)
    {
        i = 0;
        {
            pthread_mutex_lock(&ts->expectedMutex_);
            for (; i < ts->expected_.n_; i++)
            {
                if (ts->expected_.i_[i].operation_ == TsExpectedWriteAny && ts->expected_.i_[i].address_ == address)
                {
                    written = true;
                    break;
                }
            }
            if (written)
            {
                for (i++; i < ts->expected_.n_; i++)
                {
                    ts->expected_.i_[i - 1] = ts->expected_.i_[i];
                }
                ts->expected_.n_--;
            }
            pthread_mutex_unlock(&ts->expectedMutex_);
        }
    }
    if (!written)
    {
        log("missing test_write(0x%06x, %u);", address, value);
    }
}

void addInterruptHandler(uint32_t intId, void (*address)(void))
{
    TestSystem *ts = self();

    bool found = false;
    {
        pthread_mutex_unlock(&ts->interruptHandlersMutex_);
        for (int i = 0; i < ts->interruptHandlers_.n_; i++)
        {
            if (ts->interruptHandlers_.i_[i].id_ == intId && ts->interruptHandlers_.i_[i].isr_ == address)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            extend(&ts->interruptHandlers_, sizeof ts->interruptHandlers_.i_[0]);
            ts->interruptHandlers_.i_[ts->interruptHandlers_.n_++] = (struct TsInterruptHandlersItem){intId, address};
        }
        pthread_mutex_unlock(&ts->interruptHandlersMutex_);
    }
    if (found)
    {
        log("missing irq_remove_handler(%u, );", intId);
    }
}

void removeInterruptHandler(uint32_t intId, void (*address)(void))
{
    TestSystem *ts = self();

    bool added = false;
    {
        pthread_mutex_unlock(&ts->interruptHandlersMutex_);
        int i = 0;
        for (; i < ts->interruptHandlers_.n_; i++)
        {
            if (ts->interruptHandlers_.i_[i].id_ == intId && ts->interruptHandlers_.i_[i].isr_ == address)
            {
                added = true;
                break;
            }
        }
        if (added)
        {
            for (i++; i < ts->expected_.n_; i++)
            {
                ts->interruptHandlers_.i_[i - 1] = ts->interruptHandlers_.i_[i];
            }
            ts->interruptHandlers_.n_--;
        }
        pthread_mutex_unlock(&ts->interruptHandlersMutex_);
    }
    if (!added)
    {
        log("missing irq_add_shared_handler(%u, , );", intId);
    }
}

void log(char const *s, ...)
{
    va_list args;
    va_start(args, s);
    int len = vsnprintf((char *)0, 0, s, args);
    va_end(args);

    len++;

    char *newString = reallocate(0, 0, len);
    va_start(args, s);
    (void)vsnprintf(newString, len, s, args);
    va_end(args);

    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->logMutex_);
        extend(&ts->log_, sizeof ts->log_.i_[0]);
        ts->log_.i_[ts->log_.n_++] = newString;
        pthread_mutex_unlock(&ts->logMutex_);
    }
}

// test code interface
TsLog *testIntrinsicsSync(void)
{
    printf("Waiting for interrupts to be simulated - ");
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->simulateInterruptsMutex_);
        ts->simulateInterruptsNow_ = true;
        pthread_cond_broadcast(&ts->simulateInterrupts_);
        pthread_mutex_unlock(&ts->simulateInterruptsMutex_); // release lock here allows all simulateInterrupt to continue
    }

    for (int i = 0; i < ts->interrupts_.n_; i++)
    {
        pthread_join(ts->interrupts_.i_[i], 0);
    }
    printf("done\n");

    ts->interrupts_.n_ = 0;
    ts->simulateInterruptsNow_ = false;

    // todo: mutex
    int numUnfulfilledExpectations = ts->expected_.n_;
    if (numUnfulfilledExpectations != 0)
    {
        log("%d unfulfilled expectation%s:", numUnfulfilledExpectations, numUnfulfilledExpectations > 1 ? "s" : "");
        //    multiset<tuple<Expected, uint32_t, uint32_t> > expected; // address, value - value == 0 if WriteAny or ReadAny
        for (int i = 0; i < ts->expected_.n_; i++)
        {
            switch (ts->expected_.i_[i].operation_)
            {
            case TsExpectedRead:
                log("test_read(0x%06x, %u);", ts->expected_.i_[i].address_, ts->expected_.i_[i].value_);
                break;
            case TsExpectedReadAny:
                log("test_read(0x%06x);", ts->expected_.i_[i].address_);
                break;
            case TsExpectedWrite:
                log("test_write(0x%06x, %u);", ts->expected_.i_[i].address_, ts->expected_.i_[i].value_);
                break;
            case TsExpectedWriteAny:
                log("test_write(0x%06x);", ts->expected_.i_[i].address_);
                break;
            default:
                assert(!"expectations corrupt");
                break;
            }
        }
    }
    ts->expected_.n_ = 0;

    return &ts->log_; // log cleared by caller
}

void testIntrinsicsExpectRead(uint32_t address, uint32_t value)
{
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->expectedMutex_);
        extend(&ts->expected_, sizeof ts->expected_.i_[0]);
        ts->expected_.i_[ts->expected_.n_++] = (struct TsExpectedItem){TsExpectedRead, address, value};
        pthread_mutex_unlock(&ts->expectedMutex_);
    }
}

void TestIntrinsicsExpectReadAnyValue(uint32_t address)
{
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->expectedMutex_);
        extend(&ts->expected_, sizeof ts->expected_.i_[0]);
        ts->expected_.i_[ts->expected_.n_++] = (struct TsExpectedItem){TsExpectedReadAny, address, 0u};
        pthread_mutex_unlock(&ts->expectedMutex_);
    }
}

void testIntrinsicsExpectWrite(uint32_t address, uint32_t value)
{
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->expectedMutex_);
        extend(&ts->expected_, sizeof ts->expected_.i_[0]);
        ts->expected_.i_[ts->expected_.n_++] = (struct TsExpectedItem){TsExpectedWrite, address, value};
        pthread_mutex_unlock(&ts->expectedMutex_);
    }
}

void testIntrinsicsExpectWriteAnyValue(uint32_t address)
{
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->expectedMutex_);
        extend(&ts->expected_, sizeof ts->expected_.i_[0]);
        ts->expected_.i_[ts->expected_.n_++] = (struct TsExpectedItem){TsExpectedWriteAny, address, 0u};
        pthread_mutex_unlock(&ts->expectedMutex_);
    }
}

void testIntrinsicsSetMemory(uint32_t address, uint32_t value)
{
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->memoryMutex_);
        int i = 0;
        for (; i < ts->memory_.n_; i++)
        {
            if (ts->memory_.i_[i].address_ == address)
            {
                ts->memory_.i_[i].value_ = value;
                break;
            }
        }
        if (i == ts->memory_.n_)
        {
            extend(&ts->memory_, sizeof ts->memory_.i_[0]);
            ts->memory_.i_[ts->memory_.n_++] = (struct TsMemoryItem){address, value};
        }
        pthread_mutex_unlock(&ts->memoryMutex_);
    }
}

bool testIntrinsicsTriggerInterrupt(uint32_t intId)
{
    TestSystem *ts = self();
    {
        pthread_mutex_lock(&ts->interruptsMutex_);
        extend(&ts->interrupts_, sizeof ts->interrupts_.i_[0]);
        pthread_create(&ts->interrupts_.i_[ts->interrupts_.n_++], 0, simulateInterrupt, &intId);
        pthread_mutex_unlock(&ts->interruptsMutex_);
    }
    return true;
}

bool testIntrinsicsTriggerInterruptNamed(char const *s)
{
    TestSystem *ts = self();

    int i = 0;
    for (; i < sizeof lut / sizeof lut[0]; i++)
    {
        if (strcmp(lut[i].name, s) == 0)
        {
            testIntrinsicsTriggerInterrupt(lut[i].number_);
            break;
        }
    }
    if (i == sizeof lut / sizeof lut[0])
    {
        log("Unable to simulate an interrupt named %s", s);
        return false;
    }
    return true;
}
