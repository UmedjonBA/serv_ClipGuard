CREATE TABLE clipboard_events (
    id SERIAL PRIMARY KEY,
    username VARCHAR(256) NOT NULL,
    hostname VARCHAR(256) NOT NULL,
    event_type VARCHAR(50) NOT NULL,
    content TEXT,
    file_path TEXT,
    file_type VARCHAR(50),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
); 