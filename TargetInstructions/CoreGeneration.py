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
'STPS % #',
'STWN % $',
'STRN % $',
'STWV % $',
'STRV % $',
'ALOC',
'ALCR % #',
'STOF % #',
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
'LABL :',# the label is symbolic and is used to discern between multiple labels, unless it is a intrinsic_back entry
'PHIS %',
'PHIE %',
'FCST $ # :',
'FCEN',
'D32U',
'R32U',
'D32S',
'R32S',
'D64U',
'R64U',
'D64S',
'R64S',
'LAD0 % % % % % %',
'LAD1 % % % % %',
'LAD2 % % % %',
'LAD3 % % % % % % % %',
'LAD4',
'LAD5',
'LSU0 % % % %',
'LSU3 % % % % % % % %',
'LSU4',
'LSU5',
'LMU3 % % % % % % % %',
'LMU4',
'LMU5',
'LDI4',
'LDI5',
'LLS6',
'LLS7',
'LRS6',
'LRS7',
'SYRB %',
'SYRW %',
'SYRD % %',
'SYRQ % % % %',
'SYRE',
'NSNB !',
'ZNXB !',
'BYTE $',
'WORD #',
'DWRD !',
'SYDB',
'SYDW',
'SYDD',
'SYDQ',
'SYDE',
'SYCB $',
'SYCW #',
'SYCD !',
'SYCL @',# the label is symbolic and is used to discern between multiple labels, unless it is a intrinsic_back entry
'SYW0',
'SYW1',
'SYW2',
'SYW3',
'SYW4',
'SYW5',
'SYW6',
'SYW7',
'SYW8',
'SYW9',
'SBLW',
'SBRW',
'SYD0',
'SYD1',
'SYD2',
'SYD3',
'SYD4',
'SYD5',
'SYD6',
'SYD7',
'SYD8',
'SYD9',
'SBLD',
'SBRD',
'SYQ0',
'SYQ1',
'SYQ2',
'SYQ3',
'SYQ4',
'SYQ5',
'SYQ6',
'SYQ7',
'SYQ8',
'SYQ9',
'SBLQ',
'SBRQ',
'SCBW',
'SCWD',
'SCDQ',
'SCQD',
'SCDW',
'SCWB',
'SCDB',
'SCQB',
'SCZD',
'SCZQ',

'ERR_',
'DEPL',
'PEPH',
'STPI % #',
'LOFF !',
'INSR &' # the & prefix contains 1 hex character
]


dictionaryOfInitializationContents = {
"IB_stack_dupe_word":"""

POP1 %2
PU2_ %2 %2

""",


"IB_stack_dupe_dword":"""

POP2 %3 %2
PU2_ %2 %3
PU2_ %2 %3

""",


"IB_stack_swp_11":"""

POP2 %2 %3
PU2_ %2 %3

""",


"IB_stack_swp_12":"""

POP2 %3 %4
POP1 %2
PU2_ %4 %3
PU1_ %2

""",


"IB_stack_swp_21":"""

POP1 %4
POP2 %2 %3
PU1_ %4
PU2_ %3 %2

""",


"IB_stack_swp_22":"""

POP2 %2 %3
POP2 %4 %5
PU2_ %3 %2
PU2_ %5 %4

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

POP2 %3 %2
ADDN %4 %2 %3
PU1_ %4

""",


"IB_i_32add":"""

POP2 %4 %5
POP2 %6 %7
LAD0 %2 %3 %4 %5 %6 %7
PU2_ %3 %2

""",


"IB_i_16sub":"""

POP2 %2 %3
SUBN %4 %3 %2
PU1_ %4

""",


"IB_i_32sub":"""

POP2 %4 %5
POP2 %2 %3
LSU0 %2 %3 %4 %5
PU2_ %3 %2

""",


"IB_i_16mul":"""

POP2 %2 %3
MULS %3 %2
PU1_ %3

""",


"IB_i_32mul":"""

POP2 %2 %3
POP2 %D %E
MULL %2 %3
PU2_ %E %D

""",


"IB_i_16div_u_u":"""

POP2 %2 %3
DIVM %3 %2
PU1_ %3

""",


"IB_i_16mod_u_u":"""

POP2 %2 %3
DIVM %3 %2
PU1_ %2

""",


"IB_i_16div_s_s":"""

POP2 %2 %C
RL1_ %9 #8000
RL1_ %3 #FFFF
MOV_ %4 %3
AND_ %7 %C %9
LAD2 %7 %A %7 %9
MULS %3 %A
MOV_ %5 %A
XOR_ %C %C %3
ADDN %C %C %5
AND_ %7 %2 %9
LAD2 %7 %A %7 %9
MULS %4 %A
MOV_ %6 %A
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
LAD2 %7 %A %7 %9
MULS %3 %A
MOV_ %5 %A
XOR_ %C %C %3
ADDN %C %C %5
AND_ %7 %2 %9
LAD2 %7 %A %7 %9
MULS %4 %A
MOV_ %6 %A
XOR_ %2 %2 %4
ADDN %2 %2 %6
DIVM %C %2
XOR_ %2 %2 %3
ADDN %2 %2 %5
PU1_ %2

""",




"IB_intrinsic_front_i_32div_u_u":"""

POP2 %4 %5
POP2 %2 %3
D32U
PU2_ %9 %8

""",


"IB_intrinsic_front_i_32mod_u_u":"""

POP2 %4 %5
POP2 %2 %3
R32U
PU2_ %7 %6

""",


"IB_intrinsic_front_i_32div_s_s":"""

POP2 %4 %5
POP2 %2 %3
D32S
PU2_ %9 %8

""",


"IB_intrinsic_front_i_32mod_s_s":"""

POP2 %4 %5
POP2 %2 %3
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


"IB_comp_g_int_s":"""

POP2 %C %2

RL1_ %3 #8000
XOR_ %C %C %3
XOR_ %2 %2 %3

BL1_ %3 $01
SSUB %3 %C %2
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3


""",


"IB_comp_g_long_s":"""

POP2 %4 %3
POP2 %2 %C

RL1_ %5 #8000
XOR_ %3 %3 %5
XOR_ %C %C %5

BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_geq_int_s":"""

POP2 %C %2

RL1_ %3 #8000
XOR_ %C %C %3
XOR_ %2 %2 %3

BL1_ %3 $01
SSUB %3 %2 %C
PU1_ %3

""",


"IB_comp_geq_long_s":"""

POP2 %4 %3
POP2 %2 %C

RL1_ %5 #8000
XOR_ %3 %3 %5
XOR_ %C %C %5

BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %3
PU1_ %5

""",


"IB_comp_l_int_s":"""

POP2 %C %2

RL1_ %3 #8000
XOR_ %C %C %3
XOR_ %2 %2 %3

BL1_ %3 $01
SSUB %3 %2 %C
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3

""",


"IB_comp_l_long_s":"""

POP2 %4 %3
POP2 %2 %C

RL1_ %5 #8000
XOR_ %3 %3 %5
XOR_ %C %C %5

BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %3
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_leq_int_s":"""

POP2 %3 %2

RL1_ %4 #8000
XOR_ %3 %3 %4
XOR_ %2 %2 %4

BL1_ %4 $01
SSUB %4 %3 %2
PU1_ %4

""",


"IB_comp_leq_long_s":"""

POP2 %4 %3
POP2 %2 %C

RL1_ %5 #8000
XOR_ %3 %3 %5
XOR_ %C %C %5

BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
PU1_ %5

""",


"IB_comp_g_int_u":"""

POP2 %C %2
BL1_ %3 $01
SSUB %3 %C %2
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3

""",


"IB_comp_g_long_u":"""

POP2 %4 %3
POP2 %2 %C
BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_geq_int_u":"""

POP2 %C %2
BL1_ %3 $01
SSUB %3 %2 %C
PU1_ %3

""",


"IB_comp_geq_long_u":"""

POP2 %4 %3
POP2 %2 %C
BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %3
PU1_ %5

""",


"IB_comp_l_int_u":"""

POP2 %C %2
BL1_ %3 $01
SSUB %3 %2 %C
BL1_ %4 $01
XOR_ %3 %4 %3
PU1_ %3

""",


"IB_comp_l_long_u":"""

POP2 %4 %3
POP2 %2 %C
BL1_ %5 $01
SSUB %5 %2 %4
SSUB %5 %C %3
BL1_ %6 $01
XOR_ %5 %6 %5
PU1_ %5

""",


"IB_comp_leq_int_u":"""

POP2 %C %2
BL1_ %3 $01
SSUB %3 %C %2
PU1_ %3

""",


"IB_comp_leq_long_u":"""

POP2 %4 %3
POP2 %2 %C
BL1_ %5 $01
SSUB %5 %4 %2
SSUB %5 %3 %C
PU1_ %5

""",


"IB_b_and_int":"""

POP2 %2 %3
AND_ %4 %2 %3
PU1_ %4

""",


"IB_b_and_long":"""

POP2 %C %2
POP2 %3 %4
AND_ %5 %C %3
AND_ %6 %2 %4
PU2_ %6 %5

""",


"IB_b_or_int":"""

POP2 %2 %3
OR__ %4 %2 %3
PU1_ %4

""",


"IB_b_or_long":"""

POP2 %C %2
POP2 %3 %4
OR__ %5 %C %3
OR__ %6 %2 %4
PU2_ %6 %5

""",


"IB_b_xor_int":"""

POP2 %2 %3
XOR_ %4 %2 %3
PU1_ %4

""",


"IB_b_xor_long":"""

POP2 %C %2
POP2 %3 %4
XOR_ %5 %C %3
XOR_ %6 %2 %4
PU2_ %6 %5

""",


"IB_b_not_int":"""

POP1 %4
RL1_ %3 #FFFF
XOR_ %2 %3 %4
PU1_ %2

""",


"IB_b_not_long":"""

POP2 %6 %2
RL1_ %5 #FFFF
XOR_ %3 %6 %5
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
POP1 %B
BL1_ %2 $01
XOR_ %C %2 %B
SYRD %3 %4
SYCL @00000000
SYRE
PHIS %B
CJMP %3 %4 %C
INSR &1
POP1 %B
PHIS %B
LABL :00000000
PHIE %B
PU1_ %B

""",


"IB_char_s_TO_int_s":"""

POP1 %4
BL1_ %2 $80
AND_ %3 %2 %4
RL1_ %2 #01FE
MULS %3 %2
OR__ %2 %4 %3
PU1_ %2

""",


"IB_int_s_TO_long_s":"""

POP1 %4
LAD2 %2 %3 %4 %4
RL1_ %2 #FFFF
MULS %2 %3
PU2_ %2 %4

""",


"IB_int_u_TO_long":"""

POP1 %2
BL1_ %3 $00
PU2_ %3 %2

""",


"IB_long_TO_int":"""

POP2 %2 %3
PU1_ %2

""",


"IB_long_TO_bool":"""

POP2 %2 %3
OR__ %3 %2 %3
BL1_ %4 $00
SSUB %4 %3 %4
PU1_ %4

""",


"IB_int_TO_bool":"""

POP1 %2
BL1_ %4 $00
SSUB %4 %2 %4
PU1_ %4

""",


"IB_address_label":"""

SYRD %A %B
SYCL @00000000
SYRE
PU2_ %B %A

""",


"IB_mem_byte_write_n":"""

POP2 %D %3
POP1 %4
BL1_ %2 $FF
AND_ %4 %4 %2
MWBN %4 %3
PU1_ %4

""",


"IB_mem_byte_write_v":"""

POP2 %D %3
POP1 %4
BL1_ %2 $FF
AND_ %4 %4 %2
MWBV %4 %3
PU1_ %4

""",


"IB_mem_word_write_n":"""

POP2 %2 %3
POP1 %4
MWWN %4 %2 %3
PU1_ %4

""",


"IB_mem_word_write_v":"""

POP2 %2 %3
POP1 %4
MWWV %4 %2 %3
PU1_ %4

""",


"IB_mem_dword_write_n":"""

POP2 %3 %4
POP2 %6 %5
MWWN %6 %3 %4
BL1_ %7 $02
LAD1 %3 %4 %3 %4 %7
MWWN %5 %3 %4
PU2_ %5 %6

""",


"IB_mem_dword_write_v":"""

POP2 %3 %4
POP2 %6 %5
MWWV %6 %3 %4
BL1_ %7 $02
LAD1 %3 %4 %3 %4 %7
MWWV %2 %3 %4
PU2_ %5 %6

""",


"IB_mem_byte_read_n":"""

POP2 %D %2
MRBN %3 %2
PU1_ %3

""",


"IB_mem_byte_read_v":"""

POP2 %D %2
MRBV %3 %2
PU1_ %3

""",


"IB_mem_sbyte_read_n":"""

POP2 %D %2
MRBN %4 %2
BL1_ %2 $80
AND_ %3 %2 %4
RL1_ %2 #01FE
MULS %3 %2
OR__ %2 %4 %3
PU1_ %2

""",


"IB_mem_sbyte_read_v":"""

POP2 %D %2
MRBV %4 %2
BL1_ %2 $80
AND_ %3 %2 %4
RL1_ %2 #01FE
MULS %3 %2
OR__ %2 %4 %3
PU1_ %2

""",


"IB_mem_word_read_n":"""

POP2 %2 %3
MRWN %4 %2 %3
PU1_ %4

""",


"IB_mem_word_read_v":"""

POP2 %2 %3
MRWV %4 %2 %3
PU1_ %4

""",


"IB_mem_dword_read_n":"""

POP2 %3 %4
MRWN %6 %3 %4
BL1_ %7 $02
LAD1 %3 %4 %3 %4 %7
MRWN %5 %3 %4
PU2_ %5 %6

""",


"IB_mem_dword_read_v":"""

POP2 %3 %4
MRWV %6 %3 %4
BL1_ %7 $02
LAD1 %3 %4 %3 %4 %7
MRWV %5 %3 %4
PU2_ %5 %6

""",


"IB_mem_word_copy_n_n":"""

POP2 %2 %3
POP2 %4 %5
BL1_ %7 $02
MRWN %6 %2 %3
MWWN %6 %4 %5
LAD1 %2 %3 %2 %3 %7
LAD1 %4 %5 %4 %5 %7
PU2_ %5 %4
PU2_ %3 %2

""",


"IB_mem_word_copy_n_v":"""

POP2 %2 %3
POP2 %4 %5
BL1_ %7 $02
MRWN %6 %2 %3
MWWV %6 %4 %5
LAD1 %2 %3 %2 %3 %7
LAD1 %4 %5 %4 %5 %7
PU2_ %5 %4
PU2_ %3 %2

""",



"IB_mem_word_copy_v_n":"""

POP2 %2 %3
POP2 %4 %5
BL1_ %7 $02
MRWV %6 %2 %3
MWWN %6 %4 %5
LAD1 %2 %3 %2 %3 %7
LAD1 %4 %5 %4 %5 %7
PU2_ %5 %4
PU2_ %3 %2

""",


"IB_mem_word_copy_v_v":"""

POP2 %2 %3
POP2 %4 %5
BL1_ %7 $02
MRWV %6 %2 %3
MWWV %6 %4 %5
LAD1 %2 %3 %2 %3 %7
LAD1 %4 %5 %4 %5 %7
PU2_ %5 %4
PU2_ %3 %2

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

BL1_ %2 $00
PU1_ %2

""",


"IB_load_word":"""

RL1_ %2 #0000
PU1_ %2

""",


"IB_load_dword":"""

RL2_ %2 %3 !00000000
PU2_ %3 %2

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

POP2 %2 %3
AJMP %2 %3

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
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $01
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $02
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $03
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $04
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $05
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $06
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $07
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $08
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $09
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0A
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0B
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0C
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0D
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0E
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

MULS %8 %6
BL1_ %A $0F
XOR_ %A %A %7
SUBC %A %C %A
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
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $01
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $02
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $03
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $04
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $05
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $06
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $07
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $08
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $09
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0A
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0B
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0C
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0D
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0E
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

SHFT %8 %8
BL1_ %A $0F
XOR_ %A %A %7
SUBC %A %C %A
MULS %A %D
AND_ %A %A %8
OR__ %2 %2 %A

PU1_ %2

""",

"IB_intrinsic_front_Lshift32":"""

POP1 %A
POP2 %6 %7
LLS6
PU2_ %3 %2

""",

"IB_intrinsic_front_Rshift32":"""

POP1 %A
POP2 %6 %7
LRS6
PU2_ %3 %2

""",






"IB_intrinsic_back_Lshift32":"""

FCST $0A #000A :00000011
MOV_ %2 %6
MOV_ %3 %7
SYRD %6 %7
SYCL @00000021
SYDE
CJMP %6 %7 %A
BL1_ %E $00
BL1_ %B $01
RL1_ %C #8000
SYRD %6 %7
SYCL @00000020
SYDE
LABL :00000020
AND_ %D %2 %C
SUBC %D %D %C
ADDN %2 %2 %2
ADDN %3 %3 %3
OR__ %2 %2 %D
ADDN %E %E %B
SUBC %D %E %A
CJMP %6 %7 %D
LABL :00000021
RET_
FCEN

""",

"IB_intrinsic_back_Rshift32":"""

FCST $0A #000A :00000013
MOV_ %2 %6
MOV_ %3 %7
SYRD %6 %7
SYCL @00000023
SYDE
CJMP %6 %7 %A
BL1_ %E $00
BL1_ %B $01
RL1_ %C #8000
SYRD %6 %7
SYCL @00000022
SYDE
LABL :00000022
AND_ %D %3 %B
MULS %D %C
SHFT %2 %2
SHFT %3 %3
OR__ %2 %2 %D
ADDN %E %E %B
SUBC %D %E %A
CJMP %6 %7 %D
LABL :00000023
RET_
FCEN

""",


"IB_intrinsic_back_div32_u_u":"""

FCST $0A #000A :00000001

BL1_ %6 $00
BL1_ %7 $00
BL1_ %B $00
LABL :00000024
LAD3 %2 %3 %6 %7 %2 %3 %6 %7
PU2_ %2 %3
MOV_ %2 %6
MOV_ %3 %7
BL1_ %A $01
SSUB %A %2 %4
SSUB %A %3 %5
LAD0 %8 %9 %8 %9 %8 %9
OR__ %8 %8 %A
SYRD %C %D
SYCL @00000025
SYRE
CJMP %C %D %A
MOV_ %6 %2
MOV_ %7 %3
LABL :00000025
POP2 %3 %2
BL1_ %A $01
ADDN %B %B %A
BL1_ %A $20
SUBC %A %B %A
SYRD %C %D
SYCL @00000024
SYRE
CJMP %C %D %A

RET_
FCEN

""",

"IB_intrinsic_back_mod32_u_u":"""

FCST $0A #000A :00000002
BL1_ %6 $00
BL1_ %7 $00
BL1_ %B $00
SYRD %8 %9
SYCL @00000027
SYRE
SYRD %C %D
SYCL @00000026
SYRE
LABL :00000026
LAD3 %2 %3 %6 %7 %2 %3 %6 %7
PU2_ %2 %3
MOV_ %2 %6
MOV_ %3 %7
BL1_ %A $01
SSUB %A %2 %4
SSUB %A %3 %5
CJMP %8 %9 %A
MOV_ %6 %2
MOV_ %7 %3
LABL :00000027
POP2 %3 %2
BL1_ %A $01
ADDN %B %B %A
BL1_ %A $20
SUBC %A %B %A
CJMP %C %D %A
RET_
FCEN

""",

"IB_intrinsic_back_div32_s_s":"""

FCST $0A #000A :00000003
LAD2 %D %6 %3 %3
LAD2 %D %7 %5 %5
RL1_ %D #FFFF
MOV_ %C %D
MULS %C %6
MULS %D %7
XOR_ %2 %2 %C
XOR_ %3 %3 %C
XOR_ %4 %4 %D
XOR_ %5 %5 %D
LAD1 %2 %3 %2 %3 %6
LAD1 %4 %5 %4 %5 %7
XOR_ %6 %6 %7
XOR_ %C %C %D
PU2_ %C %6
D32U
POP2 %D %C
XOR_ %8 %8 %C
XOR_ %9 %9 %C
LAD1 %8 %9 %8 %9 %D
RET_
FCEN

""",

"IB_intrinsic_back_mod32_s_s":"""

FCST $0A #000A :00000004
LAD2 %D %6 %3 %3
LAD2 %D %7 %5 %5
RL1_ %D #FFFF
MOV_ %C %D
MULS %C %6
MULS %D %7
XOR_ %2 %2 %C
XOR_ %3 %3 %C
XOR_ %4 %4 %D
XOR_ %5 %5 %D
LAD1 %2 %3 %2 %3 %6
LAD1 %4 %5 %4 %5 %7
PU2_ %C %6
R32U
POP2 %D %C
XOR_ %6 %6 %C
XOR_ %7 %7 %C
LAD1 %6 %7 %6 %7 %D
RET_
FCEN

""",

}



def splitInstructionString(s):
    l = s.split('\n')
    if len(l)>0 and len(l[-1])==0:l=l[:-1]
    return l

def checkArgument(singleArgWithPrefix):
    c=singleArgWithPrefix[0]
    if c=='%' or c=='&':assert len(singleArgWithPrefix)==2
    elif c=='$':assert len(singleArgWithPrefix)==3
    elif c=='#':assert len(singleArgWithPrefix)==5
    elif c=='!' or c==':' or c=='@':assert len(singleArgWithPrefix)==9
    else:assert False


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
namesUnused=[
'IB_stack_swp_12',
]


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
    if s=='B'*len(s):s='B'+str(len(s))
    r=','.join([genInitializerForSingleArgument(index,s,arg,i) for i,arg in enumerate(instrList)])
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



f=open('GeneratedInstructionInitialization.c','w')
f.write('\n')

for k in namesToUse:
    if not (k in namesUnused):
        f.write(genFullInitization(k))

f.close()

f=open('GeneratedInstructionInsertion.c','w')


for k in namesToUse:
    if not (k in namesUnused) and k.split('_')[:3]!=['IB','intrinsic','back']:
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
            f.write('static void insert_'+k+'(InstructionBuffer* ib_ToAppendTo')
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
    print "Warning: 'IB_STPA' does not have three instructions in it. Make sure to change the LiteralLoadsAndCall.c file to respect this."

f.write('\n')
f.close()

def genPrintoutCode():
    finalString='void printSingleInstructionOptCode(const InstructionSingle instructionSingle){\n'
    def getPref(strWithI):
        for i in instructionListWithPrefix:
            if strWithI[2:]==i.split(' ')[0]:
                return i
        raise ValueError
    l=map(lambda x:'I_'+x.split(' ')[0],instructionListWithPrefix)
    finalString+='switch(instructionSingle.id){\n'
    for j in l:
        p=getPref(j).split(' ')
        acc=''
        if j!='I_ERR_':
            acc+='case '+j+':printf("'+j[2:]
            for pref in p[1:]:
                acc+=' '
                if pref in '%&':
                    acc+='%%%01X'
                if pref=='$':
                    acc+='$%02X'
                if pref=='#':
                    acc+='#%04X'
                if pref in '!@:':
                    acc+=pref+'%04X%04X'
            acc+='"'
            structName=''
            for pref in p[1:]:
                if pref=='&':
                    structName+='B'
                if pref=='%':
                    structName+='B'
                if pref=='$':
                    structName+='B'
                if pref=='#':
                    structName+='W'
                if pref=='!':
                    structName+='D'
                if pref=='@':
                    structName+='D'
                if pref==':':
                    structName+='D'
            if structName=='B'*len(structName):structName='B'+str(len(structName))
            for i,pref in enumerate(p[1:]):
                acc+=','
                accessor='instructionSingle.arg.'+structName+'.a_'+str(i)
                if pref in '&%$#':
                    acc+='(uint16_t)'+accessor
                if pref in '!@:':
                    acc+='(uint16_t)('+accessor+'>>16),(uint16_t)'+accessor
            acc+=');return;\n'
        else:
            acc+='case '+j+':return;'
        finalString+=acc
    finalString+='}\n}\n'
    f=open('PrintSingleInstruction.c','w')
    f.write(finalString)
    f.close()

genPrintoutCode()

print('\nCoreGeneration.py Finished')







