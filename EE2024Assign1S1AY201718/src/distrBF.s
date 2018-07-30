 	.syntax unified
 	.cpu cortex-m3
 	.thumb
 	.align 2
 	.global	distrBF
 	.thumb_func

@ EE2024 Assignment 1, Sem 1, AY 2017/18
@ (c) CK Tham, ECE NUS, 2017
@ Project Group 10 Friday	:
@ Name: Ong Yi Fan, Lim Sunsheng

@ R0	T
@ R1 	dij
@ R2 	Dj
@ R3 	n_res
@ R4 	T-counter
@ R5 	j-counter
@ R6 	N
@ R7 	Retrieve Input Address
@ R8 	Retrieve Output Value / Temp Address Holder
@ R9 	Sum Di
@ R10	Minimum Di
@ R11	Minimum node j

distrBF:
	PUSH {R4, R5, R6, R7, R8, R9, R10, R11, R14}

	LDR R6, [R3] //Set N
	MOV R4, #0 //Set T = 0

	Tloop:
		MOV R5, #0 //Set j = 0
@ Set first sum Di (first minimum)
		MOV R7, R1
		LDR R8, [R7] //Retrieve 1st dij
		MOV R9, R8

		MOV R7, R2
		LDR R8, [R7] //Retrieve 1st Dj
		ADD R9, R8

		MOV R10, R9
		MOV R11, R5

		ADD R5, #1

		Nloop:
			MOV R9, #0 //Set/Reset Min Di

			MOV R7, R1
			BL RETRIEVE //Retrieve dij
			MOV R9, R8

			MOV R7, R2
			BL RETRIEVE //Retrieve Dj
			ADD R9, R8

			CMP R9, R10 //Find min Di
			ITT LT
			MOVLT R10, R9
			MOVLT R11, R5

			ADD R5, #1
			CMP R5, R6
			BLT Nloop

		ADD R11, #1
		STR R10, [R3], #4
		STR R11, [R3], #4

		ADD R4, #1
		CMP R4, R0
		BLT Tloop

	POP {R4, R5, R6, R7, R8, R9, R10, R11, R14}
	BX	LR

@ Subroutine RETRIEVE
RETRIEVE:
@ write subroutine code here
@ Formula: (T*N+j)*4
	MUL R8, R4, R6
	ADD R8, R5
	LSL R8, #2
	ADD R8, R7
	LDR R8, [R8]

	BX LR

	NOP
	.end
