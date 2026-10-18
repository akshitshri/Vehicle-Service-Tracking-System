# AutoCare — Vehicle Service Tracking System (DSA Project)

**Student:** Akshit Shrivastava  
**Registration number:** 2502051942  
**Project:** C backend + HTML/CSS/JavaScript frontend

## Overview
A local browser-based service tracker. `backend/server.c` serves the frontend and a small HTTP/JSON API. It stores service records in a singly linked list and waiting service IDs in a FIFO queue, persisting both to local binary files.

## Features
- Create service bookings with customer, vehicle, service type, date, priority and issue.
- Dashboard metrics, search, status filter and sort options.
- Advance service status, view details, delete records and clear all records.
- FIFO queue, process-next action, CSV export and light/dark appearance.
- File-based persistence across server restarts.

## Run the browser application (Windows + GCC/MinGW)
Run commands from the repository root:

```powershell
gcc backend/server.c -o backend/server.exe -lws2_32
.\backend\server.exe
```

Keep the terminal open and visit **http://localhost:8080**. Do not use Live Server for the connected demo.

## Run the separate menu-driven C program
```powershell
gcc backend/main.c backend/service.c -o vehicle_service.exe
.\vehicle_service.exe
```

The menu-driven program is a separate entry point. Its sorting feature uses bubble sort by tracking ID. The browser backend uses linked records, queue operations, linear lookup and local file persistence.

## API
- `GET /api/health`
- `GET /api/services`
- `POST /api/services`
- `PATCH /api/services/{id}/status`
- `DELETE /api/services/{id}`
- `DELETE /api/services`
- `GET /api/queue`
- `POST /api/queue`
- `POST /api/queue/process`

## Core DSA
- Singly linked list for service records.
- FIFO linked queue for waiting service IDs.
- Linear search by tracking ID.
- Bubble sort by ID in the separate CLI program.
- File handling for web-server persistence.

## Repository URL
After publishing, replace this line with the real repository URL in the report and presentation:
`REPLACE_WITH_REAL_GITHUB_REPOSITORY_URL`

## Before submission
1. Compile and run the server.
2. Test create, search/filter, status, queue, delete, refresh/restart and CSV export.
3. Replace the GitHub URL placeholder in the report and presentation.
