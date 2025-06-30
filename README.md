# Backend-Database-Systems-Implementation
COMP9315 Projects involving PostgreSQL backend internals, focusing on query operators, storage, and system-level database components using C.

# COMP9315 – Database Systems Implementation (UNSW 24T1)

This repository contains coursework from COMP9315 at UNSW, focusing on low-level implementation of core components in a relational database system. Assignments were completed in C, involving direct modifications and extensions to PostgreSQL backend modules. The projects required an in-depth understanding of query evaluation, storage and buffer management, and internal performance engineering.

---

## 🔧 Projects Overview

### 🧪 Assignment 1 – Query Operators and Buffer Access
- Implemented custom query operators at the executor level.
- Interfaced with PostgreSQL buffer and page structures using `BufPageGetItem`, `ExecStoreTuple`, and related functions.
- Integrated C-based logic into the PostgreSQL execution pipeline for controlled tuple output and condition evaluation.

### 📦 Assignment 2 – Storage-Level System Modifications
- Built custom tuple insertion and retrieval logic using PostgreSQL low-level storage APIs.
- Modified page structures using `PageAddItem` and handled visibility using `ItemPointer` and block-level metadata.
- Implemented a `setPages()` function for bulk data population, and `getPages()` for targeted tuple scanning.

---

## 🛠️ Tech Stack
- **Language**: C
- **System**: PostgreSQL 15+
- **Tools**: GDB, Makefiles, Unix/Linux environment
- **Editor**: VS Code / Vim (recommended with C syntax highlighting)

---

## 📚 Key Concepts
- Query execution internals (executor, plan nodes)
- Storage formats and page layout in PostgreSQL
- Buffer pool management and tuple visibility
- PostgreSQL system catalog and backend extension
- Low-level memory and pointer manipulation in C

---

## 🚀 Getting Started

> This repository contains code built against a specific version of PostgreSQL and requires a Linux-like environment.

1. Clone the official PostgreSQL source:
   ```bash
   git clone https://github.com/postgres/postgres.git
   cd postgres

