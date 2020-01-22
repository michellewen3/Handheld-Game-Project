; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on LM4F120 or TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 
Rem	EQU 0
Star EQU 9999

    IMPORT   ST7735_OutChar
    IMPORT   ST7735_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix

    AREA    |.text|, CODE, READONLY, ALIGN=2
		PRESERVE8
    THUMB

  

;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutDec

		PUSH {R2,R3,R4, LR}
		SUB SP, #4				;allocating the SP
		
		CMP R0, #10				;checking if single digit
		BLO print
		
		MOV R1, #10
		UDIV R2, R0, R1			;find quo
		MUL R1, R1, R2			;find rem
		SUB R0, R0, R1			;R0 has rem
		
		STR R0, [SP, #Rem]		;store onto stack
		MOV R0, R2				;R0 has quotient
		BL LCD_OutDec	
		
		LDR R0, [SP, #Rem]		;load from stack

print	ADD R0, R0, #0x30	
		BL ST7735_OutChar
		
		ADD SP, #4				;deallocate SP
		POP {R2,R3,R4, PC}

		BX  LR
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.001, range 0.000 to 9.999
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.000 "
;       R0=3,    then output "0.003 "
;       R0=89,   then output "0.089 "
;       R0=123,  then output "0.123 "
;       R0=9999, then output "9.999 "
;       R0>9999, then output "*.*** "
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutFix
	  PUSH{R4, LR}
	
	  MOV R1, #9999			;check if R0 greater than 9999
	  CMP R0, R1
	  BHI stars
	
	  MOV R1, #999		
	  CMP R0, R1
	  BGT dec
	
	  MOV R4, R0				
	  MOV R0, #0x30			
	  BL ST7735_OutChar
      MOV R0, #0x2E			
	  BL ST7735_OutChar
	
check						;check for number of zeroes needed
	  MOV R3, #100			
	  UDIV R3, R4, R3			
	  CMP R3, #0
      BEQ zero1
	  MOV R0, R4
	  BL LCD_OutDec
	  B done
	
zero1 					
	  MOV R0, #0x30
	  BL ST7735_OutChar
	  MOV R3, #10
	  UDIV R3, R4, R3
	  CMP R3, #0			
      BEQ zero2
	  MOV R0, R4
	  BL LCD_OutDec
	  B done
	
zero2						
      MOV R0, #0x30
	  BL ST7735_OutChar
	  MOV R0, R4
	  BL LCD_OutDec 
	  B done
	
dec						
      MOV R4, R0
      MOV R1, #1000
	  UDIV R2,R0,R1
	  MOV R3, R2
	  ADD R0, R2, #0x30
      BL ST7735_OutChar
      MOV R0, #0x2E	
	  BL ST7735_OutChar
	
	
	  MOV R1, #1000			
	  UDIV R2,R4,R1
	  MUL R2,R2,R1
	  SUBS R4,R4,R2			
	  B check					
    
stars							;print *.*** if value is greater than 9999
	  MOV R0, #0x2A				
	  BL ST7735_OutChar
	  MOV R0, #0x2E	
	  BL ST7735_OutChar
      MOV R0, #0x2A
	  BL ST7735_OutChar
	  MOV R0, #0x2A
	  BL ST7735_OutChar
	  MOV R0, #0x2A
	  BL ST7735_OutChar

done
	  POP {R4, PC}
      BX   LR
 
     ALIGN
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN                           ; make sure the end of this section is aligned
     END                             ; end of file
