# 🚀 Run Uram Server — Development Roadmap

---

## 1. База данных (Database & Cache Infrastructure)

### 1.1. Схема БД (Migrations & DDL)
- [ ] **DDL `users`**: создать скрипт миграции для таблицы пользователей (`id`, `username`, `email`, `password_hash`, `created_at`, `updated_at`).
- [ ] **DDL `user_sessions`**: создать таблицу `refresh_tokens` (`id`, `user_id`, `token_hash`, `device_id`, `expires_at`, `is_revoked`).
- [ ] **DDL `runs` & `location_frames`**: таблицы для пробежек и геометрии (`coordinates`, `timestamps`, `speed`, `distance`, `h3_index`).

### 1.2. Пул соединений PostgreSQL (`libpqxx`)
- [ ] **`DbConnectionPool`**: реализовать асинхронный/потокобезопасный менеджер соединений к PostgreSQL в `include/database/db_pool.hpp`.
- [ ] **Fault Tolerance**: добавить таймауты запросов, механизмы retry и стратегии переподключения (reconnect).

### 1.3. Интеграция Redis (`boost-redis`)
- [ ] **Redis Client**: настроить `boost::redis::connection` в `io_context` для хранения активных сессий и кэша гексагонов H3.
- [ ] **TTL Management**: реализовать методы для автоматического продления и инвалидации ключей сессий.

---

## 2. Слой доступа к данным (Repository Pattern)

### 2.1. Интерфейсы репозиториев (`include/repository/`)
- [ ] `IUserRepository`: декларировать методы поиска (`FindById`, `FindByEmail`), создания и обновления статуса.
- [ ] `IRunRepository`: декларировать методы старта пробежки, пакетной записи `LocationFrame` и финализации.

### 2.2. Реализация на `libpqxx` (`src/repository/`)
- [ ] `PgUserRepository`: реализовать через Prepared Statements для защиты от SQL-инъекций.
- [ ] `PgRunRepository`: реализовать с использованием транзакций (`pqxx::work`) для пакетной записи геоданных.

### 2.3. Асинхронный Worker Pool
- [ ] Вынести все блокирующие сетевые/дисковые вызовы `libpqxx` в отдельный `net::defer` / worker pool, чтобы не блокировать event loop Asio.

---

## 3. Аутентификация (Auth Service)

### 3.1. Хэширование (`include/services/auth_service.hpp`)
- [ ] **`libsodium`**: интегрировать `Argon2id` (`crypto_pwhash`) для `HashPassword` и `VerifyPassword`.
- [ ] **Crypto Random**: реализовать генерацию безопасных солей и UUIDv4 для Refresh-токенов.

### 3.2. Работа с JWT (`jwt-cpp`)
- [ ] **Access Token**: реализовать генерацию короткоживущего JWT (HS256, claims: `user_id`, `exp`, `iss`).
- [ ] **JWT Verifier**: реализовать декодирование и проверку подписи/срока годности JWT.

### 3.3. Бизнес-логика `AuthService`
- [ ] `RegisterUser(email, password)` — регистрация, хэширование пароля и запись в БД.
- [ ] `Authenticate(email, password)` — проверка пароля и генерация пары `(access_token, refresh_token)`.
- [ ] `ValidateToken(token)` — верификация подписи токена для WebSocket Handshake.
- [ ] `RefreshToken(refresh_token)` — валидация Refresh-токена в DB/Redis и перевыпуск Access-токена.

---

## 4. Обработчики пакетов и роутинг (Packet Handlers & Protobuf Router)

### 4.1. Схема Protobuf (`proto/`)
- [ ] **`proto/envelope.proto`**: добавить `AuthRequest` и `AuthResponse` в `oneof payload`.
- [ ] **`proto/auth.proto`**: создать сообщения авторизации и ответа.

### 4.2. Диспетчер Пакетов (Packet Router)
- [ ] `PacketDispatcher`: реализовать парсинг входного бинарного кадра `Envelope`.
- [ ] Роутинг: добавить маппинг `Envelope::payload_case()` на лямбды/хэндлеры.

### 4.3. Конкретные хэндлеры (`include/handlers/`)
- [ ] `RunHandler`: обработка `StartRunRequest` / `FinishRunRequest`.
- [ ] `TelemetryHandler`: валидация потока `LocationFrame`, расчёт гексагонов и отправка `LocationFrameAck`.

### 4.4. Валидация и обработка ошибок
- [ ] **Auth Guard**: добавление проверки флага `is_authenticated` в сессии перед вызовом бизнес-хэндлеров.
- [ ] **Error Handling**: единая структура формирования Protobuf-ответов со статусами ошибок.

---

## 5. Сетевой слой, Сессии и Тестирование

### 5.1. Привязка Auth к `UserSession`
- [ ] Сохранение `user_id`, `device_id` и состояния авторизации внутри объекта `UserSession`.
- [ ] Инвалидация и удаление сессии из `SessionManager` при разрыве TCP/TLS соединения.

### 5.2. Unit & Integration Tests (`tests/`)
- [ ] **GTest AuthService**: тесты для Argon2id, генерации и валидации JWT.
- [ ] **GTest PacketDispatcher**: тесты корректности разбора `Envelope` пакетов.
- [ ] **Интеграционные тесты Репозиториев**: тесты выполнения SQL-запросов к тестовой базе PostgreSQL в Docker.