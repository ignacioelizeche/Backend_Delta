# Backend_Delta

**Backend** for the Delta project, implemented in **C++** using **Qt** and custom HTTP routing.  
This repository provides the server-side logic and endpoints consumed by the frontend.

> ⚠️ This project is designed to run on **port 8080**.

---

## 🚀 Purpose

This backend handles:

- User authentication (registration, login) via JWT
- RESTful API routes for modules like exams, calendar, forum, problems, dashboard, visualizations, rankings, etc.
- SQLite database operations
- Consistent API responses

---

## 🗂️ Project Structure

```

/
├── CMakeLists.txt              # Build configuration
├── main.cpp                    # Application entry point
├── api.cpp / api.h             # Core HTTP server and routing
├── \*\_routes.cpp / \*\_routes.h   # Routes for each API module
├── jwt\_helper.cpp / .h         # JWT creation and validation
├── database\_manager.cpp / .h   # Database abstraction layer
├── response\_utils.cpp / .h     # API response helpers
└── build/                      # CMake build artifacts (ignored)

````

---

## 🧰 Requirements

- **Qt (6.x or 5.x)** – Required for building the project  
- **CMake**
- **C++17** or later  

Make sure Qt is installed and properly set up in your environment.

---

## 🛠️ Building & Running

1. **Clone the repo**  
   ```bash
   git clone https://github.com/ignacioelizeche/Backend_Delta.git
   cd Backend_Delta
    ````

2. **Create build directory and run CMake**

   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run the server**

   ```bash
   ./DeltaBackend
   ```

   The server will start on:
   ➤ `http://localhost:8080`

---

## 📡 Example Endpoints

* `POST /auth/register` – Register user
* `POST /auth/login` – Login and receive JWT
* `GET /exams`, `POST /exams` – Manage exams
* `GET /calendar` – Fetch calendar events
* `GET /forum`, `POST /forum` – Forum endpoints
* And many more across routes: problems, dashboard, rankings, etc.

Explore the `*_routes.cpp` files for full API definitions.

---

## 🔐 Environment Setup

You can define environment variables or config settings like:

* `PORT` (default is **8080**)

---
