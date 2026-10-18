# AutoCare — Vehicle Service Tracking System (DSA Project)

A browser-based vehicle service dashboard connected to a lightweight HTTP server written in C.

## Features
- Create bookings with customer, vehicle, service type, preferred date, priority and issue description.
- Dashboard metrics for total, active, completed and queued services.
- Search by tracking ID, customer, phone, registration plate or service type.
- Filter by status and sort by newest, oldest, customer name or priority.
- Advance service status, view booking details, delete records and clear all records.
- FIFO waiting queue with a “Process next service” action.
- Export records to CSV and switch between light/dark mode.
- Records and queue are saved by the C backend in `backend/services.dat` and `backend/queue.dat`.

## Run the full-stack website (Windows + GCC)

1. Open the `Vehicle-Service-Tracking` project folder in VS Code.
2. Open **Terminal → New Terminal**. Make sure the terminal is in the project root (the folder containing `backend` and `frontend`).
3. Compile the HTTP server:

   ```powershell
   gcc backend/server.c -o backend/server.exe -lws2_32
   ```

4. Start the server:

   ```powershell
   .\backend\server.exe
   ```

5. Keep that terminal open and visit **http://localhost:8080** in your browser.
6. Create a test booking. It should appear in the dashboard and remain after refreshing the page because the C backend saves records to disk.
7. To stop the server, press **Ctrl+C** in the server terminal.

**Important:** For the connected version, open `http://localhost:8080`. Do not open `frontend/index.html` directly and do not use Live Server; the C server serves the page and its API from the same address.

## API endpoints
- `GET /api/health` — check that the C backend is running.
- `GET /api/services` — return all service records.
- `POST /api/services` — create a booking.
- `PATCH /api/services/{id}/status` — update a service status.
- `DELETE /api/services/{id}` — delete one booking.
- `DELETE /api/services` — clear bookings and the queue.
- `GET /api/queue` — show the FIFO queue.
- `POST /api/queue` — add a service ID to the queue.
- `POST /api/queue/process` — process the next queued service.

## DSA concepts to explain in the viva
- **Singly linked list:** stores service records dynamically.
- **FIFO queue:** stores waiting service IDs; the first added service is processed first.
- **Linear search:** finds a service by its ID in the linked list.
- **File handling:** persists the service list and queue between server restarts.
- **HTTP API:** lets the HTML/JavaScript frontend communicate with the C backend.

## Original command-line DSA demo
The menu-based C program is still available separately:

```powershell
gcc backend/main.c backend/service.c -o vehicle_service.exe
.\vehicle_service.exe
```

The command-line program and the website server are separate entry points. For the website demonstration, compile and run `backend/server.c` as described above.

## Troubleshooting
- **Port 8080 already in use:** close the other app using that port, or change `#define PORT 8080` in `backend/server.c` and recompile.
- **`gcc` is not recognized:** reopen VS Code after installing/configuring GCC, then run `gcc --version`.
- **Website cannot connect:** make sure the server is running and that the browser URL is `http://localhost:8080`.
