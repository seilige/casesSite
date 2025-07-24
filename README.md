# 🎲 Case Opening Website in C++ (Drogon Framework)

This is a web application written in modern C++ using the [Drogon](https://github.com/drogonframework/drogon) web framework. The site simulates case openings with pseudo-random drop chances. It includes user authentication, an inventory system, and unlimited case spins.

## ⚙️ Features

- 🔐 **User Authentication** – registration and login with username/password.
- 🎒 **Inventory** – each user has a personal inventory for storing dropped items.
- 🎁 **Case Opening System** – three types of cases, each with unique drop chances.
  - Drop chances are implemented using a pseudo-random number generator.
  - Unlimited case openings – no balance restrictions.
- 📊 **Optional Database** – support for SQLite/PostgreSQL (if enabled).

## 🛠️ Tech Stack

- C++17 or newer
- [Drogon Web Framework](https://github.com/drogonframework/drogon)
- CMake for build configuration
- JSON library (e.g. [nlohmann/json](https://github.com/nlohmann/json), if used)
- SQLite or PostgreSQL (optional)
