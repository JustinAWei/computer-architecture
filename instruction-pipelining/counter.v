module counter(
    input clk,
    input halt
);


    reg [63:0] count = 0;

    always @(posedge clk) begin
        if (halt) begin
            // $display("#cycles %d",count);
            // $display("done");
        end

        count <= count + 1;
    end

endmodule
