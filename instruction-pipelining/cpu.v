`timescale 1ps/1ps

module main();


    // There shall be a clock
    wire clk;

    // ... and a counter that tracks cycles
    wire halt;             // set this wire to 1 to halt the processor
    counter cnt(clk,halt);

    // ... and memory
    wire [63:2] mem_raddr0;
    wire [31:0] mem_rdata0;
    wire [63:2] mem_raddr1;
    wire [31:0] mem_rdata1;
    wire mem_wen;
    wire [63:2] mem_waddr;
    wire [31:0] mem_wdata;
    memory mem(clk,mem_raddr0,mem_rdata0,mem_raddr1,mem_rdata1,mem_wen,mem_waddr,mem_wdata);

    // ... and registers
    wire [4:0] reg_raddr0;
    wire [63:0] reg_rdata0;
    wire [4:0] reg_raddr1;
    wire [63:0] reg_rdata1;
    wire reg_wen;
    wire [4:0] reg_waddr;
    wire [63:0] reg_wdata;
    registers regs(clk,reg_raddr0,reg_rdata0,reg_raddr1,reg_rdata1,reg_wen,reg_waddr,reg_wdata, halt);

    // Read After Write hazard handling
    wire RAW = R1_RAW || R2_RAW || E_RAW || M1_RAW || M2_RAW || WB_RAW;
    wire R1_RAW = (D_defined && D_valid && R1_valid) ? R1_WA == D_RA : 0;
    wire R2_RAW = (D_defined && D_valid && R2_valid) ? R2_WA == D_RA : 0;
    wire E_RAW = (D_defined && D_valid && E_valid) ? E_WA == D_RA : 0;
    wire M1_RAW = (D_defined && D_valid && M1_valid) ? M1_WA == D_RA : 0;
    wire M2_RAW = (D_defined && D_valid && M2_valid) ? M2_WA == D_RA : 0;
    wire WB_RAW = (D_defined && D_valid && WB_valid) ? WB_WA == D_RA : 0;

    /////////////////
    // Stall       //
    /////////////////

    wire F_flush = D_BranchTaken;
    wire D_flush = D_BranchTaken;
    wire D_BranchTaken = D_valid ? D_isB || RAW : 0;

    /////////////////
    // Fetch 1     //
    /////////////////

    reg [63:2]F1_pc = 62'h0000000000000000;
    reg F1_valid = 1;

    always @(posedge clk) begin
        if (F_flush) begin
            F1_pc <= branch_pc;
        end
        else if (F1_valid) F1_pc <= F1_pc+1;
    end

    assign mem_raddr0 = F1_pc;

    /////////////
    // Fetch 2 //
    /////////////

    reg [63:2]F2_pc;
    reg F2_valid = 0;

    always @(posedge clk) begin
        if (F_flush) begin
            F2_valid <= 0;
        end
        else begin
            F2_pc <= F1_pc;
            F2_valid <= F1_valid;
        end
    end

    ////////////
    // Decode //
    ////////////

    reg [63:2]D_pc;
    wire [31:0] D_ins = mem_rdata0;
    reg D_valid = 0;
    wire D_defined = !(D_ins === 32'bx || D_ins === 32'bz);

    // control
    wire D_isLdrb = D_ins[31:22] == 10'b0011100101;
    wire D_isAddi = D_ins[30:24] == 7'b0010001 && !(shift == 2'b10 || shift == 2'b11);
    wire D_isB = D_ins[31:26] == 6'b000101;
    wire D_sf = D_ins[31];

    // extended immediate for
    wire [21:10] imm12 = D_ins[21:10];
    wire [63:0] ldrb_offset = {52'b0, imm12};

    wire [1:0] shift = D_ins[23:22];
    wire [63:0] addi_imm = shift ? {40'd0, imm12, 12'd0} : {52'd0, imm12};

    wire [25:0] imm26 = D_ins[25:0];
    wire [63:2] b_offset = {{36{imm26[25]}}, imm26};
	wire [63:2] branch_pc = D_isB ? D_pc + b_offset : D_pc;

    wire [63:0] D_offset = D_isLdrb ? ldrb_offset :
    D_isAddi ? addi_imm : b_offset;
    wire [4:0] D_WA = D_ins[4:0];

    wire [9:5] D_RA = D_ins[9:5];
    // read from mem
    assign reg_raddr0 = D_RA;

    always @(posedge clk) begin
        if (D_flush) begin
            D_valid <= 0;
        end
        else begin
	            D_valid <= F2_valid;
            D_pc <= F2_pc;
        end
    end

    // R1 //
    reg [63:2]R1_pc;
    reg R1_valid = 0;

    reg R1_isAddi, R1_isB, R1_isLdrb;

    reg R1_sf;
    reg [63:0] R1_offset;
    reg [31:0] R1_WA, R1_RA;

    always @(posedge clk) begin
        if (D_flush) begin
            R1_valid <= 0;
        end
        else begin
            // control
            R1_isAddi <= D_isAddi;
            R1_isB <= D_isB;
            R1_isLdrb <= D_isLdrb;

            // sf
            R1_sf <= D_sf;

            // src B
            R1_offset <= D_offset;

            // save write address
            R1_WA <= D_WA;
            R1_RA <= D_RA;

            R1_valid <= D_valid && D_defined;
            R1_pc <= D_pc;
        end
    end

    // R2 //
    reg [63:2]R2_pc;
    reg R2_valid = 0;

    reg R2_isAddi, R2_isB, R2_isLdrb;

    reg R2_sf;
    reg [63:0] R2_offset;
    reg [31:0] R2_WA;
    reg [31:0] R2_RA;

    always @(posedge clk) begin
        R2_isAddi <= R1_isAddi;
        R2_isB <= R1_isB;
        R2_isLdrb <= R1_isLdrb;

        // sf
        R2_sf <= R1_sf;

        // src B
        R2_offset <= R1_offset;

        // save write address
        R2_WA <= R1_WA;
        R2_RA <= R1_RA;

        R2_valid <= R1_valid;
        R2_pc <= R1_pc;
    end

    // X //
    reg [63:2]E_pc;
    reg E_valid = 0;

    reg E_isAddi, E_isB, E_isLdrb;

    reg E_sf;
    reg [63:0] E_offset, E_data;
    reg [31:0] E_WA, E_RA;
    wire [63:0] E_reg = E_data;
    wire [63:0] result = E_reg + E_offset;
    wire [63:0] addi_result = (E_isAddi && !E_sf) ? {32'd0,result[31:0]} : result;

    wire [63:0]ldrb_base_addr = {2'b00, result[63:2]};
    wire [1:0]ldrb_mem_offset = result[1:0];

    wire [63:0] E_ALUResult = (E_isLdrb) ? ldrb_base_addr : addi_result;

    always @(posedge clk) begin
        // control
        E_isAddi <= R2_isAddi;
        E_isB <= R2_isB;
        E_isLdrb <= R2_isLdrb;
        E_data <= reg_rdata0;
        // sf
        E_sf <= R2_sf;

        // src B
        E_offset <= R2_offset;

        // save write address
        E_WA <= R2_WA;
        E_RA <= R2_RA;

	    E_valid <= R2_valid;
	    E_pc <= R2_pc;
    end

    assign mem_raddr1 = M1_ALUOut;

    // M1 //
    reg [63:2]M1_pc;
    reg M1_valid = 0;

    reg [63:0] M1_offset;
    reg M1_isAddi, M1_isB, M1_isLdrb;
    reg [63:0] M1_ALUOut;
    reg [31:0] M1_WA;

    always @(posedge clk) begin
        M1_ALUOut <= E_ALUResult;
        // control
        M1_isAddi <= E_isAddi;
        M1_isB <= E_isB;
        M1_isLdrb <= E_isLdrb;

        M1_offset <= ldrb_mem_offset;
        // save write address
        M1_WA <= E_WA;

        M1_valid <= E_valid;
        M1_pc <= E_pc;
    end

    // M2 //
    reg [63:2]M2_pc;
    reg [63:0] M2_offset;
    reg M2_valid = 0;
    reg [63:0] M2_ALUOut;

    reg M2_isAddi, M2_isB, M2_isLdrb;

    reg [31:0] M2_WA;

    always @(posedge clk) begin
        M2_ALUOut <= M1_ALUOut;
        // control
        M2_isAddi <= M1_isAddi;
        M2_isB <= M1_isB;
        M2_isLdrb <= M1_isLdrb;

        M2_offset <= M1_offset;
        // save write address
        M2_WA <= M1_WA;

        M2_valid <= M1_valid;
        M2_pc <= M1_pc;
    end


    ////////
    // WB //
    ////////
    reg [63:2]WB_pc;
    reg [63:0] WB_offset;
    reg WB_valid = 0;
    reg [63:0] WB_ALUOut;

    reg WB_isAddi, WB_isB, WB_isLdrb;

    reg [31:0] WB_WA;

    always @(posedge clk) begin
        WB_ALUOut <= M2_ALUOut;
        // control
        WB_isAddi <= M2_isAddi;
        WB_isB <= M2_isB;
        WB_isLdrb <= M2_isLdrb;

        WB_offset <= M2_offset;
        // save write address
        WB_WA <= M2_WA;

        WB_valid <= M2_valid;
        WB_pc <= M2_pc;
    end

    wire [63:0] WB_result = WB_isLdrb ?
    (WB_offset == 0) ? mem_rdata1[7:0] :
    (WB_offset == 1) ? mem_rdata1[15:8] :
    (WB_offset == 2) ? mem_rdata1[23:16] : mem_rdata1[31: 24] : WB_ALUOut;

    assign SP_block_write = WB_isLdrb && reg_waddr == 31;
    assign reg_wen = WB_valid && (WB_isLdrb || WB_isAddi) && !SP_block_write;
    assign reg_waddr = WB_valid ? WB_WA : 0;
    assign reg_wdata = WB_valid ? WB_result : 0;
    assign halt = (WB_valid && !(WB_isB || WB_isAddi || WB_isLdrb)) || WB_pc >= 2048;

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

endmodule
