#include <stdio.h>
#include <stdlib.h>

#include "mmult.h"

// --------------------------------------------------------------------
// function to be accelerated in HW wrapped with AXI4-Stream interface
void mmult_hw (AXI_VAL in_stream[IS_SIZE], AXI_VAL out_stream[OS_SIZE])
{
#pragma HLS INTERFACE s_axilite port=return     bundle=CONTROL_BUS
#pragma HLS INTERFACE axis      port=in_stream
#pragma HLS INTERFACE axis      port=out_stream

	// Assertions (to avoid out of array bound writes)
	assert(CLASSES%WIDTH_RATIO==0);
	assert(FEAT%WIDTH_RATIO==0);
	assert(FEAT%WIDTH_RATIO==0);
	assert((BATCH*CLASSES)%WIDTH_RATIO==0);

	// Union used for type conversion
	union
	{
		axi_T packet;
		struct {T f0; T f1;} val;
	} converter;

	// Hardware buffers
	T offset_buf[CLASSES];
	T weight_buf[CLASSES][FEAT];
	T in_buf[BATCH][FEAT];
	T out_buf[BATCH][CLASSES];

	// Input and output AXI stream indices
	int is_idx = 0;
	int os_idx = 0;

	// Stream in offset vector
	LOAD_OFF_1: for (int i = 0; i < CLASSES; i+=WIDTH_RATIO) {
		converter.packet = pop_stream(in_stream[is_idx++]);
		offset_buf[i+0] = converter.val.f0;
		offset_buf[i+1] = converter.val.f1;
	}

	// Stream in weight matrix
	LOAD_W_1: for (int i = 0; i < CLASSES; i++) {
		LOAD_W_2: for (int j = 0; j < FEAT; j+=WIDTH_RATIO) {
			// Pop AXI data packet
			converter.packet = pop_stream(in_stream[is_idx++]);
			weight_buf[i][j+0]  = converter.val.f0;
			weight_buf[i][j+1]  = converter.val.f1;
		}
	}


	// Stream in input matrix
	LOAD_I_1: for (int i = 0; i < BATCH; i++) {
		LOAD_I_2: for (int j = 0; j < FEAT; j+=WIDTH_RATIO) {
			// Pop AXI data packet
			converter.packet = pop_stream(in_stream[is_idx++]);
			in_buf[i][j+0]  = converter.val.f0;
			in_buf[i][j+1]  = converter.val.f1;
		}
	}

	// Iterate over batch elements
	L1: for (int i = 0; i < BATCH; i++) {
		// Iterate over output classes
		L2: for (int j = 0; j < CLASSES; j++) {
			// Perform the dot product
			T tmp = offset_buf[j];
			L3: for(int k = 0; k < FEAT; k++) {
				tmp += in_buf[i][k] * weight_buf[j][k];
			}
			out_buf[i][j] = tmp;
		}
	}

	// Stream out output matrix
	STORE_O_1: for (int i = 0; i < BATCH; i++) {
		STORE_O_2: for (int j = 0; j < CLASSES; j+=WIDTH_RATIO) {
			// Push output element into AXI stream
			converter.val.f0 = out_buf[i][j+0];
			converter.val.f1 = out_buf[i][j+1];
			out_stream[os_idx++] = push_stream(converter.packet, os_idx == (OS_SIZE));
		}
	}
}


// --------------------------------------------------------
// functions to insert and extract elements from an axi stream
// includes conversion to correct data type
axi_T pop_stream(AXI_VAL const &e)
{
#pragma HLS INLINE

	axi_T ret = e.data;

	volatile ap_uint<sizeof(axi_T)> strb = e.strb;
	volatile ap_uint<sizeof(axi_T)> keep = e.keep;
	volatile ap_uint<AXI_U> user = e.user;
	volatile ap_uint<1> last = e.last;
	volatile ap_uint<AXI_TI> id = e.id;
	volatile ap_uint<AXI_TD> dest = e.dest;

	return ret;
}

AXI_VAL push_stream(axi_T const &v, bool last = false)
{
#pragma HLS INLINE

	AXI_VAL e;

	e.data = v;
	e.strb = (1<<sizeof(axi_T))-1;
	e.keep = (1<<sizeof(axi_T))-1;
	e.user = 0;
	e.last = last ? 1 : 0;
	e.id = 0;
	e.dest = 0;
	return e;
}

