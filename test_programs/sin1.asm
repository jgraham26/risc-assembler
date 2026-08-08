; x1 = sum numerator
; x2 = sum denominator
; x3 = factorial
; x4 = sign
; x5 = current odd number
; x6 = temporary
; x7 = temporary
; x8 = term numerator
; x9 = term denominator
; x10-20 = scratch registers
; x29 = stack pointer
; x31 = return address

; how to enter frame:
; jal [label] - jump and link to label, saves return address in x31
; sw x31, 0(x29) - save return address on stack
; addi x29, x29, -4 - move stack pointer down to make space for return address

; to return:
; addi x29, x29, 4 - move stack pointer up to remove return address from stack
; lw x31, 0(x29) - load return address from stack
; jr x31 - jump to return address




start:
    addi x29, x29, 1024 ; allocate stack space

addi x1, x0, 1        ; x1  = Numerator N = 1
    addi x2, x0, 1        ; x2  = Denominator D = 1
    addi x20, x0, 1       ; x20 = current factorial index n = 1
    addi x21, x0, -1      ; x21 = sign = -1 (next operation is - 1/3!)
    addi x22, x0, 11      ; x22 = max n = 11 (highest term 11!)

taylor_loop:
    beq x20, x22, taylor_done ; If n == 11, finished

    ; 1. Compute term multiplier M = (n + 1) * (n + 2)
    addi x10, x20, 1      ; x10 = n + 1
    addi x11, x20, 2      ; x11 = n + 2
    jal mul               ; x12 = (n + 1) * (n + 2) = M
    addi x23, x12, 0      ; Save M in x23

    ; 2. Update Denominator: D = D * M
    addi x10, x2, 0       ; x10 = D
    addi x11, x23, 0      ; x11 = M
    jal mul               ; x12 = D * M
    addi x2, x12, 0       ; x2 = D_new

    ; 3. Update Numerator: N = N * M + sign
    addi x10, x1, 0       ; x10 = N
    addi x11, x23, 0      ; x11 = M
    jal mul               ; x12 = N * M
    add x1, x12, x21      ; x1 = N_new = (N * M) + sign

    ; 4. Toggle sign for next term ( -1 -> +1 -> -1 ... )
    sub x21, x0, x21      ; sign = 0 - sign

    ; 5. Increment n by 2 (1 -> 3 -> 5 -> 7 -> 9 -> 11)
    addi x20, x20, 2
    j taylor_loop

taylor_done:
    j taylor_done



; returns in x12 = x10 * x11
mul:
    addi x29, x29, -4 ; allocate stack space for return address
    sw x31, 0(x29) ; save return address on stack

    addi x12, x0, 0 ; initialize result to 0

mul_loop:
    beq x11, x0, mul_loop_done ; if multiplier is 0, exit loop
    ; check if number is even or odd
    andi x13, x11, 1 ; check if multiplier is odd
    beq x13, x0, mul_shift
    add x12, x12, x10 ; x12 += x10

mul_shift:
    addi x13, x0, 1 ; set x13 to 1
    sll x10, x10, x13 ; x10 <<= 1 (multiply by 2)
    srl x11, x11, x13 ; x11 >>= 1 (divide by 2)
    j mul_loop ; repeat loop

mul_loop_done:
    lw x31, 0(x29) ; load return address from stack
    addi x29, x29, 4 ; deallocate stack space for return address
    jr x31 ; return to caller


; returns in x12 = x10 / x11, x13 = x10 % x11
div:
    addi x29, x29, -4      ; Allocate stack space
    sw x31, 0(x29)         ; Save return address

    addi x12, x0, 0        ; Initialize quotient = 0
    addi x13, x0, 0        ; Initialize remainder = 0

    beq x11, x0, div_done  ; If divisor is 0, exit
    beq x10, x0, div_done  ; If dividend is 0, exit

    ; x14 acts as our moving bitmask. Start at bit 31 (0x80000000)
    addi x14, x0, 1
    addi x17, x0, 31
    sll x14, x14, x17      ; x14 = 1 << 31
    addi x17, x0, 1        ; initialize shift amount for *2 and /2 operations

div_loop:
    ; 1. Shift remainder left by 1
    sll x13, x13, x17       ; remainder <<= 1

    ; 2. Isolate the current dividend bit using our mask
    and x16, x10, x14      ; x16 = dividend & mask
    beq x16, x0, skip_or   ; If the bit is 0, skip adding 1 to remainder
    addi x13, x13, 1       ; remainder |= 1
skip_or:

    ; 3. Core Condition: if (remainder >= divisor)
    slt x15, x13, x11      ; x15 = 1 if remainder < divisor
    bne x15, x0, div_loop_inc ; If true, skip subtraction and quotient update

    ; 4. Update Remainder and Quotient
    sub x13, x13, x11      ; remainder -= divisor
    or x12, x12, x14       ; quotient |= current bitmask

div_loop_inc:
    ; 5. Shift our loop mask right to target the next bit down
    srl x14, x14, x17       ; mask >>= 1
    
    ; 6. Exit Condition: If mask hits 0, we finished all 32 bits
    beq x14, x0, div_done  ; If mask == 0, exit loop
    beq x0, x0, div_loop   ; Otherwise, loop back unconditionally

div_done:
    lw x31, 0(x29)         ; Restore return address
    addi x29, x29, 4       ; Deallocate stack space
    jr x31                 ; Return to caller
