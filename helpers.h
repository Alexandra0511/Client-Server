#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>


typedef struct topic {
	char* name;
	int sf;
	struct topic* next;
}topic;

typedef struct message_udp {
	char name_topic[50];
	uint8_t type; //tip date pe un octet
	char content[1500];
}message_udp;

typedef struct saved_message {
	struct message_udp message;
   uint32_t addr;
   uint16_t port;
   struct saved_message* next;
}saved_message;

typedef struct client {
	char* id;
	int socket;
	int activity; // 0 pt deconectat si 1 pt online
	struct topic* topics;
   struct saved_message* messages;
}client;


//Sursa functii liste: https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm
//insert link at the first location
struct topic* insert_first(struct topic* head, char* name,int sf) {
   //create a link
   struct topic *link = (struct topic*) malloc(sizeof(struct topic));
	
	link->name = (char *) malloc(51 * sizeof(char));
   strcpy(link->name, name);
   link->sf = sf;
	if(head == NULL) {
      head = link;
      link->next = NULL;
      return head;
   }
   //point it to old first node
   link->next = head;
	
   //point first to new first node
   head = link;
   return head;
}

struct saved_message* insert_last(struct saved_message* head, struct message_udp mess, uint32_t addr, uint16_t port){

    struct saved_message* newNode = (struct saved_message*) malloc(sizeof(struct saved_message));

    newNode->message = mess;
    newNode->addr = addr;
    newNode->port = port;
    newNode->next = NULL;

    //need this if there is no node present in linked list at all
    if(head==NULL){
        head = newNode;
        return head;
    }

    struct saved_message* temp = head;

    while(temp->next!=NULL)
        temp = temp->next;

    temp->next = newNode;
    return head;
}

//delete a link with given key
struct topic* delete(struct topic* head, char* name_topic) {

   //start from the first link

   struct topic* current = head;
   struct topic* previous = NULL;
	
   //if list is empty
   if(head == NULL) {
      return NULL;
   }

   //navigate through list
   while(strcmp(current->name, name_topic)) {
      //if it is last node
      if(current->next == NULL) {
         return NULL;
      } else {
         //store reference to current link
         previous = current;
         //move to next link
         current = current->next;
      }
   }

   //found a match, update the link
   if(current == head) {
      //change first to point to next link
      head = head->next;
   } else {
      //bypass the current link
      previous->next = current->next;
   }    
   return current;
}

//find a link with given key
struct topic* find_in_list(struct topic* head, char* name) {

   //start from the first link
   struct topic* current = head;

   //if list is empty
   if(head == NULL) {
      return NULL;
   }

   //navigate through list
   while(current != NULL) {
      
      if (strcmp(current->name, name) == 0) {
         break;
      }
      //if it is last node
      if(current->next == NULL) {
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }      
	
   //if data found, return the current Link
   return current;
}

struct saved_message* free_list(struct saved_message* head)
{
   struct saved_message* tmp;

   while (head != NULL)
    {
       tmp = head;
       head = head->next;
       free(tmp);
    }
   return NULL;
}

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1600	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare

#endif

