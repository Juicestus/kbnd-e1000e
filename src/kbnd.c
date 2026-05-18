/**
 * kbnd.c - Kernel Bypass Network Driver
 *
 * Justus Languell  <jus@justusl.com>
 */

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "utils.h"

/**
 * Internal driver errors
 */
enum
{
    ERR_OK = 0,
    ERR_DEV_RST_TIMEOUT,

    __NUM_ERRORS      /* ALWAYS LAST */
};

static inline const char* error_str(int err)
{
    static const char* ERROR_STRS[__NUM_ERRORS] =
    {
        [ERR_OK] = "OK",
        [ERR_DEV_RST_TIMEOUT] = "Device Reset Timeout",
    };

    if (err < 0 || err >= __NUM_ERRORS)
        return "Unknown Error";
    return ERROR_STRS[err];
}


/**
 * PCI device information
 *
 * lpsci -vvv
 */
#define PCI_ADDR    "0000:00:03.0"
#define BAR0_SIZE   (128 * 1024)        // 128k



/**
 * Registers    10.2.1
 *
 * Note: RAL/RAH 0 holds primary MAC, device has 16
 */
#define REG_CTRL        0x00000     // RW
#define REG_STATUS      0x00008     // R
#define REG_CTRL_EXT    0x00018     // RW
#define REG_RAL0        0x05400     // RW   Low half
#define REG_RAH0        0x05404     // RW   High half
#define REG_IMC         0x000D8     // W    Interrupt Mask Clear
#define REG_ICR         0x000C0     // RW   Interrupt Cause Read

/**
 * Status bits          10.2.2.2
 */
#define STATUS_FD           (1u << 0)       // Full duplex
#define STATUS_LU           (1u << 1)       // Link up
#define STATUS_TXOFF        (1u << 4)       // TX paused

#define STATUS_SPEED_SHIFT  6                               // speed is bits 6:7
#define STATUS_SPEED_MASK   (0x3u << STATUS_SPEED_SHIFT)    // 0b00 = 10Mb
#define STATUS_SPEED_10     (0x0u << STATUS_SPEED_SHIFT)    // 0b01 = 100Mb
#define STATUS_SPEED_100    (0x1u << STATUS_SPEED_SHIFT)    // 0b10 = 1Gb
#define STATUS_SPEED_1000   (0x2u << STATUS_SPEED_SHIFT)

#define STATUS_PHYRA        (1u << 10)      // PHY Reset Asserted

/*
 * CTRL register bits   10.2.2.1
 */
#define CTRL_FD             (1u << 0)       // Full Duplex
#define CTRL_LRST           (1u << 3)       // Link Reset
#define CTRL_SLU            (1u << 6)       // Set Link Up
#define CTRL_FRCSPD         (1u << 11)      // Force Speed
#define CTRL_FRCDPLX        (1u << 12)      // Force Duplex
#define CTRL_RST            (1u << 26)      // Device Reset
#define CTRL_RFCE           (1u << 27)      // RX Flow Control Enable
#define CTRL_TFCE           (1u << 28)      // TX Flow Control Enable
#define CTRL_PHY_RST        (1u << 31)      // PHY Reset

#define SEE_FLAG(REG, MASK, T, F)   ((((REG) & (MASK)) != 0) ? (T) : (F))

/**
 * Safely read a register
 */
static inline uint32_t read32(volatile void* base, uint32_t offset)
{
    return *(volatile uint32_t*)((volatile uint8_t*)base + offset);
}

/**
 * Safely write a register
 */
static inline void write32(volatile void* base, uint32_t offset, uint32_t value)
{
    *(volatile uint32_t*)((volatile uint8_t*)base + offset) = value;
}


/**
 * Get the string representation for the current speed
 */
static inline const char* speed_str(uint32_t status)
{
    static const char* SPEED_STRS[] = { "10 Mbps", "100 Mbps", "1 Gbps", "N/A" };
    return SPEED_STRS[(status >> STATUS_SPEED_SHIFT) & 0b11];
}

#define MAC_ADDR(RAL, RAH)      (((uint64_t)(RAL)) | (((uint64_t)(RAH)) << 32))

static inline void mac_addr_str(char* mac_str, size_t str_size, uint64_t mac_addr)
{
    snprintf(mac_str, str_size, "%02x:%02x:%02x:%02x:%02x:%02x",
        (uint)((mac_addr >> 0x00)  & 0xff),
        (uint)((mac_addr >> 0x08)  & 0xff),
        (uint)((mac_addr >> 0x10)  & 0xff),
        (uint)((mac_addr >> 0x18)  & 0xff),
        (uint)((mac_addr >> 0x20)  & 0xff),
        (uint)((mac_addr >> 0x28)  & 0xff));
}


/**
 * Print device status info
 */
static int print_dev_status(volatile void* bar0)
{
    /**
     * Read core registers
     */
    uint32_t r_ctrl     = read32(bar0, REG_CTRL);
    uint32_t r_status   = read32(bar0, REG_STATUS);
    uint32_t r_ctrl_ext = read32(bar0, REG_CTRL_EXT);
    uint32_t ral        = read32(bar0, REG_RAL0);
    uint32_t rah        = read32(bar0, REG_RAH0);

    /**
     * Fmt mac addr
     */
    char mac_addr[256];
    mac_addr_str(mac_addr, sizeof(mac_addr), MAC_ADDR(ral, rah));

    /**
     * Print relevant device info
     */
    printf(
        "Registers\n"
        "\tCTRL:      0x%08x\n"
        "\tSTATUS:    0x%08x\n"
        "\tCTRL_EXT:  0x%08x\n"

        "Status\n"
        "\tLink:      %s\n"
        "\tDuplex:    %s\n"
        "\tSpeed:     %s\n"
        "\tTX:        %s\n"
        "\tReset:     %s\n"

        "Address\n"
        "\tMAC:       %s\n",

        r_ctrl, r_status, r_ctrl_ext,

        SEE_FLAG(r_status, STATUS_LU, "UP", "DOWN"),
        SEE_FLAG(r_status, STATUS_FD, "Full", "Half"),
        speed_str(r_status),
        SEE_FLAG(r_status, STATUS_TXOFF, "Off", "On"),
        SEE_FLAG(r_status, STATUS_PHYRA, "Assert", "Clear"),

        mac_addr);

    return ERR_OK;
}

/**
 * Reset the device
 */
static int dev_reset(volatile void* bar0)
{
    /**
     * Mask interrupts
     */
    write32(bar0, REG_IMC, UINT32_MAX);
    (void)read32(bar0, REG_ICR);

    /**
     * RST
     */
    write32(bar0, REG_CTRL, (read32(bar0, REG_CTRL) | CTRL_RST));

    /**
     * Wait for dev to come back on
     *
     * Poll every 100us
     * Timeout after 100*100us = 10ms
     */
    for (uint i = 0; i < 100; i++)
    {
        if ((read32(bar0, REG_CTRL) & CTRL_RST) == 0)
            break;
        usleep2(100);
    }

    /**
     * Failed
     */
    if (read32(bar0, REG_CTRL) & CTRL_RST)
    {
        return ERR_DEV_RST_TIMEOUT;
    }

    /**
     * OK
     */
    write32(bar0, REG_IMC, UINT32_MAX);
    (void)read32(bar0, REG_ICR);
    usleep2(1000);
    return ERR_OK;
}

/**
 * Just some tests right now
 */
int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    int rcode = 0;

    char path[256];
    int fd = -1;
    void* bar0 = MAP_FAILED;

    /**
     * Open BAR0
     */
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/resource0", PCI_ADDR);
    if ((fd = open(path, O_RDWR)) < 0)
    {
        fprintf(stderr, "Failed to open %s with %s (%d)\n", path, strerror(errno), errno);
        rcode = 1; goto _err;
    }
    printf("Opened %s OK %d\n", path, fd);


    /**
     * Map BAR0 into our addr space
     */
    if ((bar0 = mmap(NULL, BAR0_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "Failed to map bar0 with %s (%d)\n", strerror(errno), errno);
        rcode = 1; goto _err;
    }
    printf("Mapped bar0 at %p\n", bar0);



    /**
     * Reset device
     */
    printf("Reseting device... ");
    int err;
    if ((err = dev_reset(bar0)) != ERR_OK)
    {
        fprintf(stderr, "Failed to reset device with %s (%d)\n", error_str(err), err);
        rcode = 1; goto _err;
    }
    printf("OK\n");


    /**
     * Print info
     */
    print_dev_status(bar0);

_err:
    // unmap bar0
    if (bar0 != MAP_FAILED && munmap(bar0, BAR0_SIZE) < 0)
    {
        fprintf(stderr, "Failed to unmap bar0 with %s (%d)\n", strerror(errno), errno);
    }

    // cleanup fd
    if (fd >= 0)
        close(fd);

    return rcode;
}

