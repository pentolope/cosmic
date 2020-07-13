def convertTabToSpace(s):
    return ' '.join(s.split('\t'))
def removeDupeSpacesToNew(s):
    return ' '.join(filter(lambda v:len(v)!=0,s.split(' ')))
def removeDupeNewlineToNew(s):
    return '\n'.join(filter(lambda v:len(v)!=0,s.split('\n')))
def removeSpaceNextToNewline(s):
    return '\n'.join((  '\n'.join(s.split(' \n'))  ).split('\n '))
def removeBeginningSpaceOrNewlineAndEnsureNewlineAtEnd(s):
    s2 = s[:]
    if len(s2)!=0:
        if s2[0]==' ':
            s2=s2[1:]
    if len(s2)!=0:
        if s2[0]=='\n':
            s2=s2[1:]
    if len(s2)!=0:
        if s2[-1]!='\n':
            s2=s2+'\n'
    return s2
def singleStageNormalizeString(s):
    return removeBeginningSpaceOrNewlineAndEnsureNewlineAtEnd(
            removeSpaceNextToNewline(
                removeDupeNewlineToNew(
                    removeDupeSpacesToNew(
                        convertTabToSpace(
                            s
                            )
                        )
                    )
                )
            )
def normalizeString(s):
    prevString = s
    newString = singleStageNormalizeString(s)
    while newString!=prevString:
        prevString = newString
        newString = singleStageNormalizeString(newString)
    return newString



# this is not in normalizeString(), it's just for me to use sometimes
def removeC_comments(s):
    os = ''
    comL = False
    comB = False
    for i in range(len(s)):
        if s[i:i+2]=='//' and not comB:
            comL=True
        if s[i:i+2]=='/*' and not comL:
            comB=True
        if s[i-2:i]=='*/' and comB and not comL:
            comB=False
        if s[i:i+1]=='\n' and comL:
            comL=False
        if (not comB) and (not comL):
            os+=s[i]
    return os


instructionListWithPrefix = [
'PHIS %',
'PHIE %',
'STPI % #',
'ERR_',
'DEPL',
'PEPH',
'NOP_',
'PU1_ %',
'PU2_ % %',
'PUA1 %',
'PUA2 % %',
'POP1 %',
'POP2 % %',
'BL1_ % $',
'RL1_ % #',
'RL2_ % % !',
'CALL % %',
'RET_',
'STPA % #',
'STPS % #' ,
'STWN % $',
'STRN % $',
'STWV % $',
'STRV % $',
'ALOC',
'ALCR % #',
'STOF % % #',
'AJMP % %',
'CJMP % % %',
'JJMP % % :',
'JTEN @',
'JEND',
'MOV_ % %',
'AND_ % % %',
'OR__ % % %',
'XOR_ % % %',
'SSUB % % %',
'ADDN % % %',
'ADDC % % %',
'SUBN % % %',
'SUBC % % %',
'MULS % %',
'MULL % %',
'DIVM % %',
'SHFT % %',
'BSWP % %',
'MWWN % % %',
'MRWN % % %',
'MWWV % % %',
'MRWV % % %',
'MWBN % %',
'MRBN % %',
'MWBV % %',
'MRBV % %',
'D32U',
'R32U',
'D32S',
'R32S',
'FCST $ # :',
'FCEN',
'SYRB %',
'SYRW %',
'SYRD % %',
'SYRE',
'NSNB !',
'NSCB !',
'NSNW !',
'NSCW !',
'ZNXB !',
'ZNXW !',
'BYTE $',
'WORD #',
'DWRD !',
'SYDB',
'SYDW',
'SYDD',
'SYDE',
'LOFF !',
'SYCB $',
'SYCW #',
'SYCD !',
'SYC0',
'SYC1',
'SYC2',
'SYC3',
'SYC4',
'SYC5',
'SYC6',
'SYC7',
'SYC8',
'SYC9',
'SYCX',
'SYCY',
'SYCA',
'SYCU',
'SYCO',
'SYCQ',
'SYCC',
'SYCN',
'SYCM',
'SYCZ',
'SYCS',
'SYCT',
"SYCL @", # the label is symbolic and is used to discern between multiple labels
"LABL :", # the label is symbolic and is used to discern between multiple labels

"INSR &" # the & prefix contains 1 hex character
]


dictionaryOfInitializationContents = {
"IB_stack_dupe_word":"""

POP1 %C
PU2_ %C %C

""",


"IB_stack_dupe_dword":"""

POP2 %C %2
PU2_ %2 %C
PU2_ %2 %C

""",


"IB_stack_swp_11":"""

POP2 %3 %4
PU2_ %3 %4

""",


"IB_stack_swp_12":"""

POP2 %5 %6
POP1 %3
PU2_ %6 %5
PU1_ %3

""",


"IB_stack_swp_21":"""

POP1 %5
POP2 %3 %4
PU1_ %5
PU2_ %4 %3

""",


"IB_stack_swp_22":"""

POP2 %3 %4
POP2 %5 %6
PU2_ %4 %3
PU2_ %6 %5

""",


"IB_CJMP_list_item":"""

RL1_ %2 #0000
XOR_ %4 %3 %2
SYRD %A %B
SYCL @00000000
SYRE
CJMP %A %B %4

""",


"IB_STPA":"""

STPA %3 #0000
BL1_ %4 $00
PU2_ %4 %3

""",


"IB_i_16add":"""

POP2 %2 %C
ADDN %3 %C %2
PU1_ %3

""",


"IB_i_32add":"""

POP2 %3 %4
POP2 %C %2
ADDC %5 %C %3
ADDN %6 %4 %2
ADDN %6 %6 %F
PU2_ %6 %5

""",


"IB_i_16sub":"""

POP2 %2 %C
SUBN %3 %C %2
PU1_ %3

""",


"IB_i_32sub":"""

POP2 %3 %4
POP2 %C %2
BL1_ %5 $01
SSUB %5 %C %3
SSUB %5 %2 %4
PU2_ %C %2

""",


"IB_i_16mul":"""

POP2 %2 %C
MULS %C %2
PU1_ %C

""",


"IB_i_32mul":"""

POP2 %C %2
POP2 %E %F
MULL %C %2
PU2_ %F %E

""",


"IB_i_16div_u_u":"""

POP2 %2 %C
DIVM %C %2
PU1_ %C

""",


"IB_i_16mod_u_u":"""

POP2 %2 %C
DIVM %C %2
PU1_ %2

""",


"IB_i_16div_s_s":"""

POP2 %2 %C
RL1_ %9 #8000
RL1_ %3 #FFFF
MOV_ %4 %3
AND_ %7 %C %9
ADDC %7 %7 %9
MULS %3 %F
MOV_ %5 %F
XOR_ %C %C %3
ADDN %C %C %5
AND_ %7 %2 %9
ADDC %7 %7 %9
MULS %4 %F
MOV_ %6 %F
XOR_ %2 %2 %4
ADDN %2 %2 %6
XOR_ %3 %3 %4
XOR_ %5 %5 %6
DIVM %C %2
XOR_ %C %C %3
ADDN %C %C %5
PU1_ %C

""",


"IB_i_16mod_s_s":"""

POP2 %2 %C
RL1_ %9 #8000
RL1_ %3 #FFFF
MOV_ %4 %3
AND_ %7 %C %9
ADDC %7 %7 %9
MULS %3 %F
MOV_ %5 %F
XOR_ %C %C %3
ADDN %C %C %5
AND_ %7 %2 %9
ADDC %7 %7 %9
MULS %4 %F
MOV_ %6 %F
XOR_ %2 %2 %4
ADDN %2 %2 %6
DIVM %C %2
XOR_ %2 %2 %3
ADDN %2 %2 %5
PU1_ %2

""",




"IB_i_32div_u_u":"""

POP2 %2 %3
POP2 %4 %5
D32U
PU2_ %9 %8

""",


"IB_i_32mod_u_u":"""

POP2 %2 %3
POP2 %4 %5
R32U
PU2_ %7 %6

""",


"IB_i_32div_s_s":"""

POP2 %2 %3
POP2 %4 %5
D32S
PU2_ %9 %8

""",


"IB_i_32mod_s_s":"""

POP2 %2 %3
POP2 %4 %5
R32S
PU2_ %7 %6

""",


"IB_comp_eq_int":"""

POP2 %2 %C
XOR_ %3 %C %2
BL1_ %4 $00
SSUB %4 %3 %4
BL1_ %3 $01
XOR_ %4 %3 %4
PU1_ %4

""",


"IB_comp_eq_long":"""

POP2 %C %2
POP2 %3 %4
XOR_ %5 %2 %4
XOR_ %6 %C %3
OR__ %5 %6 %5
BL1_ %7 $00
SSUB %7 %5 %7
BL1_ %6 $01
XOR_ %7 %6 %7
PU1_ %7

""",


"IB_comp_neq_int":"""

POP2 %2 %C
XOR_ %3 %C %2
BL1_ %4 $00
SSUB %4 %3 %4
PU1_ %4

""",


"IB_comp_neq_long":"""

POP2 %C %2
POP2 %3 %4
XOR_ %5 %2 %4
XOR_ %6 %C %3
OR__ %5 %6 %5
BL1_ %7 $00
SSUB %7 %5 %7
PU1_ %7

""",


"IB_comp_geq_int_s":"""

POP2 %2 %C

RL1_ %3 #8000
XOR_ %C %C %3
XOR_ %2 %2 %3

BL1_ %3 $01
SSUB %3 %C %2
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3


""",


"IB_comp_geq_long_s":"""

POP2 %C %2
POP2 %3 %4

RL1_ %5 #8000
XOR_ %2 %2 %5
XOR_ %4 %4 %5

BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_g_int_s":"""

POP2 %2 %C

RL1_ %3 #8000
XOR_ %C %C %3
XOR_ %2 %2 %3

BL1_ %3 $01
SSUB %3 %2 %C
PU1_ %3

""",


"IB_comp_g_long_s":"""

POP2 %C %2
POP2 %3 %4

RL1_ %5 #8000
XOR_ %2 %2 %5
XOR_ %4 %4 %5

BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %2
PU1_ %5

""",


"IB_comp_leq_int_s":"""

POP2 %2 %C

RL1_ %3 #8000
XOR_ %C %C %3
XOR_ %2 %2 %3

BL1_ %3 $01
SSUB %3 %2 %C
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3

""",


"IB_comp_leq_long_s":"""

POP2 %C %2
POP2 %3 %4

RL1_ %5 #8000
XOR_ %2 %2 %5
XOR_ %4 %4 %5

BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %3
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_l_int_s":"""

POP2 %2 %C

RL1_ %3 #8000
XOR_ %C %C %3
XOR_ %2 %2 %3

BL1_ %3 $01
SSUB %3 %C %2
PU1_ %3

""",


"IB_comp_l_long_s":"""

POP2 %C %2
POP2 %3 %4

RL1_ %5 #8000
XOR_ %2 %2 %5
XOR_ %4 %4 %5

BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
PU1_ %5

""",


"IB_comp_geq_int_u":"""

POP2 %2 %C
BL1_ %3 $01
SSUB %3 %C %2
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3

""",


"IB_comp_geq_long_u":"""

POP2 %C %2
POP2 %3 %4
BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_g_int_u":"""

POP2 %2 %C
BL1_ %3 $01
SSUB %3 %2 %C
PU1_ %3

""",


"IB_comp_g_long_u":"""

POP2 %C %2
POP2 %3 %4
BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %2
PU1_ %5

""",


"IB_comp_leq_int_u":"""

POP2 %2 %C
BL1_ %3 $01
SSUB %3 %2 %C
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3

""",


"IB_comp_leq_long_u":"""

POP2 %C %2
POP2 %3 %4
BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %3
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_l_int_u":"""

POP2 %2 %C
BL1_ %3 $01
SSUB %3 %C %2
PU1_ %3

""",


"IB_comp_l_long_u":"""

POP2 %C %2
POP2 %3 %4
BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
PU1_ %5

""",


"IB_b_and_int":"""

POP2 %2 %C
AND_ %3 %2 %C
PU1_ %3

""",


"IB_b_and_long":"""

POP2 %C %2
POP2 %3 %4
AND_ %5 %C %3
AND_ %6 %2 %4
PU2_ %6 %5

""",


"IB_b_or_int":"""

POP2 %2 %C
OR__ %3 %2 %C
PU1_ %3

""",


"IB_b_or_long":"""

POP2 %C %2
POP2 %3 %4
OR__ %5 %C %3
OR__ %6 %2 %4
PU2_ %6 %5

""",


"IB_b_xor_int":"""

POP2 %2 %C
XOR_ %3 %2 %C
PU1_ %3

""",


"IB_b_xor_long":"""

POP2 %C %2
POP2 %3 %4
XOR_ %5 %C %3
XOR_ %6 %2 %4
PU2_ %6 %5

""",


"IB_b_not_int":"""

POP1 %C
RL1_ %3 #FFFF
XOR_ %2 %3 %C
PU1_ %2

""",


"IB_b_not_long":"""

POP2 %C %2
RL1_ %5 #FFFF
XOR_ %3 %C %5
XOR_ %4 %2 %5
PU2_ %4 %3


""",


"IB_logic_and_with_jmp":"""

INSR &0
POP1 %C
PU1_ %C
SYRD %3 %4
SYCL @00000000
SYRE
CJMP %3 %4 %C
POP1 %C
INSR &1
LABL :00000000

""",


"IB_logic_or_with_jmp":"""

INSR &0
POP1 %C
BL1_ %2 $01
XOR_ %C %2 %C
PU1_ %C
SYRD %3 %4
SYCL @00000000
SYRE
CJMP %3 %4 %C
INSR &1
POP2 %C %2
BL1_ %2 $01
XOR_ %C %2 %C
PU1_ %C
LABL :00000000

""",


"IB_char_s_TO_int_s":"""

POP1 %C
BL1_ %2 $80
AND_ %3 %2 %C
RL1_ %2 #01FE
MULS %3 %2
OR__ %2 %C %3
PU1_ %2

""",


"IB_int_s_TO_long_s":"""

POP1 %C
ADDC %2 %C %C
RL1_ %2 #FFFF
MULS %2 %F
PU2_ %2 %C

""",


"IB_int_u_TO_long":"""

POP1 %C
BL1_ %2 $00
PU2_ %2 %C

""",


"IB_long_TO_int":"""

POP2 %C %2
PU1_ %C

""",


"IB_long_TO_bool":"""

POP2 %C %2
OR__ %2 %C %2
BL1_ %3 $00
SSUB %3 %2 %3
PU1_ %3

""",


"IB_int_TO_bool":"""

POP1 %C
BL1_ %2 $00
SSUB %2 %C %2
PU1_ %2

""",


"IB_address_label":"""

SYRD %A %B
SYCL @00000000
SYRE
PU2_ %B %A

""",


"IB_mem_byte_write_n":"""

POP2 %E %2
POP1 %C
MWBN %C %2
PU1_ %C

""",


"IB_mem_byte_write_v":"""

POP2 %E %2
POP1 %C
MWBV %C %2
PU1_ %C

""",


"IB_mem_word_write_n":"""

POP2 %2 %3
POP1 %C
MWWN %C %2 %3
PU1_ %C

""",


"IB_mem_word_write_v":"""

POP2 %2 %3
POP1 %C
MWWV %C %2 %3
PU1_ %C

""",


"IB_mem_dword_write_n":"""

POP2 %3 %4
POP2 %C %2
MWWN %C %3 %4
BL1_ %5 $02
ADDC %3 %3 %5
ADDN %4 %4 %F
MWWN %2 %3 %4
PU2_ %2 %C

""",


"IB_mem_dword_write_v":"""

POP2 %3 %4
POP2 %C %2
MWWV %C %3 %4
BL1_ %5 $02
ADDC %3 %3 %5
ADDN %4 %4 %F
MWWV %2 %3 %4
PU2_ %2 %C

""",


"IB_mem_byte_read_n":"""

POP2 %E %2
MRBN %C %2
PU1_ %C

""",


"IB_mem_byte_read_v":"""

POP2 %E %2
MRBV %C %2
PU1_ %C

""",


"IB_mem_sbyte_read_n":"""

POP2 %E %2
MRBN %C %2
BL1_ %2 $80
AND_ %3 %2 %C
RL1_ %2 #01FE
MULS %3 %2
OR__ %2 %C %3
PU1_ %2

""",


"IB_mem_sbyte_read_v":"""

POP2 %E %2
MRBV %C %2
BL1_ %2 $80
AND_ %3 %2 %C
RL1_ %2 #01FE
MULS %3 %2
OR__ %2 %C %3
PU1_ %2

""",


"IB_mem_word_read_n":"""

POP2 %2 %3
MRWN %C %2 %3
PU1_ %C

""",


"IB_mem_word_read_v":"""

POP2 %2 %3
MRWV %C %2 %3
PU1_ %C

""",


"IB_mem_dword_read_n":"""

POP2 %3 %4
MRWN %C %3 %4
BL1_ %5 $02
ADDC %3 %3 %5
ADDN %4 %4 %F
MRWN %2 %3 %4
PU2_ %2 %C

""",


"IB_mem_dword_read_v":"""

POP2 %3 %4
MRWV %C %3 %4
BL1_ %5 $02
ADDC %3 %3 %5
ADDN %4 %4 %F
MRWV %2 %3 %4
PU2_ %2 %C

""",


"IB_mem_word_copy_n_n":"""

POP2 %C %2
POP2 %3 %4
BL1_ %6 $02
MRWN %5 %C %2
MWWN %5 %3 %4
ADDC %C %C %6
ADDN %2 %2 %F
ADDC %3 %3 %6
ADDN %4 %4 %F
PU2_ %4 %3
PU2_ %2 %C

""",


"IB_mem_word_copy_n_v":"""

POP2 %C %2
POP2 %3 %4
BL1_ %6 $02
MRWN %5 %C %2
MWWV %5 %3 %4
ADDC %C %C %6
ADDN %2 %2 %F
ADDC %3 %3 %6
ADDN %4 %4 %F
PU2_ %4 %3
PU2_ %2 %C

""",



"IB_mem_word_copy_v_n":"""

POP2 %C %2
POP2 %3 %4
BL1_ %6 $02
MRWV %5 %C %2
MWWN %5 %3 %4
ADDC %C %C %6
ADDN %2 %2 %F
ADDC %3 %3 %6
ADDN %4 %4 %F
PU2_ %4 %3
PU2_ %2 %C

""",


"IB_mem_word_copy_v_v":"""

POP2 %C %2
POP2 %3 %4
BL1_ %6 $02
MRWV %5 %C %2
MWWV %5 %3 %4
ADDC %C %C %6
ADDN %2 %2 %F
ADDC %3 %3 %6
ADDN %4 %4 %F
PU2_ %4 %3
PU2_ %2 %C

""",




"IB_apply_to_self_word_rvalue_after":"""

POP2 %2 %3
PU2_ %3 %2
PU2_ %3 %2
INSR &1
POP1 %4
POP2 %2 %3
POP1 %6
PU2_ %3 %2
PU2_ %4 %6
INSR &0
POP1 %4
POP2 %2 %3
PU1_ %4
PU2_ %3 %2
INSR &2

""",


"IB_apply_to_self_dword_rvalue_after":"""

POP2 %2 %3
PU2_ %3 %2
PU2_ %3 %2
INSR &1
POP2 %4 %5
POP2 %2 %3
POP2 %6 %7
PU2_ %3 %2
PU2_ %5 %4
PU2_ %7 %6
INSR &0
POP2 %4 %5
POP2 %2 %3
PU2_ %5 %4
PU2_ %3 %2
INSR &2

""",


"IB_apply_to_self_word_rvalue_before":"""

POP2 %2 %3
PU2_ %3 %2
PU2_ %3 %2
INSR &1
POP1 %4
POP2 %2 %3
POP1 %6
PU1_ %4
PU2_ %3 %2
PU2_ %4 %6
INSR &0
POP1 %4
POP2 %2 %3
PU1_ %4
PU2_ %3 %2
INSR &2
POP1 %C

""",


"IB_apply_to_self_dword_rvalue_before":"""

POP2 %2 %3
PU2_ %3 %2
PU2_ %3 %2
INSR &1
POP2 %4 %5
POP2 %2 %3
POP2 %6 %7
PU2_ %5 %4
PU2_ %3 %2
PU2_ %5 %4
PU2_ %7 %6
INSR &0
POP2 %4 %5
POP2 %2 %3
PU2_ %5 %4
PU2_ %3 %2
INSR &2
POP2 %B %C

""",


"IB_apply_to_self_word_lvalue":"""

POP2 %2 %3
PU2_ %3 %2
PU2_ %3 %2
INSR &1
POP1 %4
POP2 %2 %3
POP1 %6
PU2_ %3 %2
PU2_ %4 %6
INSR &0
POP1 %4
POP2 %2 %3
PU2_ %3 %2
PU1_ %4
PU2_ %3 %2
INSR &2
POP1 %C

""",


"IB_apply_to_self_dword_lvalue":"""

POP2 %2 %3
PU2_ %3 %2
PU2_ %3 %2
INSR &1
POP2 %4 %5
POP2 %2 %3
POP2 %6 %7
PU2_ %3 %2
PU2_ %5 %4
PU2_ %7 %6
INSR &0
POP2 %4 %5
POP2 %2 %3
PU2_ %3 %2
PU2_ %5 %4
PU2_ %3 %2
INSR &2
POP2 %B %C

""",


"IB_load_byte":"""

BL1_ %C $00
PU1_ %C

""",


"IB_load_word":"""

RL1_ %C #0000
PU1_ %C

""",


"IB_load_dword":"""

RL2_ %2 %C !00000000
PU2_ %C %2

""",


"IB_statement_if":"""

INSR &0
POP1 %C
SYRD %2 %3
SYCL @00000000
SYRE
CJMP %2 %3 %C
INSR &1
LABL :00000000

""",


"IB_statement_if_else":"""

INSR &0
POP1 %C
SYRD %2 %3
SYCL @00000000
SYRE
CJMP %2 %3 %C
INSR &1
SYRD %2 %3
SYCL @00000001
SYRE
AJMP %2 %3
LABL :00000000
INSR &2
LABL :00000001

""",


"IB_statement_for":"""

INSR &0
SYRD %2 %3
SYCL @00000002
SYRE
AJMP %2 %3
LABL :00000000
INSR &2
LABL :00000002
INSR &1
POP1 %C
SYRD %2 %3
SYCL @00000001
SYRE
CJMP %2 %3 %C
INSR &3
SYRD %2 %3
SYCL @00000000
SYRE
AJMP %2 %3
LABL :00000001

""",


"IB_statement_while":"""

LABL :00000000
INSR &0
POP1 %C
SYRD %2 %3
SYCL @00000001
SYRE
CJMP %2 %3 %C
INSR &1
SYRD %2 %3
SYCL @00000000
SYRE
AJMP %2 %3
LABL :00000001

""",


"IB_statement_do_while":"""

LABL :00000000
INSR &1
LABL :00000002
INSR &0
POP1 %C
BL1_ %2 $01
XOR_ %C %C %2
SYRD %3 %4
SYCL @00000000
SYRE
CJMP %3 %4 %C
LABL :00000001

""",


"IB_direct_jump":"""

POP2 %C %2
AJMP %C %2

""",


"IB_raw_label":"""

LABL :00000000

""",

"IB_Lshift16":"""

POP2 %7 %8
BL1_ %B $01
BL1_ %6 $02
RL1_ %D #FFFF
BL1_ %C $00
BL1_ %2 $00

SUBC %A %C %7
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $01
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $02
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $03
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $04
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $05
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $06
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $07
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $08
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $09
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0A
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0B
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0C
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0D
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0E
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0F
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

PU1_ %2

""",

"IB_Rshift16":"""

POP2 %7 %8
BL1_ %B $01
RL1_ %D #FFFF
BL1_ %C $00
BL1_ %2 $00

SUBC %A %C %7
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $01
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $02
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $03
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $04
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $05
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $06
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $07
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $08
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $09
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0A
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0B
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0C
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0D
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0E
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0F
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

PU1_ %2

""",

"IB_Lshift32":"""

POP1 %7
POP2 %E %F
BL1_ %B $01
BL1_ %5 $02
RL1_ %D #FFFF
BL1_ %C $00
BL1_ %2 $00
BL1_ %3 $00

SUBC %A %C %7
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $01
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $02
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $03
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $04
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $05
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $06
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $07
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $08
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $09
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $0A
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $0B
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $0C
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $0D
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $0E
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $0F
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $10
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $11
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $12
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $13
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $14
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $15
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $16
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $17
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $18
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $19
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $1A
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $1B
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $1C
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $1D
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $1E
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

MULL %C %5
BL1_ %A $1F
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %E
OR__ %2 %2 %6
AND_ %6 %A %F
OR__ %3 %3 %6

PU2_ %3 %2

""",

"IB_Rshift32":"""

POP1 %7
POP2 %8 %9
BL1_ %B $01
RL1_ %D #FFFF
BL1_ %C $00
BL1_ %2 $00
BL1_ %3 $00
BL1_ %4 $01
RL1_ %5 #1000

SUBC %A %C %7
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $01
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $02
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $03
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $04
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $05
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $06
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $07
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $08
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $09
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $0A
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $0B
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $0C
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $0D
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $0E
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $0F
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $10
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $11
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $12
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $13
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $14
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $15
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $16
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $17
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $18
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $19
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $1A
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $1B
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $1C
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $1D
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $1E
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

AND_ %6 %4 %9
MULS %6 %5
SHFT %8 %8
SHFT %9 %9
OR__ %8 %8 %6
BL1_ %A $1F
XOR_ %A %A %7
SUBC %A %C %A
XOR_ %A %A %B
MULS %A %D
AND_ %6 %A %8
OR__ %2 %2 %6
AND_ %6 %A %9
OR__ %3 %3 %6

PU2_ %3 %2

""",

"IB_internal_div32_u_u":"""

FCST $0A #000A :00000001
PHIE %2
PHIE %3
PHIE %4
PHIE %5
BL1_ %6 $00
BL1_ %7 $00
BL1_ %7 $00
BL1_ %9 $00
RL1_ %C #FFFF
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
PHIS %8
PHIS %9
RET_
FCEN

""",

"IB_internal_mod32_u_u":"""

FCST $0A #000A :00000002
PHIE %2
PHIE %3
PHIE %4
PHIE %5
BL1_ %6 $00
BL1_ %7 $00
BL1_ %7 $00
BL1_ %9 $00
RL1_ %C #FFFF
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
PHIS %6
PHIS %7
RET_
FCEN

""",

"IB_internal_div32_s_s":"""

FCST $0A #000A :00000003
PHIE %2
PHIE %3
PHIE %4
PHIE %5
RL1_ %C #FFFF
BL1_ %D $80
BSWP %D %D
AND_ %6 %3 %D
ADDC %6 %6 %D
ADDN %6 %F %C
XOR_ %6 %6 %C
MOV_ %A %6
MOV_ %7 %F
XOR_ %2 %2 %6
XOR_ %3 %3 %6
ADDC %2 %2 %7
ADDN %3 %3 %F
AND_ %6 %5 %D
ADDC %6 %6 %D
ADDN %6 %F %C
XOR_ %6 %6 %C
XOR_ %A %A %6
MOV_ %7 %F
XOR_ %4 %4 %6
XOR_ %5 %5 %6
ADDC %4 %4 %7
ADDN %5 %5 %F
PU1_ %A
BL1_ %6 $00
BL1_ %7 $00
BL1_ %7 $00
BL1_ %9 $00
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %9 %9 %9
OR__ %9 %9 %D
POP1 %A
BL1_ %B $01
AND_ %B %B %A
XOR_ %7 %7 %A
XOR_ %9 %9 %A
ADDC %7 %7 %B
ADDN %9 %9 %F
PHIS %8
PHIS %9
RET_
FCEN

""",

"IB_internal_mod32_s_s":"""

FCST $0A #000A :00000004
PHIE %2
PHIE %3
PHIE %4
PHIE %5
RL1_ %C #FFFF
BL1_ %D $80
BSWP %D %D
AND_ %6 %3 %D
ADDC %6 %6 %D
ADDN %6 %F %C
XOR_ %6 %6 %C
MOV_ %A %6
MOV_ %7 %F
XOR_ %2 %2 %6
XOR_ %3 %3 %6
ADDC %2 %2 %7
ADDN %3 %3 %F
AND_ %6 %5 %D
ADDC %6 %6 %D
ADDN %6 %F %C
XOR_ %6 %6 %C
MOV_ %7 %F
XOR_ %4 %4 %6
XOR_ %5 %5 %6
ADDC %4 %4 %7
ADDN %5 %5 %F
PU1_ %A
BL1_ %6 $00
BL1_ %7 $00
BL1_ %7 $00
BL1_ %9 $00
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
ADDC %4 %4 %4
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDC %6 %6 %6
OR__ %6 %6 %D
MOV_ %D %F
ADDN %7 %7 %7
OR__ %7 %7 %D
BL1_ %D $01
MOV_ %A %6
MOV_ %B %7
SSUB %D %A %2
SSUB %D %B %3
ADDN %E %C %D
AND_ %6 %6 %E
AND_ %7 %7 %E
XOR_ %E %E %C
AND_ %A %A %E
AND_ %B %B %E
OR__ %6 %6 %A
OR__ %7 %7 %B
POP1 %A
BL1_ %B $01
AND_ %B %B %A
XOR_ %6 %6 %A
XOR_ %7 %7 %A
ADDC %6 %6 %B
ADDN %7 %7 %F
PHIS %6
PHIS %7
RET_
FCEN

""",

}



def splitInstructionString(s):
    l = s.split('\n')
    if len(l)>0 and len(l[-1])==0:
        l=l[:-1]
    return l

def checkArgument(singleArgWithPrefix):
    c=singleArgWithPrefix[0]
    if c=='%' or c=='&':
        assert len(singleArgWithPrefix)==2
    if c=='$':
        assert len(singleArgWithPrefix)==3
    if c=='#':
        assert len(singleArgWithPrefix)==5
    if c=='!' or c==':' or c=='@':
        assert len(singleArgWithPrefix)==9


for k in dictionaryOfInitializationContents.keys():
    dictionaryOfInitializationContents[k]=normalizeString(dictionaryOfInitializationContents[k])

namesToSkipBecauseZero = []

for k in dictionaryOfInitializationContents.keys():
    splitInstructions = splitInstructionString(dictionaryOfInitializationContents[k])
    
    if len(splitInstructions)==0:
        print "Warning: block '"+k+"' has zero length"
        namesToSkipBecauseZero.append(k)

namesToUse = filter(lambda e:e not in namesToSkipBecauseZero,dictionaryOfInitializationContents.keys())
namesToUse = sorted(namesToUse)
namesInRunSection=[
'IB_internal_div32_s_s',
'IB_internal_div32_u_u',
'IB_internal_mod32_s_s',
'IB_internal_mod32_u_u',
]
namesUnused=[
'IB_stack_swp_12',
]

f=open('GeneratedInstructionInitialization.c','w')
f.write('\n')


def genInitializerForSingleArgument(index,argUnion,arg,i):
    checkArgument(arg)
    return '['+str(index)+'].arg.'+argUnion+'.a_'+str(i)+'=0x'+arg[1:]


def genInitializerForArguments(index,instrList):
    l=[]
    s=''
    d={'%':'B',
       '$':'B',
       '#':'W',
       '!':'D',
       '@':'D',
       ':':'D'
       }
    s=''.join([d[i[0]] for i in instrList])
    r=','.join([genInitializerForSingleArgument(index,s,arg,i) for i,arg in enumerate(instrList)])
    if s=='BWD':print r
    return r

def genInitializerForSplitInstructions(tpl):
    index=tpl[0]
    instrList=tpl[1].split(' ')
    instrStrWithPrefix = ' '.join([instrList[0]]+map(lambda x:x[0],instrList[1:]))
    if not (instrStrWithPrefix in instructionListWithPrefix):
        print 'instruction "'+instrStrWithPrefix+'" does not exist with those prefixes'
        assert False
    if instrList[0]=='INSR':return ''
    return ','.join(filter(lambda x:len(x)!=0,['['+str(index)+'].id=I_'+instrList[0],genInitializerForArguments(index,instrList[1:])]))

def genFullInitization(k):
    altName1 = 'ib_'+k[3:]
    altName2 = altName1+'_contents'
    splitInstructions = splitInstructionString(dictionaryOfInitializationContents[k])
    numberOfInstructionsAsString = str(len(splitInstructions))
    s='static const InstructionSingle '+altName2+'['+numberOfInstructionsAsString+']={'+','.join(filter(lambda x:len(x)!=0,map(genInitializerForSplitInstructions,list(enumerate(splitInstructions)))))+'};\nstatic const InstructionBuffer '+altName1+'={(InstructionSingle*)'+altName2+','+numberOfInstructionsAsString+','+numberOfInstructionsAsString+'};\n\n'
    while s!=s.replace('=0x00','=0x0'):s=s.replace('=0x00','=0x0')
    for letter in map(str,range(10))+['A','B','C','D','E','F']:s=s.replace('=0x0'+letter,'=0x'+letter)
    return s


f.write('#ifdef IS_BUILDING_RUN\n')

for k in namesToUse:
    if k in namesInRunSection:
        if not (k in namesUnused):
            f.write(genFullInitization(k))

f.write('#else\n')

for k in namesToUse:
    if not (k in namesInRunSection):
        if not (k in namesUnused):
            f.write(genFullInitization(k))

f.write('#endif\n')
f.close()

f=open('GeneratedInstructionInsertion.c','w')


for k in namesToUse:
    altName1 = 'ib_'+k[3:]
    splitInstructions = splitInstructionString(dictionaryOfInitializationContents[k])
    insertNumbers=[]
    labelNumbers=[]
    for intr in splitInstructions:
        opCode = intr.split(' ')[0]
        if opCode=='INSR':
            numberOn = int(intr.split(' ')[1][1:],base=16)
            if not (numberOn in insertNumbers):
                insertNumbers.append(numberOn)
        elif opCode=='LABL' or opCode=='SYCL':
            numberOn = int(intr.split(' ')[1][1:],base=16)
            if not (numberOn in labelNumbers):
                labelNumbers.append(numberOn)
    if len(insertNumbers)!=0 or len(labelNumbers)!=0:
        f.write('void insert_'+k+'(InstructionBuffer* ib_ToAppendTo')
        for i in sorted(insertNumbers):
            f.write(',const InstructionBuffer* ib_ToInsertFor'+str(i))
        for i in sorted(labelNumbers):
            f.write(',uint32_t labelNumFor'+str(i))
        if k=='IB_CJMP_list_item':f.write(',uint16_t valueFor0')
        f.write('){\n')
        if k=='IB_CJMP_list_item':
            for i,intr in enumerate(splitInstructions):
                opCode = intr.split(' ')[0]
                f.write('\taddInstruction(ib_ToAppendTo,'+altName1+'.buffer['+str(i)+']);\n')
                if opCode=='RL1_':
                    f.write('\tib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.BW.a_1=valueFor0;\n')
                if opCode=='SYCL':
                    f.write('\tib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;\n')
        else:
            for i,intr in enumerate(splitInstructions):
                opCode = intr.split(' ')[0]
                if opCode=='INSR':
                    f.write('\tsingleMergeIB(ib_ToAppendTo,ib_ToInsertFor'+str(int(intr.split(' ')[1][1:],base=16))+');\n')
                else:
                    f.write('\taddInstruction(ib_ToAppendTo,'+altName1+'.buffer['+str(i)+']);\n')
                if opCode=='LABL' or opCode=='SYCL':
                    f.write('\tib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor'+str(int(intr.split(' ')[1][1:],base=16))+';\n')
        f.write('}\n')


if 2!=len(splitInstructionString(dictionaryOfInitializationContents['IB_load_byte'])):
    print "Warning: 'IB_load_byte' does not have two instructions in it. Make sure to change the LiteralLoadsAndCall.c file to respect this."
if 2!=len(splitInstructionString(dictionaryOfInitializationContents['IB_load_word'])):
    print "Warning: 'IB_load_word' does not have two instructions in it. Make sure to change the LiteralLoadsAndCall.c file to respect this."
if 2!=len(splitInstructionString(dictionaryOfInitializationContents['IB_load_dword'])):
    print "Warning: 'IB_load_dword' does not have two instructions in it. Make sure to change the LiteralLoadsAndCall.c file to respect this."
if 3!=len(splitInstructionString(dictionaryOfInitializationContents['IB_STPA'])):
    print "Warning: 'IB_STPA' does not have two instructions in it. Make sure to change the LiteralLoadsAndCall.c file to respect this."

f.write('\n')
print '\nDone'
f.close()





def getPrintoutCode():
    def getPref(strWithI):
        for i in instructionListWithPrefix:
            if strWithI[2:]==i.split(' ')[0]:
                return i
        raise ValueError
    l=map(lambda x:'I_'+x.split(' ')[0],instructionListWithPrefix)
    print 'switch(instructionSingle.id){'
    for j in l:
        p=getPref(j).split(' ')
        acc=''
        acc+='\tcase '+j+':printf("'+j[2:]
        for pref in p[1:]:
            acc+=' '
            if pref in '%&':
                acc+='%%%01X'
            if pref=='$':
                acc+='$%02X'
            if pref=='#':
                acc+='#%04X'
            if pref in '!@:':
                acc+=pref+'%08X'
        acc+='"'
        stuctName=''
        for pref in p[1:]:
            if pref=='&':
                stuctName+='B'
            if pref=='%':
                stuctName+='B'
            if pref=='$':
                stuctName+='B'
            if pref=='#':
                stuctName+='W'
            if pref=='!':
                stuctName+='D'
            if pref=='@':
                stuctName+='D'
            if pref==':':
                stuctName+='D'
        for i,pref in enumerate(p[1:]):
            acc+=','
            if pref=='&':
                acc+='instructionSingle.arg.'+stuctName+'.a_'+str(i)
            if pref=='%':
                acc+='instructionSingle.arg.'+stuctName+'.a_'+str(i)
            if pref=='$':
                acc+='instructionSingle.arg.'+stuctName+'.a_'+str(i)
            if pref=='#':
                acc+='instructionSingle.arg.'+stuctName+'.a_'+str(i)
            if pref in '!@:':
                acc+='instructionSingle.arg.'+stuctName+'.a_'+str(i)
        acc+=');break;'
        print acc
    print '}'

