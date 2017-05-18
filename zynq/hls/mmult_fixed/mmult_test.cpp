#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "mmult.h"

void matrix_multiply_ref(out_T offsets[CLASSES], w_T weights[CLASSES][FEAT],  in_T in[BATCH][FEAT], out_T out[BATCH][CLASSES])
{
	// matrix multiplication of a A*B matrix
	for (int i = 0; i < BATCH; ++i) {
		for (int j = 0; j < CLASSES; ++j) {
			out_T sum = offsets[j];
			for (int k = 0; k < FEAT; ++k) {
				sum += in[i][k] * weights[j][k];
			}
			out[i][j] = sum;
		}
	}
	return;
}


int main(void)
{
	int i,j,err;

	out_T offsets[CLASSES];
	w_T weights[CLASSES][FEAT];
	in_T inputs[BATCH][FEAT];
	out_T output_sw[BATCH][CLASSES];
	out_T output_hw[BATCH][CLASSES];

	/** Matrix Initiation */
	for(i = 0; i<CLASSES; i++) {
		offsets[i] = (out_T) ((rand()%(1ULL<<OUT_WIDTH)) - (1ULL<<(OUT_WIDTH-1)));
	}

	for(i = 0; i<CLASSES; i++) {
		for(j = 0; j<FEAT; j++) {
			weights[i][j] = (w_T) ((rand()%(1ULL<<W_WIDTH)) - (1ULL<<(W_WIDTH-1)));
		}
	}

	for(i = 0; i<BATCH; i++) {
		for(j = 0; j<FEAT; j++) {
			inputs[i][j] = (in_T) (rand() % (1ULL<<IN_WIDTH));
		}
	}
	/** End of Initiation */


	printf("DEBUGGING AXI4 STREAMING DATA TYPES!\r\n");

	// prepare data for the DUT
	AXI_VAL in_stream[IS_SIZE];
	AXI_VAL out_stream[OS_SIZE];

	// Input and output stream indices
	int is_idx = 0;
	int os_idx = 0;

	// stream in the offset vector
	for(int i=0; i<CLASSES-OUT_WIDTH_RATIO; i+=OUT_WIDTH_RATIO) {
		axi_T packet = 0;
		PACK_OFF: for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
			out_bit_T bits = *((out_bit_T*) &offsets[i+w]);
			packet |= (bits & ((1ULL<<OUT_WIDTH)-1))<<(w*OUT_WIDTH);
		};
		in_stream[is_idx++] = push_stream(packet, 0);
	}
	// pad the last packet in case things don't align
	axi_T packet = 0;
	FINISH_OFF: for (int i = CLASSES-OUT_WIDTH_RATIO; i < CLASSES; i++) {
		out_bit_T bits = *((out_bit_T*) &offsets[i]);
		packet |= (bits & ((1ULL<<OUT_WIDTH)-1))<<((i%OUT_WIDTH_RATIO)*OUT_WIDTH);
	}
	in_stream[is_idx++] = push_stream(packet, 0);

	// stream in the weigth matrix
	for(int i=0; i<CLASSES; i++) {
		for(int j=0; j<FEAT; j+=W_WIDTH_RATIO) {
			axi_T packet = 0;
			PACK_W: for (int w = 0; w <W_WIDTH_RATIO; w++) {
				w_bit_T bits = *((w_bit_T*) &weights[i][j+w]);
				packet |= (bits & ((1ULL<<W_WIDTH)-1))<<(w*W_WIDTH);
			};
			in_stream[is_idx++] = push_stream(packet, 0);
		}
	}

	// stream in the input matrix
	for(int i=0; i<BATCH; i++) {
		for(int j=0; j<FEAT; j+=IN_WIDTH_RATIO) {
			axi_T packet = 0;
			PACK_IN: for (int w = 0; w <IN_WIDTH_RATIO; w++) {
				in_bit_T bits = *((in_bit_T*) &inputs[i][j+w]);
				packet |= (bits & ((1ULL<<IN_WIDTH)-1))<<(w*IN_WIDTH);
			};
			in_stream[is_idx++] = push_stream(packet, is_idx==(IS_SIZE));
		}
	}

	//call the DUT
	mmult_hw(in_stream, out_stream);

	// extract the output matrix from the out stream
	for(int i=0; i<BATCH; i++) {
		for(int j=0; j<CLASSES-OUT_WIDTH_RATIO; j+=OUT_WIDTH_RATIO) {
			axi_T packet = pop_stream(out_stream[os_idx++]);
			UNPACK_OUT: for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
				out_bit_T bits = (packet>>(w*OUT_WIDTH));
				output_hw[i][j+w] = *((out_T*) &bits) & ((1ULL<<OUT_WIDTH)-1);
			}
		}
		// Pop last AXI data packet
		axi_T packet = pop_stream(out_stream[os_idx++]);
		FINISH_OUT: for (int j = CLASSES-OUT_WIDTH_RATIO; j < CLASSES; j++) {
			out_bit_T bits = (packet>>((j%OUT_WIDTH_RATIO)*OUT_WIDTH));
			output_hw[i][j] = *((out_T*) &bits) & ((1ULL<<OUT_WIDTH)-1);
		}
	}

	/* reference Matrix Multiplication */
	matrix_multiply_ref(offsets, weights, inputs, output_sw);

	/** Matrix comparison */
	err = 0;
	for (i = 0; i<BATCH; i++) {
		for (j = 0; j<CLASSES; j++) {
			if (output_sw[i][j] != output_hw[i][j]) {
				err++;
				std::cout << i << "," << j << ": expected " << output_sw[i][j] << " but got " << output_hw[i][j] << std::endl;
			}
		}
	}

	if (err == 0)
		printf("Matrices identical ... Test successful!\r\n");
	else
		printf("Test failed!\r\n");

	return err;

}
