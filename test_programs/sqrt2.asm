


start:
    addi x29, x0, 4096

    ; x1 = numerator
    ; x2 = denominator

    addi x1, x0, 1
    addi x1, x0, 1
    addi x15, x0, 1

    addi x10, x0, 15

loop:
    beq x10, x0, done

    ; x3, x4 = temps
    sll x3, x2, x15
    add x3, x3, x1
    add x4, x1, x2

    addi x1, x3, 0
    addi x2, x4, 0

    addi x10, x10, -1
    j loop

done:
    j done


