
.code

    Jump64 PROC

		mov qword ptr [rsp+10h], rbp  ; save base pointer
		mov rax,  qword ptr [rsp+20h] ; jump address
	    jmp rax
	    ret

    Jump64 ENDP

END
