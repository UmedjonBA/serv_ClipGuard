from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import psycopg2
from psycopg2.extras import RealDictCursor
from datetime import datetime
from typing import List, Optional
from pydantic import BaseModel

app = FastAPI(title="Clipboard Monitor API")

# Настройка CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # В продакшене замените на конкретные домены
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Модели данных
class ClipboardEvent(BaseModel):
    id: int
    username: str
    hostname: str
    event_type: str
    content: Optional[str]
    file_path: Optional[str]
    file_type: Optional[str]
    created_at: datetime

class ClientInfo(BaseModel):
    username: str
    hostname: str
    total_events: int
    last_activity: datetime

# Конфигурация базы данных
DB_CONFIG = {
    "dbname": "clipboard_monitor",
    "user": "postgres",
    "password": "your_password",  # Замените на ваш пароль
    "host": "localhost",
    "port": "5432"
}

def get_db_connection():
    return psycopg2.connect(**DB_CONFIG)

@app.get("/events/", response_model=List[ClipboardEvent])
async def get_events(
    skip: int = 0,
    limit: int = 100,
    username: Optional[str] = None,
    event_type: Optional[str] = None
):
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        query = "SELECT * FROM public.clipboard_events WHERE 1=1"
        params = []
        
        if username:
            query += " AND username = %s"
            params.append(username)
        if event_type:
            query += " AND event_type = %s"
            params.append(event_type)
            
        query += " ORDER BY created_at DESC LIMIT %s OFFSET %s"
        params.extend([limit, skip])
        
        cur.execute(query, params)
        events = cur.fetchall()
        
        cur.close()
        conn.close()
        
        return events
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/events/{event_id}", response_model=ClipboardEvent)
async def get_event(event_id: int):
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        cur.execute(
            "SELECT * FROM public.clipboard_events WHERE id = %s",
            [event_id]
        )
        event = cur.fetchone()
        
        cur.close()
        conn.close()
        
        if event is None:
            raise HTTPException(status_code=404, detail="Event not found")
            
        return event
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/stats/")
async def get_stats():
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        # Общее количество событий
        cur.execute("SELECT COUNT(*) as total FROM public.clipboard_events")
        total = cur.fetchone()['total']
        
        # Количество событий по типам
        cur.execute("""
            SELECT event_type, COUNT(*) as count 
            FROM public.clipboard_events 
            GROUP BY event_type
        """)
        by_type = cur.fetchall()
        
        # Количество событий по пользователям
        cur.execute("""
            SELECT username, COUNT(*) as count 
            FROM public.clipboard_events 
            GROUP BY username
        """)
        by_user = cur.fetchall()
        
        cur.close()
        conn.close()
        
        return {
            "total_events": total,
            "by_type": by_type,
            "by_user": by_user
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/all/", response_model=List[ClipboardEvent])
async def get_all_events(
    skip: int = 0,
    limit: int = 100
):
    """Получить все события из базы данных"""
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        query = """
            SELECT * FROM public.clipboard_events 
            ORDER BY created_at DESC 
            LIMIT %s OFFSET %s
        """
        
        cur.execute(query, [limit, skip])
        events = cur.fetchall()
        
        cur.close()
        conn.close()
        
        return events
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/clients/", response_model=List[ClientInfo])
async def get_clients():
    """Получить список всех клиентов с их статистикой"""
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        query = """
            SELECT 
                username,
                hostname,
                COUNT(*) as total_events,
                MAX(created_at) as last_activity
            FROM public.clipboard_events
            GROUP BY username, hostname
            ORDER BY last_activity DESC
        """
        
        cur.execute(query)
        clients = cur.fetchall()
        
        cur.close()
        conn.close()
        
        return clients
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/clients/{username}/{hostname}/", response_model=List[ClipboardEvent])
async def get_client_events(
    username: str,
    hostname: str,
    skip: int = 0,
    limit: int = 100
):
    """Получить все события конкретного клиента"""
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        query = """
            SELECT * FROM public.clipboard_events 
            WHERE username = %s AND hostname = %s
            ORDER BY created_at DESC 
            LIMIT %s OFFSET %s
        """
        
        cur.execute(query, [username, hostname, limit, skip])
        events = cur.fetchall()
        
        cur.close()
        conn.close()
        
        return events
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000) 