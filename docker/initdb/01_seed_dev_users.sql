INSERT INTO users (username, email, password_hash)
VALUES 
    (
        'smayl1ks', 
        'smayl1ks@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo'
    ),
    (
        'tonitaga', 
        'tonitaga@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo'
    ),
    (
        'antonk', 
        'antonk@runuram.dev', 
        '$argon2id$v=19$m=65536,t=2,p=1$B3K4dJJG8xeagw4XSrYtSg$5ztxPfORNaUJDZ5phrx8C1JSdl6yLGuPl0nPMu8LvIo'
    )
ON CONFLICT (username) 
DO UPDATE SET 
    password_hash = EXCLUDED.password_hash,
    updated_at = NOW();