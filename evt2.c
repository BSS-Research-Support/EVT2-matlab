/*
 * MEX gateway function for communicating with a HID device via HIDAPI.
 *
 * Supported commands:
 *   - EventExchanger('open', vid, pid)
 *   - EventExchanger('write', value)
 *   - EventExchanger('pulse', value, duration)
 *   - EventExchanger('read')
 *   - EventExchanger('clear')
 *   - EventExchanger('set')
 *   - EventExchanger('close')
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

/// Global HID device handle
static hid_device *device = NULL;

static unsigned char hid_package[64] = {0};


// Cleanup function called when the MEX file is cleared from memory
void cleanup(void) {
    if (device) {
        hid_close(device);
        device = NULL;
        mexPrintf("HID device closed via cleanup.\n");
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

    // === OPEN DEVICE ===
    if (STRICMP(cmd, "open") == 0) {

        unsigned short vid = VENDOR_ID;
        unsigned short pid = PRODUCT_ID;

        // If arguments are provided, override defaults
        if (nrhs >= 3) {
            vid = (unsigned short) mxGetScalar(prhs[1]);
            pid = (unsigned short) mxGetScalar(prhs[2]);
        } else if (nrhs != 1) {
            mexErrMsgTxt("Usage: evt2('open', [vid, pid])");
        }

        if (device) {
            hid_close(device);
            device = NULL;
        }

        device = hid_open(vid, pid, NULL);
        if (!device) {
            mexErrMsgTxt("Failed to open EVT-2 (HID) device. On Linux, check udev rules or run with sudo.");
        }

        hid_set_nonblocking(device, 1);
        mexPrintf("EVT-2 device opened (VID: 0x%04x, PID: 0x%04x)\n", vid, pid);
    }

    // === WRITE TO DEVICE ===
    else if (STRICMP(cmd, "write") == 0) {
        if (!device) mexErrMsgTxt("Device not open.");
        if (nrhs < 2) mexErrMsgTxt("Usage: EventExchanger('write', value)");
        
        unsigned short value = (unsigned short) mxGetScalar(prhs[1]);
		
        hid_package[CMD_CODE] = CMD_WR_OP;
        hid_package[PARAM_1] = value;
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, sizeof(tx_buffer));
        #else
        memcpy(&tx_buffer[0], hid_package, sizeof(tx_buffer));
        #endif                
        
        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === READ FROM DEVICE ===
    else if (STRICMP(cmd, "read") == 0) {
        if (!device) mexErrMsgTxt("Device not open.");

        int res = hid_read_timeout(device, rx_buffer, sizeof(rx_buffer), 1000); // 1 sec timeout

        if (res < 0) {
            mexErrMsgTxt("Read error.");
        }

        plhs[0] = mxCreateNumericMatrix(1, res, mxUINT8_CLASS, mxREAL);
        if (res > 0) {
            memcpy(mxGetData(plhs[0]), rx_buffer, res);
        }
    }

    // === PULSE COMMAND ===
    else if (STRICMP(cmd, "pulse") == 0) {
        if (!device) mexErrMsgTxt("Device not open.");
        if (nrhs < 3) mexErrMsgTxt("Usage: EventExchanger('pulse', value, duration)");

        unsigned short value = (unsigned short) mxGetScalar(prhs[1]);
        unsigned short duration = (unsigned short) mxGetScalar(prhs[2]);

        hid_package[CMD_CODE] = CMD_PULSE;
        hid_package[PARAM_1] = value; // Pulse target value
        hid_package[PARAM_2] = duration & 0xFF;        // Duration LSB
        hid_package[PARAM_3] = (duration >> 8) & 0xFF; // Duration MSB
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, sizeof(tx_buffer));
        #else
        memcpy(&tx_buffer[0], hid_package, sizeof(tx_buffer));
        #endif  

        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === CLEAR COMMAND ===
    else if (STRICMP(cmd, "clear") == 0) {
        if (!device) mexErrMsgTxt("Device not open.");

        hid_package[CMD_CODE] = CMD_CLR;
        hid_package[PARAM_1] = 0; // Should not be needed... (don't care)       
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, sizeof(tx_buffer));
        #else
        memcpy(&tx_buffer[0], hid_package, sizeof(tx_buffer));
        #endif   

        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === SET COMMAND ===
    else if (STRICMP(cmd, "set") == 0) {
        if (!device) mexErrMsgTxt("Device not open.");

        hid_package[CMD_CODE] = CMD_SET;
        #if defined(_WIN32)          
        memcpy(&tx_buffer[1], hid_package, sizeof(tx_buffer));
        #else
        memcpy(&tx_buffer[0], hid_package, sizeof(tx_buffer));
        #endif   

        int res = hid_write(device, tx_buffer, sizeof(tx_buffer));
        if (nlhs > 0) plhs[0] = mxCreateDoubleScalar(res);
    }

    // === CLOSE DEVICE ===
    else if (STRICMP(cmd, "close") == 0) {
        if (device) {
            hid_close(device);
            device = NULL;
            mexPrintf("HID device closed\n");
        }
    }

    // === UNKNOWN COMMAND ===
    else {
        mexErrMsgTxt(
            "Unknown command. Valid commands:\n"
            "  'open', 'write', 'read', 'pulse', 'clear', 'close'."
        );
    }
}
