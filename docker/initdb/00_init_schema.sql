-- 1. Пользователи
CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- 2. Сессии и Refresh-токены
CREATE TABLE IF NOT EXISTS refresh_tokens (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash VARCHAR(64) NOT NULL, -- SHA-256 хэш токена
    device_id VARCHAR(128) NOT NULL,
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    is_revoked BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_refresh_tokens_user_id ON refresh_tokens(user_id);
CREATE INDEX IF NOT EXISTS idx_refresh_tokens_token_hash ON refresh_tokens(token_hash);

-- 3. Пробежки (Сводная информация)
CREATE TABLE IF NOT EXISTS runs (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) NOT NULL DEFAULT 'active', -- 'active', 'finished', 'cancelled'
    total_distance_meters DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    total_duration_seconds BIGINT NOT NULL DEFAULT 0,
    started_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    finished_at TIMESTAMP WITH TIME ZONE
);

CREATE INDEX IF NOT EXISTS idx_runs_user_id ON runs(user_id);
CREATE INDEX IF NOT EXISTS idx_runs_status ON runs(status);

-- 4. Телеметрия пробежки (GPS трек)
CREATE TABLE IF NOT EXISTS location_frames (
    id BIGSERIAL PRIMARY KEY,
    run_id BIGINT NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
    sequence_number BIGINT NOT NULL,
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    altitude DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    speed REAL NOT NULL DEFAULT 0.0,
    heading REAL NOT NULL DEFAULT 0.0,
    accuracy REAL NOT NULL DEFAULT 0.0,
    h3_index BIGINT NOT NULL, -- 64-битный Uber H3 Cell ID
    recorded_at TIMESTAMP WITH TIME ZONE NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_location_frames_run_id_seq ON location_frames(run_id, sequence_number);

-- 5. Карта территорий (Состояние гексагонов)
CREATE TABLE IF NOT EXISTS hexagons (
    h3_index BIGINT PRIMARY KEY,
    owner_user_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    score INT NOT NULL DEFAULT 100,
    captured_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_hexagons_owner ON hexagons(owner_user_id);

-- 6. Лог захвата территорий
CREATE TABLE IF NOT EXISTS territory_captures (
    id BIGSERIAL PRIMARY KEY,
    run_id BIGINT REFERENCES runs(id) ON DELETE SET NULL,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    h3_index BIGINT NOT NULL,
    previous_owner_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    score_gained INT NOT NULL DEFAULT 0,
    captured_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_territory_captures_user_id ON territory_captures(user_id);
CREATE INDEX IF NOT EXISTS idx_territory_captures_h3_index ON territory_captures(h3_index);