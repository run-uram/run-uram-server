INSERT INTO teams (id, name, tag, color_hex)
VALUES 
    (1, 'Cyber Uram',      'CYB', '#a614d3'),
    (2, 'Kazan Striders',  'STR', '#FF9100'),
    (3, 'Neon Runners',    'NEO', '#11FFFF')
ON CONFLICT (id) DO UPDATE SET 
    name = EXCLUDED.name,
    tag = EXCLUDED.tag,
    color_hex = EXCLUDED.color_hex;

INSERT INTO users (username, email, password_hash, team_id, player_color_hex)
VALUES 
    (
        'smayl1ks', 
        'smayl1ks@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo',
        1,
        '#a614d3'
    ),
    (
        'tonitaga', 
        'tonitaga@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo',
        2,
        '#FF9100'
    ),
    (
        'antonk', 
        'antonk@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo',
        3,
        '#11FFFF'
    )

ON CONFLICT (username) 
DO UPDATE SET 
    password_hash = EXCLUDED.password_hash,
    team_id = EXCLUDED.team_id,
    player_color_hex = EXCLUDED.player_color_hex,
    updated_at = NOW();

SELECT setval('teams_id_seq', (SELECT COALESCE(MAX(id), 1) FROM teams));
SELECT setval('users_id_seq', (SELECT COALESCE(MAX(id), 1) FROM users));