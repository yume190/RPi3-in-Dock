//#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LAST_NAME_SIZE 16

typedef struct __PHONE_BOOK_ENTRY {
	char LastName[MAX_LAST_NAME_SIZE];
	char FirstName[16];
	char email[16];
	char phone[10];
	char cell[10];
	char addr1[16];
	char addr2[16];
	char city[16];
	char state[2];
	char zip[5];
	struct __PHONE_BOOK_ENTRY *pNext;
} PhoneBook;

typedef struct __PHONE_BOOK_NODE {
	PhoneBook *c;
	PhoneBookNode *l;
	PhoneBookNode *r;
} PhoneBookNode;

//typedef struct __LAST_NAME_ENTRY{
//	char lastName[MAX_LAST_NAME_SIZE];
////	entry *detail;
//	struct __LAST_NAME_ENTRY *pNext;
//} lastNameEntry;

void insertNode(PhoneBookNode *t, PhoneBook *pb);
PhoneBook *FindName(char Last[], PhoneBook *pHead);

PhoneBook *FindName(char Last[], PhoneBook *pHead) {
//	while (pHead != NULL) {
//		if (stricmp(Last, pHead->LastName) == 0)
//			return pHead;
//		pHead = pHead->pNext;
//	}
//	return NULL;

	if (!pHead) 
		return NULL;

	PhoneBook *walk = pHead;
	PhoneBookNode tree = {
		.c = pHead,
	};
	walk = walk->pNext;
	
	while (walk) {
		insertNode(&tree,walk);
		walk = walk->pNext;
	}
}

void insertNode(PhoneBookNode *t, PhoneBook *pb) {
	
//	if (stricmp(Last, pHead->LastName) == 0)
	if (stricmp(t->c->LastName, pb->LastName) == 0) {
		if (t->r == NULL) {
			//insertR
			return;
		}
		insertNode(t->r, pb);
	} else {
		if (t->r == NULL) {
			//insertL
			return;
		}
		insertNode(t->l, pb);		
	}
	return;
}

int main(int argc, char *argv[]) {
	return 0;
}