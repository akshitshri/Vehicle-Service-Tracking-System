#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "service.h"

static ServiceRecord *head = NULL;
static QueueNode *queueFront = NULL;
static QueueNode *queueRear = NULL;
static int nextId = 1001;

static void readLine(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

static ServiceRecord *findById(int id) {
    ServiceRecord *current = head;
    while (current != NULL) {
        if (current->id == id) return current;
        current = current->next;
    }
    return NULL;
}

void addService(void) {
    ServiceRecord *record = malloc(sizeof(ServiceRecord));
    if (!record) {
        puts("Error: not enough memory.");
        return;
    }

    record->id = nextId++;
    readLine("Customer name: ", record->customer, sizeof(record->customer));
    readLine("Vehicle number: ", record->vehicle, sizeof(record->vehicle));
    readLine("Service type: ", record->serviceType, sizeof(record->serviceType));
    strcpy(record->status, "Booked");
    record->next = NULL;

    if (head == NULL) {
        head = record;
    } else {
        ServiceRecord *current = head;
        while (current->next != NULL) current = current->next;
        current->next = record;
    }

    printf("\nService booked successfully. Tracking ID: %d\n", record->id);
}

void listServices(void) {
    if (head == NULL) {
        puts("No service records yet.");
        return;
    }

    puts("\n--- Service Records ---");
    ServiceRecord *current = head;
    while (current != NULL) {
        printf("ID: %d | Customer: %s | Vehicle: %s | Type: %s | Status: %s\n",
               current->id, current->customer, current->vehicle,
               current->serviceType, current->status);
        current = current->next;
    }
}

void searchService(void) {
    int id;
    printf("Enter tracking ID: ");
    if (scanf("%d", &id) != 1) {
        puts("Invalid ID.");
        while (getchar() != '\n') {}
        return;
    }
    while (getchar() != '\n') {}

    ServiceRecord *record = findById(id);
    if (record == NULL) {
        puts("Service record not found.");
        return;
    }
    printf("ID: %d | Customer: %s | Vehicle: %s | Type: %s | Status: %s\n",
           record->id, record->customer, record->vehicle,
           record->serviceType, record->status);
}

void sortServicesById(void) {
    if (head == NULL || head->next == NULL) {
        puts("Not enough records to sort.");
        return;
    }

    /* Bubble sort by swapping record data; list nodes remain linked. */
    for (ServiceRecord *end = NULL; end != head; ) {
        ServiceRecord *current = head;
        ServiceRecord *lastSwap = NULL;
        while (current->next != end) {
            if (current->id > current->next->id) {
                int id = current->id;
                char customer[NAME_LEN], vehicle[VEHICLE_LEN];
                char type[TYPE_LEN], status[STATUS_LEN];

                strcpy(customer, current->customer);
                strcpy(vehicle, current->vehicle);
                strcpy(type, current->serviceType);
                strcpy(status, current->status);

                current->id = current->next->id;
                strcpy(current->customer, current->next->customer);
                strcpy(current->vehicle, current->next->vehicle);
                strcpy(current->serviceType, current->next->serviceType);
                strcpy(current->status, current->next->status);

                current->next->id = id;
                strcpy(current->next->customer, customer);
                strcpy(current->next->vehicle, vehicle);
                strcpy(current->next->serviceType, type);
                strcpy(current->next->status, status);
                lastSwap = current;
            }
            current = current->next;
        }
        if (lastSwap == NULL) break;
        end = lastSwap;
    }
    puts("Records sorted by tracking ID.");
}

void enqueueService(void) {
    int id;
    printf("Enter tracking ID to add to waiting queue: ");
    if (scanf("%d", &id) != 1) {
        puts("Invalid ID.");
        while (getchar() != '\n') {}
        return;
    }
    while (getchar() != '\n') {}

    if (findById(id) == NULL) {
        puts("No service exists with that ID.");
        return;
    }

    QueueNode *node = malloc(sizeof(QueueNode));
    if (!node) {
        puts("Error: not enough memory.");
        return;
    }
    node->serviceId = id;
    node->next = NULL;

    if (queueRear == NULL) {
        queueFront = queueRear = node;
    } else {
        queueRear->next = node;
        queueRear = node;
    }
    printf("Service %d added to the waiting queue.\n", id);
}

void processNextService(void) {
    if (queueFront == NULL) {
        puts("Waiting queue is empty.");
        return;
    }

    QueueNode *node = queueFront;
    queueFront = queueFront->next;
    if (queueFront == NULL) queueRear = NULL;

    ServiceRecord *record = findById(node->serviceId);
    if (record != NULL) {
        strcpy(record->status, "In Progress");
        printf("Service %d is now In Progress.\n", record->id);
    } else {
        printf("Service %d was removed from queue but its record no longer exists.\n",
               node->serviceId);
    }
    free(node);
}

void showQueue(void) {
    if (queueFront == NULL) {
        puts("Waiting queue is empty.");
        return;
    }
    puts("Waiting queue (FIFO):");
    QueueNode *current = queueFront;
    while (current != NULL) {
        printf("%d", current->serviceId);
        if (current->next != NULL) printf(" -> ");
        current = current->next;
    }
    putchar('\n');
}

void freeAll(void) {
    while (head != NULL) {
        ServiceRecord *next = head->next;
        free(head);
        head = next;
    }
    while (queueFront != NULL) {
        QueueNode *next = queueFront->next;
        free(queueFront);
        queueFront = next;
    }
    queueRear = NULL;
}
