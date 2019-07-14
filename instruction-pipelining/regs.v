`timescale 1ps/1ps

module registers(
    input clk,
    input [4:0]raddr0,
    output [63:0]rdata0,
    input [4:0]raddr1,
    output [63:0]rdata1,
    input wen,
    input [4:0]waddr,
    input [63:0]wdata,
    input halt
);

    reg [63:0]data[0:31];

    reg [4:0]mar0;
    reg [4:0]mar1;

    reg [63:0]mdr0;
    reg [63:0]mdr1;

    assign rdata0 = mdr0;
    assign rdata1 = mdr1;
    wire [31:0] r0 = data[0];
    wire [31:0] r1 = data[1];
    wire [31:0] r2 = data[2];
    wire [31:0] r3 = data[3];
    wire [31:0] r4 = data[4];
    wire [31:0] r5 = data[5];
    wire [31:0] r6 = data[6];
    wire [31:0] r7 = data[7];
    wire [31:0] r8 = data[8];
    wire [31:0] r9 = data[9];
    wire [31:0] r10 = data[10];
    wire [31:0] r11 = data[11];
    wire [31:0] r12 = data[12];
    wire [31:0] r13 = data[13];
    wire [31:0] r14 = data[14];
    wire [31:0] r15 = data[15];
    wire [31:0] r16 = data[16];
    wire [31:0] r17 = data[17];
    wire [31:0] r18 = data[18];
    wire [31:0] r19 = data[19];
    wire [31:0] r20 = data[20];
    wire [31:0] r21 = data[21];
    wire [31:0] r22 = data[22];
    wire [31:0] r23 = data[23];
    wire [31:0] r24 = data[24];
    wire [31:0] r25 = data[25];
    wire [31:0] r26 = data[26];
    wire [31:0] r27 = data[27];
    wire [31:0] r28 = data[28];
    wire [31:0] r29 = data[29];
    wire [31:0] r30 = data[30];

    always @(posedge clk) begin
	    mar0 <= raddr0;
	    mdr0 <= data[mar0];
	    mar1 <= raddr1;
	    mdr1 <= data[mar1];

        if (wen) begin
            data[waddr] <= wdata;
        end
        if (halt) begin
            // $write("done at %d\n",pc);
            $display(data[0]);
            $display(data[1]);
            $display(data[2]);
            $display(data[3]);
            $display(data[4]);
            $display(data[5]);
            $display(data[6]);
            $display(data[7]);
            $display(data[8]);
            $display(data[9]);
            $display(data[10]);
            $display(data[11]);
            $display(data[12]);
            $display(data[13]);
            $display(data[14]);
            $display(data[15]);
            $display(data[16]);
            $display(data[17]);
            $display(data[18]);
            $display(data[19]);
            $display(data[20]);
            $display(data[21]);
            $display(data[22]);
            $display(data[23]);
            $display(data[24]);
            $display(data[25]);
            $display(data[26]);
            $display(data[27]);
            $display(data[28]);
            $display(data[29]);
            $display(data[30]);
            $display("\n");
            $finish;
        end
    end

    ////////////////////////////
    // Simulation only things //
    ////////////////////////////

    // initialize memory
    initial begin
        $readmemh("regs.hex",data);
    end

endmodule
