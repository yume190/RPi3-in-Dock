#include <stdio.h>

// 給定一個 singly-linked list 結構:
typedef struct _List {
	struct _List *next;
	int val;
} List;

//	Q1: 實做得以交換存在 list 中兩個節點的函式:
void swap_list(List **head, List *a, List *b) {
//	if (*head == NULL || a == NULL || b == NUll)
//		return 
	return;
}

//	Q2: 透過 Q1 建立的程式 (swap)，對 singly linked-list 進行 bubble sort
void bubblesort(List **head) {
	return;
}

*List makeNode(int value) {
	List node = {
//		.next = NULL,
		.val = value,
	};
	return &node;
}

int main(int argc, char *argv[]) {
//	List node = {
////		.next = NULL,
//		.val = 3,
//	};

	List *c = &makeNode(3);
	List *b = makeNode(2);
	b->next = c;
	List *a = makeNode(1);
	a->next = b;
}

