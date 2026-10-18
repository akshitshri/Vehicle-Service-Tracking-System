#include <stdio.h>
#include "service.h"

int main(void) {
    int choice;

    do {
        puts("\n==================================");
        puts(" VEHICLE SERVICE TRACKING SYSTEM ");
        puts("==================================");
        puts("1. Book a service");
        puts("2. View all service records");
        puts("3. Search service by tracking ID");
        puts("4. Sort records by tracking ID");
        puts("5. Add service to waiting queue");
        puts("6. Process next service in queue");
        puts("7. View waiting queue");
        puts("0. Exit");
        printf("Choose an option: ");

        if (scanf("%d", &choice) != 1) {
            puts("Please enter a valid number.");
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            choice = -1;
            continue;
        }
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}

        switch (choice) {
            case 1: addService(); break;
            case 2: listServices(); break;
            case 3: searchService(); break;
            case 4: sortServicesById(); break;
            case 5: enqueueService(); break;
            case 6: processNextService(); break;
            case 7: showQueue(); break;
            case 0: puts("Goodbye!"); break;
            default: puts("Invalid option. Try again.");
        }
    } while (choice != 0);

    freeAll();
    return 0;
}
