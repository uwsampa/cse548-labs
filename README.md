# Lab 3: Custom Acceleration with FPGAs

**Due** Wednesday May 24rd at 23:59 via the Catalyst dropbox.

Questions about this assignment should got to the TA, Thierry.

### Turnin

You should submit the following files:
* Your report in `.pdf` format with concise answers to the questions asked in Parts 1 and 2. In addition, the report should contain a short (couple paragraphs) description of your optimized implementation for Part 3 (what you changed about the hardware, or classifier, or both).
* Your code implementation of the floating point and fixed point classifiers, gzipped as `src.tar.gz` with the following directory structure (these designs should compile with the `hls.tcl` without errors):
   *  `mmult_float/mmult_float.cpp`
   *  `mmult_float/mmult.h`
   *  `mmult_fixed/mmult_fixed.cpp`
   *  `mmult_fixed/mmult.h`
* Your optimized implementation source code, gzipped as `solution.tar.gz` which includes:
   * Training scripts/programs for reproducibility (if you re-trained or changed the classifier)
   * Your hardware design source (if you changed the hardware source, as a .cpp and .h file)
   * The `classifier.bit` bitstream and the iPython notebook `classifier.ipynb` to run design on the PYNQ board.

### Story

You are a successful researcher at Wuugle and your direct supervisor Deff Jean has commissioned you to develop a top-secret prototype hardware accelerator, called the Matrix Processing Unit, or MPU™.
You've only been given two weeks to implement a successful prototype on an FPGA. You recall that you took an awesome computer architecture class that taught you everything there is to know about hardware design at the F. Phil G. Allan (F.P.G.A) School of Computer Science!
You go to the drawing board to design an MPU™ prototype that will revolutionize shallow learning.


# Introduction and Background:

The goal of this lab is to provide exposure to custom hardware design for software programmers. Your task will be to accelerate a machine learning classifier with a low-power FPGA. This lab will introduce you to concepts such as:
* Hardware resource management.
* Pipeline optimization with data partitioning, batching and tiling.
* Fixed-point optimizations to maximize throughput in hardware.

### High Level Synthesis

Hardware is most commonly described with low-level hardware description languages such as Verilog or VHDL. Those domain specific languages have a fairly steep learning curve so we’ll rely on a higher-level entry language instead: [HLS-C](https://www.xilinx.com/support/documentation-navigation/design-hubs/dh0012-vivado-high-level-synthesis-hub.html).

This high-level language is very similar to C/C++, but incorporates compiler pragmas to express hardware-specific optimizations. While HLS-C still requires getting used to, its learning curve is not as steep as Verilog, and when utilized correctly can outperform expert-designed hardware code.

### MNIST Classification

[MNIST handwritten digit recognition dataset](http://yann.lecun.com/exdb/mnist/) is a standard vision dataset that has been mostly solved in the late 90’s with [Convolutional Neural Networks](http://yann.lecun.com/exdb/publis/pdf/lecun-01a.pdf). This dataset is perfect for experimenting hardware optimizations due to its extreme simplicity.

In this lab, we’ve trained a naive [linear classifier](https://en.wikipedia.org/wiki/Linear_classifier) to perform hand digit recognition on scaled-down 16x16 images of hand-written digits. This equates to performing matrix multiplication over **I** and **W** where **I** is an (BxF) input matrix and **W** is an (FxC) weight matrix, and where B, F and C denote batch size, input features size and category count respectively.

You can find the implementation of the linear classifier under `python/mnist.py`. To execute this Python program, you will need:
* Python 2.7 and pip
* The following packages: numpy, scipy, sklearn, Pillow

### Hardware Kit Overview

You will be given an FPGA kit that consists of the following components:
* A [PYNQ board](http://www.pynq.io/) with a Xilinx [Zynq](https://www.xilinx.com/products/silicon-devices/soc/zynq-7000.html) Programmable SoC (Artix 7 FPGA + ARM Cortex A-9 Processor).
* An external 12V, 3A power adapter to power on the PYNQ board.
* An SD card to serve as primary drive.

In addition, you will need an Ethernet port on your machine to communicate with the board. We can provide complementary Ethernet to USB adapters.

# Prerequisites and PYNQ Setup:

### Linux 64-bit OS

If you don’t have a 64-bit Linux OS installed on your machine, we recommend [VirtualBox](https://www.virtualbox.org/wiki/VirtualBox) (free), [VMWare](http://www.vmware.com/) (free under [CSE VMWare Academic Program](https://www.cs.washington.edu/lab/software/homeVMs)), or dual booting your machine. 

Make sure to allocate at least 32GB (or 64GB preferably) of disk drive space for your VM’s main partition. In addition, compilation jobs can be resource-intensive, so allocating 4-8GB of DRAM for your VM would be wise. We’ve tested the tools under Ubuntu 16.04.2 but any of the following OSes or newer should work:
* Red Hat Enterprise Linux 6.6 64-bit
* CentOS Linux 6.7
* SUSE Enterprise Linux 11.4 64-bit
* Ubuntu Linux 16.04.1 LTS 64-bit

**Note** If you're using VMWare, do not have your source and work directory sit on a shared drive with your host OS. For some reason VMWare directory sharing is slow to update file changes from the host OS to the virtual OS, which can lead to compilation bugs.

### Vivado HL WebPACK 2017.1

You’ll need to install Xilinx’ FPGA compilation toolchain, [Vivado HL WebPACK 2017.1](https://www.xilinx.com/products/design-tools/vivado.html), which a license-free version of the Vivado HLx toolchain.

1. Go to the [download webpage](https://www.xilinx.com/support/download.html), and download the Linux Self Extracting Web Installer for Vivado HL 2017.1 WebPACK and Editions.
2. You’ll have to sign in with a Xilinx account. This requires a Xilinx account creation that will take 2 minutes.
3. Pass the Name and Address Verification by clicking “Next”, and you will get the opportunity to download a binary file, called `Xilinx_Vivado_SDK_2017.1_0415_1_Lin64.bin`.
4. Now that the file is downloaded, go to your Downloads directory, and change the file permissions so it can be executed: `chmod u+x Xilinx_Vivado_SDK_2017.1_0415_1_Lin64.bin`
5. Now you can execute the binary: `./Xilinx_Vivado_SDK_2017.1_0415_1_Lin64.bin`
6. A Vivado 2017.1 Installer program GUI will launch.
   * Click “Next” on the **Welcome** screen.
   * Enter your Xilinx User Credentials under “User Authentication” and select the “Download and Install Now” before clicking “Next” on the **Select Install Type** screen.
   * Accept all terms before clicking on “Next” on the **Accept License Agreements** screen.
   * Select the “Vivado HL WebPACK” before clicking on “Next” on the **Select Edition to Install** screen.
   * Under the **Vivado HL WebPACK** screen, before hitting “Next", check the following options (the rest should be unchecked):
     * Design Tools -> Vivado Design Suite -> Vivado
     * Design Tools -> Vivado Design Suite -> Vivado High Level Synthesis
     * Devices -> Production Services -> SoCs -> Zynq-7000 Series
   * Your total download size should be about 3GB and the amount of Disk Space Required 13GB.
   * Set the installation directory before clicking “Next” on the **Select Destination Directory** screen. It might highlight some paths as red - that’s because the installer doesn’t have the permission to write to that directory. In that case select a path that doesn’t require special write permissions (e.g. in your home directory).
   * Hit “Install” under the **Installation Summary** screen.
   * An **Installation Progress Window** will pop-up to track progress of the download and the installation.
   * This process will take about 20-30 minutes depending on your connection speed.
   * A pop-up window will inform you that the installation completed successfully. Click "OK".
   * Finally the **Vivado License Manager** will launch. Select "Get Free ISE WebPACK, ISE/Vivado IP or PetaLinux License" and click "Connect Now" to complete the license registration process. 
7. The last step is to update your `~/.bashrc` with the following line:
```bash
# Xilinx Vivado 2017.1 environment
source <install_path>/Vivado/2017.1/settings64.sh
```

### PYNQ board

The PYNQ board website complete with documentation and forums is available [here](http://www.pynq.io/).
Follow the **Getting Started** tutorial to get your Pynq board set up (please read the notes below first).

**SD card flashing notes**
* We recommend using [Etcher](https://etcher.io/) for one-step SD-card flashing. You can download the image for the SD card on the PYNQ board [website](http://www.pynq.io/).

**Board setup notes:**
* Your boot jumper should be set to SD position.
* Your power jumper should be set to the REG position.
* No need to connect a USB cable.
* Connect your 12V adapter to power on the board.

**Ethernet connection notes:**
* Instead of connecting to a network, you will connect the board directly to an Ethernet port on the computer.
* Note that it'll be easier easier to connect to the board via your primary OS rather than your VM (if you are using one).

**Connecting to Jupyter notes:**
* It seems like you won’t be able to connect to the board successfully using either Firefox or Safari. We recommend using [Chrome](https://www.google.com/chrome/) instead.

Try one of the iPython notebook examples available out-of-the-box on your PYNQ board to make sure that it works as intended!

# Part 1: Matrix Multiplication Pipeline Optimization in HLS (50 pts)

This first part will cover fundamentals of high level synthesis. 

### Recommended Reading

The [Vivado HLS User Guide](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_1/ug902-vivado-high-level-synthesis.pdf) provides plenty of valuable reference information. We *strongly* recommend reading the **Understanding High-Level Synthesis** pages 5-12 as an introduction.

As an optional exercise, we recommend going through the [Vivado HLS Tutorial](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_1/ug871-vivado-high-level-synthesis-tutorial.pdf) Chapters 6 to familiarize yourself with design analysis and design optimization techniques (about 30mins). 


## A. Understanding the baseline matrix multiply (background)

### Code Overview

A standard matrix multiply algorithm in C++ is described below (**we assume that w_buf is already transposed for improved locality**). The loops have been labeled, which facilitates the analysis of the program performed by HLS.
```c++
// Standard Matrix Multiply/Linear Classifier algorithm (on in_buf and transposed w_buf to produce out_buf)
L1: for (int i = 0; i < BATCH; i++)
{
	L2: for (int j = 0; j < CLASSES; j++)
    {
		// Perform the dot product in the inner loop
		float tmp = offset_buf[j];
		L3: for(int k = 0; k < FEAT; k++)
        {
			tmp += in_buf[i][k] * weight_buf[j][k];
		}
        out_buf[i][j] = tmp;
    }
}
```

You will find a naive matrix multiply implementation in `hls/mmult_float/mmult_float.cpp` ready to be synthesized to hardware with HLS.

The `mmult_hw()` function contains certain high-level HLS-specific pragmas defining an [AXI Lite](https://www.xilinx.com/products/intellectual-property/axi_lite_ipif.html) control interface and an [AXI stream](https://www.xilinx.com/products/intellectual-property/axi4-stream_interconnect.html) input and output communication interface.
* *AXI Lite* is a simple protocol that provides point-to-point bi-directional communication between the ARM processor and the HLS accelerator to control the HLS accelerator tasks.
* *AXI Stream* provides a unidirectional throughput-oriented communication channel between the ARM processor's memory system and the hardware accelerator and vice-versa.

In order to interface with the input and output AXI streams, we've provided two function helpers: `pop_stream()` and `push_stream()`. The code below shows how these two helper functions are used to stream data into and out of the user-instantiated hardware buffers. 

```c++
// Input stream and output stream sizes
#define IS_SIZE CLASSES+CLASSES*FEAT+BATCHES*FEAT
#define OS_SIZE BATCHES*CLASSES

// AXI Stream interface
void mmult_hw (AXI_VAL in_stream[IS_SIZE], AXI_VAL out_stream[OS_SIZE]) {
	
    // Hardware buffers
	float offset_buf[CLASSES];
	float weight_buf[CLASSES][FEAT];
	float in_buf[BATCHES][FEAT];
	float out_buf[BATCHES][CLASSES];
    
	// Input and output AXI stream indices
	int is_idx = 0;
	int os_idx = 0;

	// Stream data into offset_buf
	LOAD_OFF: for (int i = 0; i < CLASSES; i++) {
		// Pop data from stream
		offset_buf[i] = pop_stream(in_stream[is_idx++]);
	}
	
    // Stream data into weight_buf
	LOAD_W_1: for (int i = 0; i < CLASSES; i++) {
		LOAD_W_2: for (int j = 0; j < FEAT; j++) {
		// Pop data from stream
		weight_buf[i][j] = pop_stream(in_stream[is_idx++]);
	}
    
    // Stream data into in_buf
	LOAD_I_1: for (int i = 0; i < BATCH; i++) {
		LOAD_I_2: for (int j = 0; j < FEAT; j++) {
		// Pop data from stream
		in_buf[i][j] = pop_stream(in_stream[is_idx++]);
	}
    
    // Do Matrix Multiplication
    ...
    
    
    // Stream data out of out_buf
	STORE_O_1: for (int i = 0; i < BATCH; i++) {
		STORE_O_2: for (int j = 0; j < CLASSES; j++) {
		// Push output element into AXI stream
        // push_stream's second argument should be set to True when sending the last packet out
		out_stream[os_idx] = push_stream(out_buf[i][j], os_idx++ == (BATCH*CLASSES-1));
	}
}
```

The code above assumes that the size of each AXI Stream data packet matches the `float` width. Actually the AXI Stream packet size as instantiated in our hardware design is 64-bits, meaning that `pop_stream()` returns two floats at a time, and `push_stream()` consumes two floats at a time.

In order to handle data type conversions you can use a `union`. The code below shows how to perform type conversion quickly.

```c++
union
{
	axi_T packet;
	struct {float in_0; float in_1;} val;
} converter;

...

// Stream data into w_buf
LOAD_W_1: for (int i = 0; i < CLASSES; i++) {
	// Increment by 2 (ratio between AXI bus width and float width)
	LOAD_W_2: for (int j = 0; j < FEAT; j+=2) {
		// Pop data from stream
		int k = i*FEAT+j;
    	converter.packet = pop_stream(in_stream[k]);
		w_buf[i][j+0] = converter.val.in_0;
		w_buf[i][j+1] = converter.val.in_1;
	}
}
```

### HLS Compilation and Design Analysis

Now the code in `mmult_float.cpp` should look a lot more familiar!

Execute the HLS compilation (takes about 15-30s for the base design):
```
cd hls/mmult_float/
vivado_hls -f hls.tcl
```
The `hls.tcl` contains a sequence of commands to compile your design via HLS. You don't need to modify this file. It will run basic correctness checks in simulation, which are specified in the `mmult_test.cpp` test-bench. If the simulation fails, your hardware design won't be synthesized.

Open the report under `hls/mmult_float/accel/solution0/syn/report/mmult_hw_csynth.rpt` to analyze your synthesized design.

You will find a resource utilization report that breaks down the synthesized module footprint on the FPGA.
The `BRAM_18K` are on-chip SRAM memories, the `DSP48E` are hard fused multiply and add (FMA) units, the `FF` are flip flops, and `LUT` are lookup-tables.
It is clear that the design has quite a small footprint: less than 5% utilization overall (bottlenecked by the `BRAM_18K`).

```
+-----------------+---------+-------+--------+-------+
|       Name      | BRAM_18K| DSP48E|   FF   |  LUT  |
+-----------------+---------+-------+--------+-------+
|DSP              |        -|      -|       -|      -|
|Expression       |        -|      -|       0|    308|
|FIFO             |        -|      -|       -|      -|
|Instance         |        0|      5|     384|    751|
|Memory           |       16|      -|       0|      0|
|Multiplexer      |        -|      -|       -|    381|
|Register         |        -|      -|     714|      -|
+-----------------+---------+-------+--------+-------+
|Total            |       16|      5|    1098|   1440|
+-----------------+---------+-------+--------+-------+
|Available        |      280|    220|  106400|  53200|
+-----------------+---------+-------+--------+-------+
|Utilization (%)  |        5|      2|       1|      2|
+-----------------+---------+-------+--------+-------+
```

You will also find a performance estimate in the report:

```
+--------+--------+--------+--------+---------+
|     Latency     |     Interval    | Pipeline|
|   min  |   max  |   min  |   max  |   Type  |
+--------+--------+--------+--------+---------+
|  209851|  209851|  209852|  209852|   none  |
+--------+--------+--------+--------+---------+
```

The Latency indicates how many FPGA cycles it takes to perform one matrix multiplication on the FPGA (i.e. a batch of inference tasks). The Initiation Interval describes how many cycles you'd have to wait until you can process the next batch of input data. These two metrics are identical because the entire algorithm is too large to be pipelined. 

Since the FPGA design is clocked at 100MHz, it takes 2.099ms to perform a single inference. This is very slow and the ARM CPU clocked at 667MHz with a dedicated FPU would have no problem beating this naive implementation.

Let's take a deeper look at the loop analysis report (you now understand why we labeled the loops in the first place) to identify optimization opportunities.

```
+--------------+--------+--------+----------+-----------+-----------+------+----------+
|              |     Latency     | Iteration|  Initiation Interval  | Trip |          |
|   Loop Name  |   min  |   max  |  Latency |  achieved |   target  | Count| Pipelined|
+--------------+--------+--------+----------+-----------+-----------+------+----------+
|- LOAD_OFF_1  |      10|      10|         2|          -|          -|     5|    no    |
|- LOAD_W_1    |    2580|    2580|       258|          -|          -|    10|    no    |
| + LOAD_W_2   |     256|     256|         2|          -|          -|   128|    no    |
|- LOAD_I_1    |    2064|    2064|       258|          -|          -|     8|    no    |
| + LOAD_I_2   |     256|     256|         2|          -|          -|   128|    no    |
|- L1          |  205056|  205056|     25632|          -|          -|     8|    no    |
| + L2         |   25630|   25630|      2563|          -|          -|    10|    no    |
|  ++ L3       |    2560|    2560|        10|          -|          -|   256|    no    |
|- STORE_O_1   |     136|     136|        17|          -|          -|     8|    no    |
| + STORE_O_2  |      15|      15|         3|          -|          -|     5|    no    |
+--------------+--------+--------+----------+-----------+-----------+------+----------+

```

This report tells us that every step of our matrix multiply hardware is executing sequentially (as if we executed our entire matrix multiply on a single-cycle CPU). You'll observe that the L3 loop (inner dot product loop) takes 2560 cycles to perform a dot product of two vectors with 256 elements, meaning that it takes 10 cycles to multiply and add two elements!
This is because a floating point multiplication on the FPGA takes 4 cycles, and an addition 5 cycles.

Let's look at how we can improve this design with pipelining and batching!

## B. Pipelining in HLS (10 pts)

The base design has pretty underwhelming performance. But from analyzing the design we can make two observations:
* We are clearly under-utilizing our resources, so we have room to utilize more memory and logic resources to improve overall throughput.
* We are also not taking advantage of pipeline parallelism in any part of the loop, as indicated by the rather high inference latency of our matrix multiply.

We will exercise some common pipeline optimization techniques described in the **Optimizing For Throughput** chapter of the [Vivado HLS User Guide](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_1/ug902-vivado-high-level-synthesis.pdf), pages 188-206.

### Problem Statement

Carefully insert `#pragma HLS PIPELINE II=1` directives in your code to tell HLS to pipeline your loops in order to exploit pipeline parallelism.

Report (1) the design latency in cycles, (2) the overall device utilization (as Total per Resource), (3) the number of floating point adders and multipliers (you can find this information under the Instance section of the synthesis report) and (4) the Initiation Interval of the loops you pipelined.

**Hints**:
* Pragmas should be inserted after the target loop header that you wish to unroll. You can always use the Vivado GUI after compilation with the following command: `vivado_hls -p accel/` to correctly insert pragmas.
* Chapter 7 of the [Vivado HLS Tutorial](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_1/ug871-vivado-high-level-synthesis-tutorial.pdf) should provide enough guidance on how to do this effectively.
* Start from the inner-most loop(s) before moving to an outer-loop. Starting at an outer loop will flatten it entirely and your resource usage and compilation time will explode.

**What to expect**: You should roughly get a 15x improvement in latency over our baseline! This is the power of pipeline-parallelism. HLS has identified parallelism between multiplications of a single dot product, and pipeline parallelism between successive invocations of a dot product. This approach greatly reduces initiation latency of the inner loop.

## C. Increasing Pipeline Parallelism by Repartitioning Memories (10 pts)

If you examine the log from HLS in `vivado_hls.log` you will find the following warning message:
```
WARNING: [SCHED 204-69] Unable to schedule 'load' operation ('in_buf_load_2', ./mmult_accel.cpp:79) on array 'in_buf', ./mmult_accel.cpp:32 due to limited memory ports.
```
The pipelined design suffers from a resource contention problem. While HLS tries to allocate more adders and multipliers to expose more parallelism in the design, it can only leverage as much parallelism as the FPGA memories allow.

By default, FPGA-based SRAM memories are dual-ported. However you can tell the compiler to distribute your buffer across multiplier SRAM memories to offer more ports and therefore expose more parallelism. 

### Problem Statement

Carefully insert `#pragma HLS ARRAY_PARTITION variable=<variable> block factor=<factor> dim=<dim>` directives in your code to tell HLS to partition your data across multiple memories to expose more ports, and unlock more pipeline parallelism.

Gradually increase the partitioning factor by power of 2 increments until you exceed your resource budget. Report (1) the design latency in cycles, (2) the overall device utilization (as Total per Resource), (3) the number of floating point adders and multipliers (you can find this information under the Instance section of the synthesis report) and (4) the Initiation Interval of the loops you pipelined.

**Hints**:
* Again, Chapter 7 of the [Vivado HLS Tutorial](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_1/ug871-vivado-high-level-synthesis-tutorial.pdf) should provide enough guidance on how to do this effectively.

**What to expect**: You should roughly get a 42x improvement in latency over our baseline! All we did there was increasing the amount of pipeline parallelism in the matrix multiply algorithm by exposing more memory ports, thus increasing the capacity to perform more reads in parallel.

## D. Amortizing Iteration Latency with Batching (10 pts)

So far we've really improved our design performance, but we can still do better!

If you analyze your Loop, you'll observe that while your matrix multiplication loop has a relatively low initiation interval, it's *initiation latency* is very high. The initiation latency is the latency of the loop, while the initiation interval is the number of cycles that go by before your hardware pipeline can start a new invocation.

The latency of the matrix multiplication loop can be expressed as: `latency = iteration_latency + initiation_interval * (trip_count-1)`.
With a batch size of `8` that our design currently utilizes, the iteration latency accounts for about half of the loop latency!

To remedy this problem we are going to increase the batch size, `BATCH` defined under `/hls/mmult/mmult.h`, in order to amortize the iteration latency as much as possible.

### Problem Statement

Increase the batch size by power of 2 increments until you exceed the FPGA resource budget. For the largest batch size that does not exceed FPGA resource requirements, report (1) the design latency in cycles, and (2) the overall device utilization (as Total per Resource).

**What to expect**: Increasing the batch size will amortize the iteration latency of your overall design. Normalized to the batch size, your current design should now see an 86x latency reduction compared to the baseline design and see roughly a 55% `BRAM_18K` utilization.


## E. Extending Batch Size with Tiling (10 pts)

This final optimization step should help us decouple the `BRAM_18K` requirements the and batch size of our algorithm. This will allow us to reduce our normalized total latency even further.

You might have observed that in terms of utilization, we could minimize storage requirements by processing only a handful of input rows at a time.
Similarly only a handful of output rows need to be buffered at any point in time.

This allows us to arbitrarily grow our batch without incurring excessive overheads. We provide the pseudo-code below to provide guidance on how to apply tiling.

```c++
#define TILE_SIZE 128

...

T in_buf[TILE_SIZE][FEAT];
T out_buf[TILE_SIZE][CLASSES];

// Load offsets
...
// Load weights
...

// Iterate over tiles
LT: for (t=0; t<BATCH; t+=TILE_SIZE)
	// Load input tile
    ...
    // Perform matrix multiplication on input tile
	...
    // Store output tile
```

### Problem Statement

Given a tile size of `128`, and a batch size of `2048`, implement input tiling in mmult_hw(). Make sure that your implementation passes simulation.

Report (1) the design latency in cycles and (2) the overall device utilization (as Total per Resource).

**What to expect**: This optimization will both reduce the overall utilization of BRAM without affecting overall batch normalized latency. Increasing the batch size will amortize the overheads of invoking the FPGA accelerator from the CPU, which in our simple test setup can have high latency.


## F. Hardware compilation and FPGA testing on the PYNQ (10 pts)

You now have well optimized hardware accelerator, and are ready to take it for a spin!

Thankfully the painstaking process of integrating your piece of hardware into an FPGA system has been fully automated! In the `zynq/` directory, all you have to type is:
```
make -j1
```

And voilà! This process will take a little while (30 mins) depending on utilization. Regarding utilization, to avoid *no-place* errors, we recommend compiling your design where no resource exceeds 60%. This will ensure that your place and route tools have breathing space. In addition, it will make compilation time go a little faster. **Consequently we recommend scaling the design down a little, by limiting the array partitioning factor to `4`, and keeping both tile size and batch size to `128` and `2048` respectively.**

When your design is done compiling, it is ready to be tested on the PYNQ! Let's go ahead and transfer (1) the hardware design overlay files, (2) the test program, (3) the trained linear model, and (4) the MNIST validation data and labels to the PYNQ board.
```bash
scp build/export/classifier.bit xilinx@192.168.2.99:/home/xilinx/pynq/bitstream/.
scp tcl/classifier.tcl xilinx@192.168.2.99:/home/xilinx/pynq/bitstream/.
scp jupyter/classifier_1.ipynb xilinx@192.168.2.99:/home/xilinx/jupyter_notebooks/.
scp python/*.npy xilinx@192.168.2.99:/home/xilinx/jupyter_notebooks/.
```

Now log onto your PYNQ board on Chrome by entering the following address: http://192.168.2.99:9090/. Make sure you've properly powered on the board, and connected the board via Ethernet to your host machine. In addition, ensure that you've properly configured your machine's network settings as indicated in the PYNQ getting started guide.

Use the `xilinx` credentials (pwd: `xilinx`) to log into the iPython notebook server. If the file transfer completed with success, you should see your `classifier_1.ipynb` notebook in the jupyter directory tree.

Click on it to launch the program! You can simply execute the entire program by clicking on **Kernel -> Restart & Run All**.

### Problem Statement

Report (1) the measured speedup and (2) measured classification accuracy.

**What to expect**: You should measure a roughly 6.7x speedup over the numpy implementation. While this speedup is not mind-blowing,  it is encouraging for the following reasons:
* We are targeting floating point computation which low-power FPGAs are notoriously bad at. We have room perform more aggressive data type quantization techniques in order to push more throughput out of our FPGA (covered next).
* We are compiling a low frequency (100MHz) which is 1/6.6th of what the CPU is running at. With more frequency optimizations, it's not impossible to clock the FPGA at 250MHz (however this won't be covered during this lab).
* Lastly we are utilizing less than ~50% of the FPGA resources and only 1/4 of the available memory throughput on the FPGA, so we could potentially improve throughput significantly.

# Part 2: Fixed-Point Optimizations (30 pts)

In Part 1 you implemented an accelerator that performs computation on floating-point data. In Part 2 you'll use a fixed-point implementation that is a lot more compact and efficient.

We'll use `int8` for the weights, and `uint8` for the inputs since those are gray-scale pixels valued between 0 and 255. For the outputs and the offsets, we'll use `int32` values.

You will find under `hls/mmult_fixed/` a source skeletton to implement your fixed-point matrix multiply. To build your bitstream, you'll need to change the following line of `tcl/hts.tcl` from:
```tcl
set src_dir "../hls/mmult_float"
```

to:
```tcl
set src_dir "../hls/mmult_fixed"
```

To test your design on the board, you'll need to transfer your new fixed-point classifier overlay to the PYNQ file system with the following commands:
```bash
scp build/export/classifier.bit xilinx@192.168.2.99:/home/xilinx/pynq/bitstream/classifier_fixed.bit
scp tcl/classifier.tcl xilinx@192.168.2.99:/home/xilinx/pynq/bitstream/classifier_fixed.tcl
scp jupyter/classifier_2.ipynb xilinx@192.168.2.99:/home/xilinx/jupyter_notebooks/.
scp python/*.npy xilinx@192.168.2.99:/home/xilinx/jupyter_notebooks/.
```

Finally, once you've logged onto the iPython notebook server on the PYNQ, open the `classifier_2.ipynb` notebook, and execute your test program by clicking on **Kernel -> Restart & Run All**.

### Problem Statement

Implement a fixed-point linear classifier, by completing the `TODOs` in `hls/mmult_fixed/mmult_fixed.cpp` and `hls/mmult_fixed/mmult_fixed.h`. The batch size has been set to `8192` and should not be altered. Your design should pass the tests in `hls/mmult_fixed/mmult_test.cpp`.

You will also have to perform floating-point to fixed-point conversion of your learned weight and offset coefficients. In the training file under `python/mnist.py` modify the `SCALE` factor to achieve less than 20% validation error on fixed point inference. Re-running the `python/mnist.py` will produce updated `.npy` files that will then be copied onto the Zynq when you re-run the `scp` commands.

Report the following:
* (1) the fixed-point validation accuracy reported by `mnist.py` after you've tweaked the `SCALE` factor.
* (2) the design latency in cycles 
* (3) the overall device utilization (as Total per Resource).
* (4) your measured system speedup over the fixed-point CPU implementation
* (5) your measured classification accuracy on the 8k MNIST test sample

Also report the following:
* (6) how many multipliers are instantiated in your desing?
* (7) report the initiation interval of the matrix multiplication loop that you pipelined
* (8) given the number of multipliers in your design and input throughput via the AXI port, is the design bandwidth- or computer-limited?

**Hints**
* Again you will have to tweak your memory partitioning, tiling factor to optimize your kernel latency/throughput.
* Make sure that you close your AXI stream properly when you push data to it (see `push_stream()` calls in `mmult_float.cpp`). The `bool last` argument should be set to `true` on the last stream packet or the DMA drivers will hang when you try to test your design.
* You'll notice that in `mmult.h` we are using `ap_int` which are arbitrary precision integers, which is HLS' solution to providing integers of arbitrary width (so not just 1, 8, 16, 32, 64). Unfortunately the `union` type conversion trick does not work on `ap_ints`. Instead you'll need to do a bit of bit manipulation on the AXI raw values before converting to `ap_int`. The `mmult_test.cpp` file should provide a good reference on how data is packed and unpacked before being pushed or popped from AXI channels.
* By default HLS will implement 8-bit multipliers on hard `BRAM_18K` blocks. But the Zynq FPGA only contains 220 multipliers. If you want to allocate more multipliers, you can use the following directive which will tell HLS to synthesize multipliers using LUTs instead: `#pragma HLS RESOURCE variable=mult core=Mul_LUT`.

**What to expect**: In terms of latency as HLS reports, this design should achieve 400-500x improvement in batch-normalized latency over the first naive floating point design. On the board, you should see a roughly 10x improvement over Part 1's  FPGA over CPU speedup.

# Part 3: Open-ended design optimization (20 pts + 20 bonus pts)

You have now a mature understanding of the process of optimizing hardware, and adapting learning models to more efficiently execute inference on hardware (e.g. fixed point optimization).

### Problem Statement

You are given free range over how to improve either performance or the accuracy or both of your classifier.

You will receive full points if you can both improve performance and accuracy of your classifier by a significant margin (over 85% in accuracy, and achieve less than 4.5ms of FPGA inference time as measured on the board for 8k invocations). You are free to change the classifier algorithm. Bonus points will be awarded to very ambitious classifier implementations (e.g. neural networks).

This goes without saying: you are not allowed to train your model on test data.

**Hints**
To provide some guidance, you can implement one of the following optimizations:
* Image resizing to reduce bandwidth constraints
* Int4 computation to improve overall throughput and compute parallelism
* Weight re-training to recover accuracy loss
* Static weight and input pruning to improve throughput
