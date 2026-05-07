/*
 * rwave.h 
 *
 */

#ifndef _EVT2_H_
#define _EVT2_H_

#ifdef __cplusplus
 extern "C" {
#endif

#define VENDOR_ID      0x0808  // rWave vendor ID. This should never change
#define PRODUCT_ID     0x0001  // Product ID. Set this product ID to match your device.

enum commandInstructions {
	CMD_CLR = 0x00,     // Clear all outputs
	CMD_SET,			// Set all outputs
	CMD_WR_OP,			// Write to outputs
	CMD_WR_SNGL_OP,		// Write to single output
	CMD_PULSE,			// Pulse outputs
	CMD_PULSE_SNGL_OP,	// Pulse a single output
	CMD_RESET = 0x80	// Reset device.
};


enum txPackageBytes {
	CMD_CODE,
	PARAM_1,
    PARAM_2,
    PARAM_3
};


#endif /* _RWAVE_H_ */
