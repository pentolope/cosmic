



// ensures that the node is a modifiable lvalue
void etc_00(struct AdvancedTypeInfo* ati){
	if ((!ati->post->isLValue|ati->post->isLValueConst)|(ati->isSU & ati->post->isLValueStructResultOfAssignment)){
		if (ati->post->isLValueConst){
			printInformativeMessageForExpression(true,"Must be a modifiable lvalue (this value is constant)",ati->node);
		} else {
			printInformativeMessageForExpression(true,"Must be a modifiable lvalue (this value is an rvalue)",ati->node);
		}
		exit(1);
	}
}

// ensures that the node is a lvalue (using AdvancedTypeInfo)
void etc_01(struct AdvancedTypeInfo* ati){
	if (!ati->post->isLValue){
		printInformativeMessageForExpression(true,"Must be a lvalue",ati->node);
		exit(1);
	}
}

// ensures that the node is a lvalue (using ExpressionTreeNode)
void etc_02(ExpressionTreeNode* node){
	if (!node->post.isLValue){
		printInformativeMessageForExpression(true,"Must be a lvalue",node);
		exit(1);
	}
}

// gets the size of tsDerefNQ, and throws error if it is zero
uint32_t etc_03(struct AdvancedTypeInfo* ati){
	uint32_t val=getSizeofForTypeString(ati->tsDerefNQ,true);
	if (val==0){
		printInformativeMessageForExpression(
			true,"The size of the type behind the pointer of this operand is required to be calculable",ati->node);
		exit(1);
	}
	return val;
}

// ensures that the node is a pointer
void etc_04(struct AdvancedTypeInfo* ati){
	if (!ati->isPtr){
		printInformativeMessageForExpression(
			true,"This must be a pointer",ati->node);
		exit(1);
	}
}

// ensures that the node is not pointer
void etc_05(struct AdvancedTypeInfo* ati){
	if (ati->isPtr){
		printInformativeMessageForExpression(
			true,"This must not be a pointer",ati->node);
		exit(1);
	}
}

// ensures that the node is a struct or union
void etc_06(struct AdvancedTypeInfo* ati){
	if (!ati->isSU){
		printInformativeMessageForExpression(
			true,"This must be a struct or union",ati->node);
		exit(1);
	}
}

// ensures that the node is not a struct or union
void etc_07(struct AdvancedTypeInfo* ati){
	if (ati->isSU){
		printInformativeMessageForExpression(
			true,"This must not be a struct or union",ati->node);
		exit(1);
	}
}

// ensures that the node is a pointer to struct or union
void etc_08(struct AdvancedTypeInfo* ati){
	if (!ati->isPtr || !isTypeStringOfStructOrUnion(ati->tsDerefNQ)){
		printInformativeMessageForExpression(
			true,"This must be a pointer to a struct or union",ati->node);
		exit(1);
	}
}

// struct and union member access
void etc_09(struct AdvancedTypeInfo* l_ati,ExpressionTreeNode* rn,ExpressionTreeNode* tn){
	char* tempNameForMember = copyStringSegmentToHeap(
		sourceContainer.string,
		rn->startIndexInString,
		rn->endIndexInString);
	struct TypeMemberEntry* typeMemberEntryPtr = getOffsetAndTypeStringOfMemberInType(
		l_ati->post->typeStringNQ,tempNameForMember);
	if (typeMemberEntryPtr==NULL){
		printInformativeMessageForExpression(true,"This member name does not exist in that struct or union",rn);
		exit(1);
	}
	cosmic_free(tempNameForMember);
	tn->post.typeString=applyToTypeStringRemoveIdentifierToNew(typeMemberEntryPtr->typeString);
	tn->post.extraVal=typeMemberEntryPtr->offset;
	if (l_ati->post->isLValue){
		tn->post.operatorTypeID=1;
		tn->post.isLValue=true;
		tn->post.isLValueConst    = l_ati->post->isLValueConst;
		tn->post.isLValueVolatile = l_ati->post->isLValueVolatile;
		tn->post.isLValueStructResultOfAssignment = l_ati->post->isLValueStructResultOfAssignment;
		genTypeStringNQ(tn);
		if (l_ati->post->isLValueStructResultOfAssignment && !isTypeStringOfStructOrUnion(tn->post.typeStringNQ)){
			// I am not 100% confident in this transformation.
			// We do need to give the compiler the message that this isn't assignable, and it basically should be an rvalue
			tn->post.isLValueStructResultOfAssignment=false;
			applyConvertToRvalue(tn);
		}
	} else {
		tn->post.operatorTypeID=2;
		printf("Unimplemented error: cannot handle structs or union in that way yet\n");
		exit(1);
	}
}









