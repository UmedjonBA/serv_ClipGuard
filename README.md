# Clipboard Monitor API

API сервер для мониторинга буфера обмена. Позволяет получать данные о событиях буфера обмена с разных клиентских устройств.

## Установка

### 1. Установка Python

1. Скачайте Python 3.12.2 с официального сайта:
   - Перейдите на https://www.python.org/downloads/release/python-3122/
   - Скачайте "Windows installer (64-bit)"
   - **ВАЖНО**: При установке поставьте галочку "Add Python 3.12 to PATH"

2. Проверьте установку. Откройте командную строку (cmd) и введите:
```bash
python --version
```
Должно показать "Python 3.12.2"

### 2. Установка зависимостей

Откройте командную строку (cmd) и выполните:
```bash
pip install fastapi uvicorn psycopg2-binary
```

### 3. Настройка базы данных

1. Установите PostgreSQL, если еще не установлен
2. Создайте базу данных:
```sql
CREATE DATABASE clipboard_monitor;
```
3. Создайте таблицу:
```sql
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
```

4. Отредактируйте настройки подключения к базе данных в файле `api_server.py`:
```python
DB_CONFIG = {
    "dbname": "clipboard_monitor",
    "user": "postgres",
    "password": "ваш_пароль",  # Замените на ваш пароль
    "host": "localhost",
    "port": "5432"
}
```

## Запуск API

1. Откройте командную строку (cmd)
2. Перейдите в папку с проектом
3. Запустите сервер:
```bash
python api_server.py
```

API будет доступен по адресу: http://localhost:8000

## Использование API

### Документация
- Swagger UI: http://localhost:8000/docs
- ReDoc: http://localhost:8000/redoc

### Основные эндпоинты

1. Получить все события:
```
GET /all/
```
Параметры:
- `skip` (опционально): сколько записей пропустить
- `limit` (опционально): сколько записей вернуть (по умолчанию 100)

2. Получить список клиентов:
```
GET /clients/
```
Возвращает для каждого клиента:
- username
- hostname
- total_events (общее количество событий)
- last_activity (время последней активности)

3. Получить события конкретного клиента:
```
GET /clients/{username}/{hostname}/
```
Параметры в URL:
- `username`: имя пользователя
- `hostname`: имя компьютера
Параметры запроса:
- `skip` (опционально): сколько записей пропустить
- `limit` (опционально): сколько записей вернуть

### Примеры запросов

1. Получить все события:
```bash
curl http://localhost:8000/all/
```

2. Получить список клиентов:
```bash
curl http://localhost:8000/clients/
```

3. Получить события конкретного клиента:
```bash
curl http://localhost:8000/clients/username/hostname/
```

## Устранение неполадок

1. Если порт 8000 занят:
   - Измените порт в файле `api_server.py`:
   ```python
   uvicorn.run(app, host="0.0.0.0", port=8001)  # Измените 8000 на другой порт
   ```

2. Если не удается подключиться к базе данных:
   - Проверьте настройки в `DB_CONFIG`
   - Убедитесь, что PostgreSQL запущен
   - Проверьте, что база данных создана
   - Проверьте права доступа пользователя

3. Если API не отвечает:
   - Проверьте, что сервер запущен
   - Проверьте логи на наличие ошибок
   - Убедитесь, что порт не блокируется файрволом 