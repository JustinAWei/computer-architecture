`timescale 1ps/1ps

module main();

    // There shall be a clock
    wire clk;

    // PC
    reg [63:0]pc = 64'h0000000000000000;

    // Memory -- you can only use one read port.
    //           your design can't use mem[...]
    //           you have to assign raddr in your design
    //           rdata will contain the byte at that address
    reg [7:0]mem[0:1023];

    // registers
    reg [63:0]regs[0:30];
    reg [63:0] XSP = 0;

    // state
    reg [3:0]state = 0;

    // defined
    wire isDefined = (state < 4 || isAddi || isB || isLdrb) && !((state==4 && isAddi && (shift == 2'b10 || shift == 2'b11)) || ins === 32'bx || ins === 32'bz);
    wire isUndefined = ~isDefined;

    // instruction
    reg [31:0]ins = 32'b0;

    // ldrb
    wire [31:22] isLdrb = ins[31:22] == 10'b0011100101;
    wire [21:10] ldrb_imm12 = ins[21:10];
    wire [9:5] rn = ins[9:5];
    wire [4:0] rt = ins[4:0];
    wire [63:0] offset = {52'b0, ldrb_imm12};
    wire [63:0] addr = (rn == 31) ? XSP : regs[rn];
    wire [32:0] ldrb_result = {24'b0, rdata};

    // addi
    wire isAddi = ins[30:24] == 7'b0010001;
    wire [4:0]rd = ins[4:0];
    wire [63:0] op1 = isAddi ? (rn==31 ? XSP : regs[rn]) : 0;
    wire [11:0] addi_imm12 = ins[21:10];
    wire [1:0] shift = ins[23:22];
    wire [63:0] imm = shift ? {40'd0, addi_imm12, 12'd0} : {52'd0, addi_imm12};
    wire [1:0] sf = ins[31:30];
    wire [63:0] result = imm + op1;
    wire [63:0] addi_result = sf ? result : {32'd0,result[31:0]};

    // B
    wire isB = ins[31:26] == 6'b000101;
    wire [25:0] imm26 = ins[25:0];
    wire [63:0] b_offset = {{36{imm26[25]}}, imm26, 2'd0};

    // PC
    wire updatePC = isDefined;
    wire [63:0] nextPC = isB ? pc+b_offset : pc+4;

    // read
    wire [63:0] raddr =
    (state == 0) ? pc :
    (state == 1) ? pc + 1:
    (state == 2) ? pc + 2:
    (state == 3) ? pc + 3:
    (isLdrb) ? addr + offset: 0;
    wire [7:0]rdata = mem[raddr];

    // write
    wire wen = isAddi || (isLdrb && rt != 31);
    wire [4:0]waddr = isAddi ? rd : rt;
    wire [63:0] wdata = isAddi ? addi_result : ldrb_result;

    always @(posedge clk) begin
        if (isUndefined) begin
        $write("done at %d\n",pc);
        $display(regs[0]);
        $display(regs[1]);
        $display(regs[2]);
        $display(regs[3]);
        $display(regs[4]);
        $display(regs[5]);
        $display(regs[6]);
        $display(regs[7]);
        $display(regs[8]);
        $display(regs[9]);
        $display(regs[10]);
        $display(regs[11]);
        $display(regs[12]);
        $display(regs[13]);
        $display(regs[14]);
        $display(regs[15]);
        $display(regs[16]);
        $display(regs[17]);
        $display(regs[18]);
        $display(regs[19]);
        $display(regs[20]);
        $display(regs[21]);
        $display(regs[22]);
        $display(regs[23]);
        $display(regs[24]);
        $display(regs[25]);
        $display(regs[26]);
        $display(regs[27]);
        $display(regs[28]);
        $display(regs[29]);
        $display(regs[30]);
        $finish;
        end

        if(state == 0) ins[7:0] <= rdata;
        else if(state == 1) ins[15:8] <= rdata;
        else if(state == 2) ins[23:16] <= rdata;
        else if(state == 3) ins[31:24] <= rdata;

        else if(state == 4) begin
            if (wen) begin
                if(waddr==31) begin
                    XSP <= wdata;
                end
                else begin
                    regs[waddr] <= wdata;
                end
            end
            if (updatePC) pc <= nextPC;
            // $display("%h %h %h %h", state, ins, raddr, rdata);
            // $display(wen, waddr, wdata);
            // $display("%h %h", pc, nextPC);
            // $display("%h %h %h", isAddi, isB, isLdrb);
        end
        state <= state + 1;
        if(state >= 4) state <= 0;
    end

    ////////////////////////////
    // Simulation only things //
    ////////////////////////////

    // Simulation only constructs to dump debug information
    initial begin
        $dumpfile("cpu.vcd");
        $dumpvars(0,main);
    end


    // Simulation only constructs to simulate the clock
    reg theClock = 0;

    assign clk = theClock;

    always begin
        #500;
        theClock = !theClock;
    end

    // initialize memory
    initial begin
        $readmemh("mem.hex",mem);
    end

    // initialize registers
    initial begin
        $readmemh("regs.hex",regs);
    end

endmodule
