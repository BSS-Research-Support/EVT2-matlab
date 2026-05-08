/*
 * MEX gateway function for communicating with multiple HID devices via HIDAPI.
 *
 * Supported commands:
 *   - devs = evt2('list')
 *   - h = evt2('open', index)
 *   - evt2('write', h, value)
 *   - evt2('pulse', h, value, duration)
 *   - evt2('read', h, timeout_ms)
 *   - evt2('flush', h)
 *   - evt2('clear', h)
 *   - evt2('set', h)
 *   - evt2('close', h)
 */

#include <string.h>
#include "mex.h"
#include "evt2.h"

#if defined(_WIN32)
    #include "include/hidapi.h"
    #define STRICMP _stricmp
#else
    #include <hidapi/hidapi.h>
    #define STRICMP strcasecmp
#endif

#define MAX_HANDLES 16

/// Global HID device handles (1-based index for MATLAB)
static hid_device *handles[MAX_HANDLES] = {NULL};

static unsigned char hid_package[64] = {0};

// Helper to validate handle
static hid_device* get_handle(const mxArray *prhs) {
    if (mxIsEmpty(prhs)) return NULL;
    int h = (int)mxGetScalar(prhs);
    if (h < 1 || h > MAX_HANDLES) {
        mexErrMsgIdAndTxt("evt2:invalidHandle", "Handle must be between 1 and %d.", MAX_HANDLES);
    }
    if (handles[h-1] == NULL) {
        mexErrMsgIdAndTxt("evt2:deviceNotOpen", "Device handle %d is not open.", h);
    }
    return handles[h-1];
}

// Cleanup function called when the MEX file is cleared from memory
void cleanup(void) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (handles[i]) {
            hid_close(handles[i]);
            handles[i] = NULL;
        }
    }
    hid_exit();
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    unsigned char rx_buffer[64];
    #if defined(_WIN32)    
    unsigned char tx_buffer[65] = {0}; // Report ID ([0] must be zero on Windows systems)
    #else
    unsigned char tx_buffer[64];
    #endif

    static bool initialized = false;
    
    if (!initialized) {
        if (hid_init() != 0) {
            mexErrMsgTxt("Failed to initialize HIDAPI.");
        }
        mexAtExit(cleanup);
        initialized = true;
    }

    if (nrhs < 1 || !mxIsChar(prhs[0])) {
        mexErrMsgTxt("First argument must be a command string.");
    }

    char cmd[64];
    mxGetString(prhs[0], cmd, sizeof(cmd));

    // === LIST DEVICES ===
    if (STRICMP(cmd, "list") == 0) {
        struct hid_device_info *devs, *cur_dev;
        devs = hid_enumerate(0, 0);
        
        int count = 0;
        cur_dev = devs;
        while (cur_dev) {
            count++;
            cur_dev = cur_dev->next;
        }

        const char *fields[] = {"Index", "VendorID", "ProductID", "Path", "Product", "SerialNumber"};
        plhs[0] = mxCreateStructMatrix(1, count, 6, fields);

        cur_dev = devs;
        for (int i = 0; i < count; i++) {
            mxSetField(plhs[0], i, "Index", mxCreateDoubleScalar(i + 1));
            mxSetField(plhs[0], i, "VendorID", mxCreateDoubleScalar(cur_dev->vendor_id));
            mxSetField(plhs[0], i, "ProductID", mxCreateDoubleScalar(cur_dev->product_id));
            mxSetField(plhs[0], i, "Path", mxCreateString(cur_dev->path));
            
            char product[256] = "";
            if (cur_dev->product_string) {
                wcstombs(product, cur_dev->product_string, sizeof(product));
            }
            mxSetField(plhs[0], i, "Product", mxCreateString(product));

            char serial[256] = "";
            if (cur_dev->serial_number) {
                wcstombs(serial, cur_dev->serial_number, sizeof(serial));
            }
            mxSetField(plhs[0], i, "SerialNumber", mxCreateString(serial));
            
            cur_dev = cur_dev->next;
        }
        hid_free_enumeration(devs);
    }

    // === OPEN DEVICE ===
    else if (STRICMP(cmd, "open") == 0) {
        if (nrhs < 2) mexErrMsgTxt("Usage: h = evt2('open', index)");
        
        int index = (int)mxGetScalar(prhs[1]);
        struct hid_device_info *devs, *cur_dev;
        devs = hid_enumerate(0, 0);
        
        cur_dev = devs;
        for (int i = 1; i < index && cur_dev; i++) {
            cur_dev = cur_dev->next;
        }

        if (!cur_dev) {
            hid_free_enumeration(devs);
            mexErrMsgIdAndTxt("evt2:invalidIndex", "Device index %d not found.", index);
        }

        hid_device *dev = hid_open_path(cur_dev->path);
        hid_free_enumeration(devs);

        if (!dev) {
            mexErrMsgTxt("Failed to open HID device. Check permissions.");
        }

        int slot = -1;
        for (int i = 0; i < MAX_HANDLES; i++) {
            if (handles[i] == NULL) {
                slot = i;
                break;
            }
        }

        if (slot == -1) {
            hid_close(dev);
            mexErrMsgTxt("Too many open handles (max 16).");
        }

        handles[slot] = dev;
        hid_set_nonblocking(dev, 1);
        plhs[0] = mxCreateDoubleScalar(slot + 1);
    }

    // === WRITE TO DEVICE ===
    else if (STRICMP(cmd, "write") == 0) {
        if (nrhs < 3) mexErrMsgTxt("Usage: evt2('write', h, value)");
        hid_device *device = get_handle(prhs[1]);
        unsigned short value = (unsigned short) mxGetScalar(prhs[2]);
		
        hid_package[CMD_CODE] = CMD_WR_OP;
        hid_package[PARAM_1] = value;
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, 64);
        #else
        memcpy(&tx_buffer[0], hid_package, 64);
        #endif                
        
        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === READ FROM DEVICE ===
    else if (STRICMP(cmd, "read") == 0) {
        if (nrhs < 2) mexErrMsgTxt("Usage: data = evt2('read', h, [timeout_ms])");
        hid_device *device = get_handle(prhs[1]);

        int timeout = 1000; // Default 1s
        if (nrhs >= 3) {
            timeout = (int)mxGetScalar(prhs[2]);
            if (timeout < 0) timeout = 0;
            if (timeout > 10000) timeout = 10000; // Cap at 10s
        }

        int res = hid_read_timeout(device, rx_buffer, sizeof(rx_buffer), timeout);

        if (res < 0) mexErrMsgTxt("Read error.");

        plhs[0] = mxCreateNumericMatrix(1, res, mxUINT8_CLASS, mxREAL);
        if (res > 0) memcpy(mxGetData(plhs[0]), rx_buffer, res);
    }

    // === FLUSH READ BUFFER ===
    else if (STRICMP(cmd, "flush") == 0) {
        if (nrhs < 2) mexErrMsgTxt("Usage: evt2('flush', h)");
        hid_device *device = get_handle(prhs[1]);

        int res;
        do {
            res = hid_read_timeout(device, rx_buffer, sizeof(rx_buffer), 0);
        } while (res > 0);
        
        if (res < 0) mexErrMsgTxt("Flush error.");
    }

    // === PULSE COMMAND ===
    else if (STRICMP(cmd, "pulse") == 0) {
        if (nrhs < 4) mexErrMsgTxt("Usage: evt2('pulse', h, value, duration)");
        hid_device *device = get_handle(prhs[1]);
        unsigned short value = (unsigned short) mxGetScalar(prhs[2]);
        unsigned short duration = (unsigned short) mxGetScalar(prhs[3]);

        hid_package[CMD_CODE] = CMD_PULSE;
        hid_package[PARAM_1] = value;
        hid_package[PARAM_2] = duration & 0xFF;
        hid_package[PARAM_3] = (duration >> 8) & 0xFF;
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, 64);
        #else
        memcpy(&tx_buffer[0], hid_package, 64);
        #endif  

        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === CLEAR COMMAND ===
    else if (STRICMP(cmd, "clear") == 0) {
        if (nrhs < 2) mexErrMsgTxt("Usage: evt2('clear', h)");
        hid_device *device = get_handle(prhs[1]);

        hid_package[CMD_CODE] = CMD_CLR;
        hid_package[PARAM_1] = 0;       
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, 64);
        #else
        memcpy(&tx_buffer[0], hid_package, 64);
        #endif   

        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === SET COMMAND ===
    else if (STRICMP(cmd, "set") == 0) {
        if (nrhs < 2) mexErrMsgTxt("Usage: evt2('set', h)");
        hid_device *device = get_handle(prhs[1]);

        hid_package[CMD_CODE] = CMD_SET;
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, 64);
        #else
        memcpy(&tx_buffer[0], hid_package, 64);
        #endif   

        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === CLOSE DEVICE ===
    else if (STRICMP(cmd, "close") == 0) {
        if (nrhs < 2) mexErrMsgTxt("Usage: evt2('close', h)");
        int h = (int)mxGetScalar(prhs[1]);
        if (h >= 1 && h <= MAX_HANDLES && handles[h-1]) {
            hid_close(handles[h-1]);
            handles[h-1] = NULL;
            mexPrintf("HID device handle %d closed\n", h);
        }
    }

    // === UNKNOWN COMMAND ===
    else {
        mexErrMsgTxt(
            "Unknown command. Valid commands:\n"
            "  'list', 'open', 'write', 'read', 'flush', 'pulse', 'clear', 'set', 'close'."
        );
    }
}
