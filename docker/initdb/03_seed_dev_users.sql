BEGIN;

-- 1. Команды
INSERT INTO teams (id, name, tag, color_hex, avatar_url)
VALUES 
    (1, 'Cyber Uram',    'CYB', '#a614d3', 'https://api.dicebear.com/7.x/identicon/svg?seed=CYB'),
    (2, 'Kazan Striders', 'STR', '#FF9100', 'https://api.dicebear.com/7.x/identicon/svg?seed=STR'),
    (3, 'Neon Runners',   'NEO', '#11FFFF', 'https://api.dicebear.com/7.x/identicon/svg?seed=NEO')
ON CONFLICT (id) DO UPDATE SET 
    name = EXCLUDED.name,
    tag = EXCLUDED.tag,
    color_hex = EXCLUDED.color_hex,
    avatar_url = EXCLUDED.avatar_url;

-- 2. Пользователи
INSERT INTO users (id, username, email, password_hash, team_id, avatar_url, player_color_hex)
VALUES 
    (
        1,
        'smayl1ks', 
        'smayl1ks@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo',
        1,
        'https://ui-avatars.com/api/?name=smayl1ks&background=a614d3&color=fff',
        '#a614d3'
    ),
    (
        2,
        'tonitaga', 
        'tonitaga@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo',
        2,
        'https://ui-avatars.com/api/?name=tonitaga&background=FF9100&color=fff',
        '#FF9100'
    ),
    (
        3,
        'antonk', 
        'antonk@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo',
        3,
        'https://ui-avatars.com/api/?name=antonk&background=11FFFF&color=000',
        '#11FFFF'
    )
ON CONFLICT (id) DO UPDATE SET 
    username = EXCLUDED.username,
    email = EXCLUDED.email,
    password_hash = EXCLUDED.password_hash,
    team_id = EXCLUDED.team_id,
    avatar_url = EXCLUDED.avatar_url,
    player_color_hex = EXCLUDED.player_color_hex,
    updated_at = NOW();

SELECT setval('teams_id_seq', (SELECT COALESCE(MAX(id), 1) FROM teams));
SELECT setval('users_id_seq', (SELECT COALESCE(MAX(id), 1) FROM users));

INSERT INTO user_stats (user_id)
SELECT id FROM users
ON CONFLICT (user_id) DO NOTHING;

-- 3. Пробежки

INSERT INTO runs (id, user_id, status, total_distance_meters, total_duration_seconds, uram_points_earned, started_at, finished_at)
VALUES (1, 1, 'finished', 5100.0, 1750, 560, NOW() - INTERVAL '40 minutes', NOW() - INTERVAL '10 minutes')
ON CONFLICT (id) DO UPDATE SET
    total_distance_meters = EXCLUDED.total_distance_meters,
    total_duration_seconds = EXCLUDED.total_duration_seconds,
    uram_points_earned = EXCLUDED.uram_points_earned;

INSERT INTO runs (id, user_id, status, total_distance_meters, total_duration_seconds, uram_points_earned, started_at, finished_at)
VALUES (2, 2, 'finished', 3500.0, 1260, 390, NOW() - INTERVAL '2 hours', NOW() - INTERVAL '1 hour 38 minutes')
ON CONFLICT (id) DO UPDATE SET
    total_distance_meters = EXCLUDED.total_distance_meters,
    total_duration_seconds = EXCLUDED.total_duration_seconds,
    uram_points_earned = EXCLUDED.uram_points_earned;

INSERT INTO runs (id, user_id, status, total_distance_meters, total_duration_seconds, uram_points_earned, started_at, finished_at)
VALUES (3, 3, 'finished', 2800.0, 960, 290, NOW() - INTERVAL '3 hours', NOW() - INTERVAL '2 hours 44 minutes')
ON CONFLICT (id) DO UPDATE SET
    total_distance_meters = EXCLUDED.total_distance_meters,
    total_duration_seconds = EXCLUDED.total_duration_seconds,
    uram_points_earned = EXCLUDED.uram_points_earned;

DELETE FROM run_points WHERE run_id IN (1, 2, 3);

INSERT INTO run_points (run_id, latitude, longitude, altitude, speed, h3_index, recorded_at) VALUES
(1, 55.784500, 49.118500, 53.0, 3.2, 617286173153820671, NOW() - INTERVAL '40 minutes'),
(1, 55.783000, 49.117200, 53.0, 3.3, 617286173153820671, NOW() - INTERVAL '38 minutes'),
(1, 55.780500, 49.117800, 53.1, 3.4, 617286173152509951, NOW() - INTERVAL '35 minutes'),
(1, 55.777500, 49.119200, 52.8, 3.3, 617286173153034239, NOW() - INTERVAL '32 minutes'),
(1, 55.774500, 49.121000, 52.9, 3.2, 617286173156966399, NOW() - INTERVAL '29 minutes'),
(1, 55.771500, 49.123500, 53.0, 3.5, 617286173157490687, NOW() - INTERVAL '26 minutes'),
(1, 55.768500, 49.126500, 53.2, 3.4, 617286173149364223, NOW() - INTERVAL '23 minutes'),
(1, 55.766800, 49.129500, 53.0, 3.1, 617286173148315647, NOW() - INTERVAL '20 minutes'),
(1, 55.769500, 49.130500, 52.8, 3.3, 617286173148577791, NOW() - INTERVAL '18 minutes'),
(1, 55.770534, 49.127745, 53.0, 3.2, 617286173149626367, NOW() - INTERVAL '17 minutes'),
(1, 55.773000, 49.128000, 53.0, 3.2, 617286173145694207, NOW() - INTERVAL '16 minutes'),
(1, 55.776500, 49.125500, 53.1, 3.3, 617286173145169919, NOW() - INTERVAL '14 minutes'),
(1, 55.779500, 49.123500, 53.0, 3.4, 617286173153296383, NOW() - INTERVAL '13 minutes'),
(1, 55.782500, 49.121500, 53.0, 3.2, 617286173152772095, NOW() - INTERVAL '12 minutes'),
(1, 55.784200, 49.120500, 53.0, 3.1, 617286173152772095, NOW() - INTERVAL '10 minutes');

INSERT INTO run_points (run_id, latitude, longitude, altitude, speed, h3_index, recorded_at) VALUES
(2, 55.796000, 49.148000, 65.0, 3.1, 617286173337059327, NOW() - INTERVAL '2 hours'),
(2, 55.794000, 49.152000, 62.0, 3.3, 617286173329719295, NOW() - INTERVAL '1 hour 55 minutes'),
(2, 55.793989, 49.148289, 62.0, 3.3, 617286173337583615, NOW() - INTERVAL '1 hour 52 minutes'),
(2, 55.791500, 49.155000, 58.0, 3.4, 617286173330243583, NOW() - INTERVAL '1 hour 50 minutes'),
(2, 55.789000, 49.152500, 55.0, 3.2, 617286173342040063, NOW() - INTERVAL '1 hour 46 minutes'),
(2, 55.791000, 49.147000, 60.0, 3.5, 617286173341515775, NOW() - INTERVAL '1 hour 43 minutes'),
(2, 55.793500, 49.145000, 63.0, 3.2, 617286173338632191, NOW() - INTERVAL '1 hour 38 minutes');

INSERT INTO run_points (run_id, latitude, longitude, altitude, speed, h3_index, recorded_at) VALUES
(3, 55.798500, 49.105500, 70.0, 3.0, 617286173124460543, NOW() - INTERVAL '3 hours'),
(3, 55.800500, 49.106800, 75.0, 3.2, 617286173120528383, NOW() - INTERVAL '2 hours 56 minutes'),
(3, 55.802000, 49.109500, 72.0, 3.3, 617286173118955519, NOW() - INTERVAL '2 hours 52 minutes'),
(3, 55.801000, 49.113000, 60.0, 3.5, 617286173119741951, NOW() - INTERVAL '2 hours 48 minutes'),
(3, 55.798800, 49.111500, 65.0, 3.1, 617286173119479807, NOW() - INTERVAL '2 hours 45 minutes'),
(3, 55.797200, 49.108500, 68.0, 3.2, 617286173124460543, NOW() - INTERVAL '2 hours 44 minutes');

SELECT setval('runs_id_seq', (SELECT COALESCE(MAX(id), 1) FROM runs));
SELECT setval('run_points_id_seq', (SELECT COALESCE(MAX(id), 1) FROM run_points));

-- 4. Захват гексагонов

INSERT INTO hexagons (h3_index, owner_user_id, top_score, captured_at) VALUES
(617286173153820671, 1, 180, NOW() - INTERVAL '40 minutes'),
(617286173152509951, 1, 175, NOW() - INTERVAL '35 minutes'),
(617286173153034239, 1, 160, NOW() - INTERVAL '32 minutes'),
(617286173156966399, 1, 165, NOW() - INTERVAL '29 minutes'),
(617286173157490687, 1, 150, NOW() - INTERVAL '26 minutes'),
(617286173149364223, 1, 155, NOW() - INTERVAL '23 minutes'),
(617286173148315647, 1, 170, NOW() - INTERVAL '20 minutes'),
(617286173148577791, 1, 160, NOW() - INTERVAL '18 minutes'),
(617286173149626367, 1, 160, NOW() - INTERVAL '17 minutes'),
(617286173145694207, 1, 165, NOW() - INTERVAL '16 minutes'),
(617286173145169919, 1, 190, NOW() - INTERVAL '14 minutes'),
(617286173153296383, 1, 185, NOW() - INTERVAL '13 minutes'),
(617286173152772095, 1, 195, NOW() - INTERVAL '10 minutes'),

(617286173337059327, 2, 140, NOW() - INTERVAL '2 hours'),
(617286173329719295, 2, 150, NOW() - INTERVAL '1 hour 55 minutes'),
(617286173337583615, 2, 155, NOW() - INTERVAL '1 hour 52 minutes'),
(617286173330243583, 2, 160, NOW() - INTERVAL '1 hour 50 minutes'),
(617286173342040063, 2, 145, NOW() - INTERVAL '1 hour 46 minutes'),
(617286173341515775, 2, 155, NOW() - INTERVAL '1 hour 43 minutes'),
(617286173338632191, 2, 165, NOW() - INTERVAL '1 hour 38 minutes'),

(617286173124460543, 3, 150, NOW() - INTERVAL '3 hours'),
(617286173120528383, 3, 160, NOW() - INTERVAL '2 hours 56 minutes'),
(617286173118955519, 3, 170, NOW() - INTERVAL '2 hours 52 minutes'),
(617286173119741951, 3, 165, NOW() - INTERVAL '2 hours 48 minutes'),
(617286173119479807, 3, 155, NOW() - INTERVAL '2 hours 45 minutes')

ON CONFLICT (h3_index) DO UPDATE SET 
    owner_user_id = EXCLUDED.owner_user_id,
    top_score = EXCLUDED.top_score,
    captured_at = EXCLUDED.captured_at;

-- 5. Статистика в гексагонах для лидербордов (hexagon_user_stats)
INSERT INTO hexagon_user_stats (h3_index, user_id, uram_points, total_distance_meters, visits_count, last_visited_at) VALUES

(617286173153820671, 1, 180, 520.0, 2, NOW() - INTERVAL '40 minutes'),
(617286173152509951, 1, 175, 480.0, 2, NOW() - INTERVAL '35 minutes'),
(617286173153034239, 1, 160, 410.0, 1, NOW() - INTERVAL '32 minutes'),
(617286173156966399, 1, 165, 430.0, 1, NOW() - INTERVAL '29 minutes'),
(617286173157490687, 1, 150, 390.0, 1, NOW() - INTERVAL '26 minutes'),
(617286173149364223, 1, 155, 400.0, 1, NOW() - INTERVAL '23 minutes'),
(617286173148315647, 1, 170, 460.0, 1, NOW() - INTERVAL '20 minutes'),
(617286173148577791, 1, 160, 420.0, 1, NOW() - INTERVAL '18 minutes'),
(617286173149626367, 1, 160, 430.0, 1, NOW() - INTERVAL '17 minutes'),
(617286173145694207, 1, 165, 440.0, 1, NOW() - INTERVAL '16 minutes'),
(617286173145169919, 1, 190, 550.0, 2, NOW() - INTERVAL '14 minutes'),
(617286173153296383, 1, 185, 530.0, 2, NOW() - INTERVAL '13 minutes'),
(617286173152772095, 1, 195, 580.0, 2, NOW() - INTERVAL '10 minutes'),

(617286173153820671, 2, 95, 260.0, 1, NOW() - INTERVAL '1 day'),
(617286173152772095, 2, 110, 310.0, 1, NOW() - INTERVAL '1 day'),

(617286173337059327, 2, 140, 450.0, 1, NOW() - INTERVAL '2 hours'),
(617286173329719295, 2, 150, 520.0, 1, NOW() - INTERVAL '1 hour 55 minutes'),
(617286173337583615, 2, 155, 540.0, 1, NOW() - INTERVAL '1 hour 52 minutes'),
(617286173330243583, 2, 160, 560.0, 1, NOW() - INTERVAL '1 hour 50 minutes'),
(617286173342040063, 2, 145, 480.0, 1, NOW() - INTERVAL '1 hour 46 minutes'),
(617286173341515775, 2, 155, 510.0, 1, NOW() - INTERVAL '1 hour 43 minutes'),
(617286173338632191, 2, 165, 590.0, 1, NOW() - INTERVAL '1 hour 38 minutes'),

(617286173124460543, 3, 150, 480.0, 1, NOW() - INTERVAL '3 hours'),
(617286173120528383, 3, 160, 540.0, 1, NOW() - INTERVAL '2 hours 56 minutes'),
(617286173118955519, 3, 170, 580.0, 1, NOW() - INTERVAL '2 hours 52 minutes'),
(617286173119741951, 3, 165, 560.0, 1, NOW() - INTERVAL '2 hours 48 minutes'),
(617286173119479807, 3, 155, 510.0, 1, NOW() - INTERVAL '2 hours 45 minutes')

ON CONFLICT (h3_index, user_id) DO UPDATE SET
    uram_points = EXCLUDED.uram_points,
    total_distance_meters = EXCLUDED.total_distance_meters,
    visits_count = EXCLUDED.visits_count,
    last_visited_at = EXCLUDED.last_visited_at;

-- 6. Обновление общей статистики

UPDATE user_stats SET 
    total_distance_meters = 5100.0,
    total_duration_seconds = 1750,
    total_runs = 1,
    total_uram_points = 560,
    current_held_hexagons = 13
WHERE user_id = 1;

UPDATE user_stats SET 
    total_distance_meters = 3500.0,
    total_duration_seconds = 1260,
    total_runs = 1,
    total_uram_points = 390,
    current_held_hexagons = 7
WHERE user_id = 2;

UPDATE user_stats SET 
    total_distance_meters = 2800.0,
    total_duration_seconds = 960,
    total_runs = 1,
    total_uram_points = 290,
    current_held_hexagons = 5
WHERE user_id = 3;

COMMIT;