static void insert_IB_CJMP_list_item(InstructionBuffer* ib_ToAppendTo,uint32_t labelNumFor0,uint16_t valueFor0){
	addInstruction(ib_ToAppendTo,ib_CJMP_list_item.buffer[0]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.BW.a_1=valueFor0;
	addInstruction(ib_ToAppendTo,ib_CJMP_list_item.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_CJMP_list_item.buffer[2]);
	addInstruction(ib_ToAppendTo,ib_CJMP_list_item.buffer[3]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_CJMP_list_item.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_CJMP_list_item.buffer[5]);
}
static void insert_IB_address_label(InstructionBuffer* ib_ToAppendTo,uint32_t labelNumFor0){
	addInstruction(ib_ToAppendTo,ib_address_label.buffer[0]);
	addInstruction(ib_ToAppendTo,ib_address_label.buffer[1]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_address_label.buffer[2]);
	addInstruction(ib_ToAppendTo,ib_address_label.buffer[3]);
}
static void insert_IB_apply_to_self_dword_lvalue(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2){
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[0]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[2]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[8]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[9]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[11]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[12]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[13]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[14]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[15]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_lvalue.buffer[17]);
}
static void insert_IB_apply_to_self_dword_rvalue_after(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2){
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[0]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[2]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[8]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[9]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[11]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[12]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[13]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_after.buffer[14]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
}
static void insert_IB_apply_to_self_dword_rvalue_before(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2){
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[0]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[2]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[8]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[9]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[10]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[12]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[13]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[14]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[15]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_dword_rvalue_before.buffer[17]);
}
static void insert_IB_apply_to_self_word_lvalue(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2){
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[0]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[2]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[8]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[10]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[11]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[12]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[13]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[14]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_lvalue.buffer[16]);
}
static void insert_IB_apply_to_self_word_rvalue_after(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2){
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[0]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[2]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[8]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[10]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[11]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[12]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_after.buffer[13]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
}
static void insert_IB_apply_to_self_word_rvalue_before(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2){
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[0]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[2]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[8]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[9]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[11]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[12]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[13]);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[14]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
	addInstruction(ib_ToAppendTo,ib_apply_to_self_word_rvalue_before.buffer[16]);
}
static void insert_IB_logic_and_with_jmp(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,uint32_t labelNumFor0){
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[2]);
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[3]);
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[4]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[7]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_logic_and_with_jmp.buffer[9]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
}
static void insert_IB_logic_or_with_jmp(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,uint32_t labelNumFor0){
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[2]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[3]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[5]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[8]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[10]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[11]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[12]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[13]);
	addInstruction(ib_ToAppendTo,ib_logic_or_with_jmp.buffer[14]);
}
static void insert_IB_raw_label(InstructionBuffer* ib_ToAppendTo,uint32_t labelNumFor0){
	addInstruction(ib_ToAppendTo,ib_raw_label.buffer[0]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
}
static void insert_IB_statement_do_while(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,uint32_t labelNumFor0,uint32_t labelNumFor1,uint32_t labelNumFor2){
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[0]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[2]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor2;
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[6]);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[8]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[9]);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[10]);
	addInstruction(ib_ToAppendTo,ib_statement_do_while.buffer[11]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor1;
}
static void insert_IB_statement_for(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2,const InstructionBuffer* ib_ToInsertFor3,uint32_t labelNumFor0,uint32_t labelNumFor1,uint32_t labelNumFor2){
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[2]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor2;
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[3]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[5]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[7]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor2;
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[9]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[10]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[11]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor1;
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[12]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[13]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor3);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[15]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[16]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[17]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[18]);
	addInstruction(ib_ToAppendTo,ib_statement_for.buffer[19]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor1;
}
static void insert_IB_statement_if(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,uint32_t labelNumFor0){
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_statement_if.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_statement_if.buffer[2]);
	addInstruction(ib_ToAppendTo,ib_statement_if.buffer[3]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_statement_if.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_statement_if.buffer[5]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_statement_if.buffer[7]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
}
static void insert_IB_statement_if_else(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,const InstructionBuffer* ib_ToInsertFor2,uint32_t labelNumFor0,uint32_t labelNumFor1){
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[1]);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[2]);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[3]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[4]);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[5]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[7]);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[8]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor1;
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[9]);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[10]);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[11]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor2);
	addInstruction(ib_ToAppendTo,ib_statement_if_else.buffer[13]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor1;
}
static void insert_IB_statement_while(InstructionBuffer* ib_ToAppendTo,const InstructionBuffer* ib_ToInsertFor0,const InstructionBuffer* ib_ToInsertFor1,uint32_t labelNumFor0,uint32_t labelNumFor1){
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[0]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor0);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[2]);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[3]);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[4]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor1;
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[5]);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[6]);
	singleMergeIB(ib_ToAppendTo,ib_ToInsertFor1);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[8]);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[9]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor0;
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[10]);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[11]);
	addInstruction(ib_ToAppendTo,ib_statement_while.buffer[12]);
	ib_ToAppendTo->buffer[ib_ToAppendTo->numberOfSlotsTaken-1].arg.D.a_0=labelNumFor1;
}

