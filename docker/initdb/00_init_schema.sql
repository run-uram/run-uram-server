-- 1. Пользователи
CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- 2. Пробежки
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

-- 3. Общая карта
CREATE TABLE IF NOT EXISTS hexagons (
    h3_index BIGINT PRIMARY KEY,
    owner_user_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    score INT NOT NULL DEFAULT 100,
    captured_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_hexagons_owner ON hexagons(owner_user_id);