CREATE TABLE IF NOT EXISTS hexagons (
    h3_index BIGINT PRIMARY KEY,
    owner_user_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    top_score INT NOT NULL DEFAULT 0, -- in Uram Points
    captured_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_hexagons_owner ON hexagons(owner_user_id);

CREATE TABLE IF NOT EXISTS hexagon_user_stats (
    h3_index BIGINT NOT NULL,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    uram_points INT NOT NULL DEFAULT 0,
    total_distance_meters DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    visits_count INT NOT NULL DEFAULT 0,
    last_visited_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    PRIMARY KEY (h3_index, user_id)
);

CREATE INDEX IF NOT EXISTS idx_hex_user_leaderboard 
ON hexagon_user_stats (h3_index, uram_points DESC);

CREATE TABLE IF NOT EXISTS hexagon_history (
    id BIGSERIAL PRIMARY KEY,
    h3_index BIGINT NOT NULL,
    previous_owner_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    new_owner_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    score_at_capture INT NOT NULL,
    captured_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_hex_history_h3 ON hexagon_history(h3_index);