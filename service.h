#ifndef SERVICE_H
#define SERVICE_H

#define NAME_LEN 80
#define VEHICLE_LEN 24
#define TYPE_LEN 48
#define STATUS_LEN 32

typedef struct ServiceRecord {
    int id;
    char customer[NAME_LEN];
    char vehicle[VEHICLE_LEN];
    char serviceType[TYPE_LEN];
    char status[STATUS_LEN];
    struct ServiceRecord *next;
} ServiceRecord;

typedef struct QueueNode {
    int serviceId;
    struct QueueNode *next;
} QueueNode;

void addService(void);
void listServices(void);
void searchService(void);
void sortServicesById(void);
void enqueueService(void);
void processNextService(void);
void showQueue(void);
void freeAll(void);

#endif
