[BITS 64]

; Import + Export
    GLOBAL isPrime
    GLOBAL check_block

section .data

NULL                EQU 0

section .text

 ; extern char isPrime(uint64_t n)
isPrime:
    push rdx
    push rcx

    ; removed for SPEED
	;cmp rdi, 2
	;jl short .notPrime    ; 0 and 1 are not prime numbers
    ;je short .prime       ; 2 is a prime number(the only even)

	mov rcx, 2
	.next:
		xor rdx, rdx ; must be zero for division
		mov rax, rdi
		div rcx
		
		or rdx, rdx
		jz short .notPrime

        ; if (n*n > rdi): rdi->prime
        mov rax, rcx
        mul rcx
        or rdx, rdx
        jnz .prime
        cmp rax, rdi
        jg .prime
		
		inc rcx
		cmp rcx, rdi
        jl short .next
	
	.prime:
	mov rax, 1
    pop rcx
    pop rdx
	ret
	
	.notPrime:
	xor rax, rax
    pop rcx
    pop rdx
	ret

check_block: ; rdi=(uint64)start, (uint64)rsi=count, rdx=(uint64*)array
    push rbp
    mov rbp, rsp

    push rdi
    push rdx
    push R8 ; for return value

    xor R8, R8 ; clear return value
    cmp rdi, 2
    jne .startNotTwo
    inc R8  ; two is prime and even
    inc rdi
    mov QWORD [rdx], 2
    add rdx, 8
    jmp .startNrIsOdd ; skip odd/even checks because we know it was even and is now odd, no need to check again
    .startNotTwo:

    mov rax, rdi
    and rax, 1
    test rax, rax
    jnz .startNrIsOdd
    inc rdi
    .startNrIsOdd:


    add rsi, rdi
    .next:
        call isPrime
        or eax, eax
        jz short .notPrime
            inc R8
            mov QWORD [rdx], rdi
            add rdx, 8
        .notPrime:

        add rdi, 2
        cmp rdi, rsi
        jl short .next

    mov rax, R8 ; return value
    pop R8
    pop rdx
    pop rdi

    mov rsp, rbp ; restore stack pointer + free local variables
    pop rbp
    ret