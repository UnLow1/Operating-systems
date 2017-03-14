//
// Created by Admin on 12.03.2017.
//

#ifndef LAB_1_LIST_H
#define LAB_1_LIST_H

#include "contact.h"

typedef struct Node Node;

struct Node {
    Contact *contact;
    Node *next;
    Node *prev;
};

// ????? Node* makeAddressBook();

void deleteAddressBookList(Node **head);

void addContactToList(Node **head, Contact *contact);

void deleteContactFromList(Node **head, Contact *contact);

Node *searchContactInList(Node *head, Contact *contact);

int sizeOfAddressBookList(Node *head);

void sortList(Node **head, enum option option);

bool checkSortOptionList(enum option *option, Node *tmp);

void printAddressBookList(Node *head);

#endif //LAB_1_LIST_H
