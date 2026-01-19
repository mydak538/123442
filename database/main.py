import time
import os
import sqlite3
from datetime import datetime

# ANSI color codes
COLORS = {
    'INFO': '\033[94m',
    'SUCCESS': '\033[92m',
    'WARN': '\033[93m',
    'ERROR': '\033[91m',
    'RESET': '\033[0m'
}

def log_message(level, message):
    color = COLORS.get(level, COLORS['RESET'])
    timestamp = time.strftime('%H:%M:%S')
    print(f"{color}[{timestamp}] [{level}]{COLORS['RESET']} {message}")

def init_database(db_path="database.db"):
    """Initialize SQLite database with proper schema"""
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Create table if not exists
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS lines (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                content TEXT NOT NULL,
                line_number INTEGER NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                source_file TEXT
            )
        ''')
        
        # Create index for faster queries
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_timestamp 
            ON lines(timestamp)
        ''')
        
        conn.commit()
        conn.close()
        log_message("INFO", f"Database initialized: {db_path}")
        
    except Exception as e:
        log_message("ERROR", f"Failed to initialize database: {e}")
        raise

def process_lines(lines, db_path="database.db", source_file="txt/nice.txt"):
    """Insert lines into SQLite database"""
    if not lines:
        return 0
    
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Get current max line number for this file
        cursor.execute('''
            SELECT MAX(line_number) 
            FROM lines 
            WHERE source_file = ?
        ''', (source_file,))
        
        result = cursor.fetchone()
        start_line = (result[0] or 0) + 1
        
        # Prepare data for batch insert
        data_to_insert = []
        for i, line in enumerate(lines):
            line = line.strip()
            if line:  # Skip empty lines
                data_to_insert.append((
                    line,
                    start_line + i,
                    source_file
                ))
        
        if not data_to_insert:
            return 0
        
        # Batch insert
        cursor.executemany('''
            INSERT INTO lines (content, line_number, source_file)
            VALUES (?, ?, ?)
        ''', data_to_insert)
        
        conn.commit()
        conn.close()
        
        log_message("SUCCESS", f"Inserted {len(data_to_insert)} lines into database")
        return len(data_to_insert)
        
    except Exception as e:
        log_message("ERROR", f"Database insert failed: {e}")
        return 0

def monitor_file(txt_path="txt/nice.txt", db_path="database.db", check_interval=5):
    """Monitor text file and insert new lines into SQLite database"""
    
    # Ensure txt directory exists
    os.makedirs(os.path.dirname(txt_path), exist_ok=True)
    
    # Initialize database
    init_database(db_path)
    
    # Wait for source file if it doesn't exist
    if not os.path.exists(txt_path):
        log_message("WARN", f"Source file not found: {txt_path}")
        while not os.path.exists(txt_path):
            time.sleep(check_interval)
    
    log_message("INFO", f"Monitoring started: {txt_path}")
    
    last_position = 0
    total_processed = 0
    
    try:
        while True:
            try:
                # Read new lines from file
                with open(txt_path, 'r', encoding='utf-8') as txt_file:
                    txt_file.seek(last_position)
                    new_lines = txt_file.readlines()
                    
                    if new_lines:
                        # Process and insert into database
                        inserted = process_lines(
                            new_lines, 
                            db_path, 
                            os.path.basename(txt_path)
                        )
                        total_processed += inserted
                        last_position = txt_file.tell()
                    
            except UnicodeDecodeError:
                log_message("WARN", "UTF-8 decode failed, trying latin-1")
                try:
                    with open(txt_path, 'r', encoding='latin-1') as txt_file:
                        txt_file.seek(last_position)
                        new_lines = txt_file.readlines()
                        
                        if new_lines:
                            inserted = process_lines(
                                new_lines, 
                                db_path, 
                                os.path.basename(txt_path)
                            )
                            total_processed += inserted
                            last_position = txt_file.tell()
                except Exception as e:
                    log_message("ERROR", f"Failed to read file: {e}")
            
            except Exception as e:
                log_message("ERROR", f"File operation error: {e}")
            
            time.sleep(check_interval)
            
    except KeyboardInterrupt:
        log_message("INFO", f"Monitoring stopped. Total lines processed: {total_processed}")
    except Exception as e:
        log_message("ERROR", f"Critical error: {e}")

if __name__ == "__main__":
    monitor_file()
