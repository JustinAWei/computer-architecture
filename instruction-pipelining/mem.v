`timescale 1ps/1ps

module memory(
    input clk,
    input [63:2]raddr0,
    output [31:0]rdata0,
    input [63:2]raddr1,
    output [31:0]rdata1,
    input wen,
    input [63:2]waddr,
    input [31:0]wdata);

    reg [31:0]data[0:2047];

    reg [63:2]mar0;
    reg [63:2]mar1;

    reg [31:0]mdr0;
    reg [31:0]mdr1;

    assign rdata0 = mdr0;
    assign rdata1 = mdr1;

    always @(posedge clk) begin
	    mar0 <= raddr0;
	    mdr0 <= data[mar0];
	    mar1 <= raddr1;
	    mdr1 <= data[mar1];
    end

    ////////////////////////////
    // Simulation only things //
    ////////////////////////////

    // initialize memory
    initial begin
        $readmemh("mem.hex",data);
    end

endmodule
