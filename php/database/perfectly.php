<?php
// perfectly.php - Система управления персональными данными

// Настройки
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Конфигурация базы данных
define('DB_FILE', 'people_database.db');

// Инициализация базы данных
function initDatabase() {
    try {
        $db = new SQLite3(DB_FILE);
        
        // Создаем таблицу, если её нет
        $db->exec("
            CREATE TABLE IF NOT EXISTS people (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                first_name TEXT NOT NULL,
                last_name TEXT NOT NULL,
                birth_date DATE,
                email TEXT,
                phone TEXT,
                address TEXT,
                notes TEXT,
                photo_url TEXT,
                is_active INTEGER DEFAULT 1
            )
        ");
        
        // Создаем индекс для поиска
        $db->exec("CREATE INDEX IF NOT EXISTS idx_name ON people (last_name, first_name)");
        
        return $db;
    } catch (Exception $e) {
        die("Ошибка базы данных: " . $e->getMessage());
    }
}

// Функция для добавления человека
function addPerson($data, $db) {
    $stmt = $db->prepare("
        INSERT INTO people (first_name, last_name, birth_date, email, phone, address, notes, photo_url)
        VALUES (:first_name, :last_name, :birth_date, :email, :phone, :address, :notes, :photo_url)
    ");
    
    $stmt->bindValue(':first_name', trim($data['first_name']), SQLITE3_TEXT);
    $stmt->bindValue(':last_name', trim($data['last_name']), SQLITE3_TEXT);
    $stmt->bindValue(':birth_date', !empty($data['birth_date']) ? $data['birth_date'] : null, SQLITE3_TEXT);
    $stmt->bindValue(':email', !empty($data['email']) ? trim($data['email']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':phone', !empty($data['phone']) ? trim($data['phone']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':address', !empty($data['address']) ? trim($data['address']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':notes', !empty($data['notes']) ? trim($data['notes']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':photo_url', !empty($data['photo_url']) ? trim($data['photo_url']) : null, SQLITE3_TEXT);
    
    return $stmt->execute();
}

// Функция для получения всех людей
function getAllPeople($db, $active_only = true) {
    $where = $active_only ? "WHERE is_active = 1" : "";
    $result = $db->query("SELECT * FROM people $where ORDER BY last_name, first_name");
    
    $people = [];
    while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
        $people[] = $row;
    }
    
    return $people;
}

// Функция для получения человека по ID
function getPerson($id, $db) {
    $stmt = $db->prepare("SELECT * FROM people WHERE id = :id");
    $stmt->bindValue(':id', $id, SQLITE3_INTEGER);
    $result = $stmt->execute();
    
    return $result->fetchArray(SQLITE3_ASSOC);
}

// Функция для обновления данных
function updatePerson($id, $data, $db) {
    $stmt = $db->prepare("
        UPDATE people 
        SET first_name = :first_name,
            last_name = :last_name,
            birth_date = :birth_date,
            email = :email,
            phone = :phone,
            address = :address,
            notes = :notes,
            photo_url = :photo_url,
            is_active = :is_active
        WHERE id = :id
    ");
    
    $stmt->bindValue(':id', $id, SQLITE3_INTEGER);
    $stmt->bindValue(':first_name', trim($data['first_name']), SQLITE3_TEXT);
    $stmt->bindValue(':last_name', trim($data['last_name']), SQLITE3_TEXT);
    $stmt->bindValue(':birth_date', !empty($data['birth_date']) ? $data['birth_date'] : null, SQLITE3_TEXT);
    $stmt->bindValue(':email', !empty($data['email']) ? trim($data['email']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':phone', !empty($data['phone']) ? trim($data['phone']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':address', !empty($data['address']) ? trim($data['address']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':notes', !empty($data['notes']) ? trim($data['notes']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':photo_url', !empty($data['photo_url']) ? trim($data['photo_url']) : null, SQLITE3_TEXT);
    $stmt->bindValue(':is_active', isset($data['is_active']) ? 1 : 0, SQLITE3_INTEGER);
    
    return $stmt->execute();
}

// Функция для удаления (деактивации) человека
function deletePerson($id, $db) {
    $stmt = $db->prepare("UPDATE people SET is_active = 0 WHERE id = :id");
    $stmt->bindValue(':id', $id, SQLITE3_INTEGER);
    return $stmt->execute();
}

// Функция для поиска людей
function searchPeople($search, $db) {
    $stmt = $db->prepare("
        SELECT * FROM people 
        WHERE is_active = 1 
        AND (first_name LIKE :search 
             OR last_name LIKE :search 
             OR email LIKE :search 
             OR phone LIKE :search)
        ORDER BY last_name, first_name
    ");
    
    $stmt->bindValue(':search', "%$search%", SQLITE3_TEXT);
    $result = $stmt->execute();
    
    $people = [];
    while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
        $people[] = $row;
    }
    
    return $people;
}

// Функция для получения статистики
function getStats($db) {
    $stats = [];
    
    // Общее количество
    $result = $db->query("SELECT COUNT(*) as total FROM people WHERE is_active = 1");
    $stats['total'] = $result->fetchArray(SQLITE3_ASSOC)['total'];
    
    // По возрасту
    $result = $db->query("
        SELECT 
            SUM(CASE WHEN birth_date IS NOT NULL AND date('now') - birth_date < 18 THEN 1 ELSE 0 END) as under_18,
            SUM(CASE WHEN birth_date IS NOT NULL AND date('now') - birth_date BETWEEN 18 AND 30 THEN 1 ELSE 0 END) as age_18_30,
            SUM(CASE WHEN birth_date IS NOT NULL AND date('now') - birth_date BETWEEN 31 AND 60 THEN 1 ELSE 0 END) as age_31_60,
            SUM(CASE WHEN birth_date IS NOT NULL AND date('now') - birth_date > 60 THEN 1 ELSE 0 END) as over_60
        FROM people WHERE is_active = 1
    ");
    $stats['age_groups'] = $result->fetchArray(SQLITE3_ASSOC);
    
    return $stats;
}

// Обработка POST запросов
$db = initDatabase();
$message = '';
$action = $_POST['action'] ?? '';
$search_query = $_GET['search'] ?? '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    switch ($action) {
        case 'add':
            if (!empty($_POST['first_name']) && !empty($_POST['last_name'])) {
                if (addPerson($_POST, $db)) {
                    $message = '<div class="alert success">Человек успешно добавлен!</div>';
                } else {
                    $message = '<div class="alert error">Ошибка при добавлении!</div>';
                }
            }
            break;
            
        case 'update':
            if (!empty($_POST['id']) && !empty($_POST['first_name']) && !empty($_POST['last_name'])) {
                if (updatePerson($_POST['id'], $_POST, $db)) {
                    $message = '<div class="alert success">Данные успешно обновлены!</div>';
                } else {
                    $message = '<div class="alert error">Ошибка при обновлении!</div>';
                }
            }
            break;
            
        case 'delete':
            if (!empty($_POST['id'])) {
                if (deletePerson($_POST['id'], $db)) {
                    $message = '<div class="alert success">Человек удален!</div>';
                } else {
                    $message = '<div class="alert error">Ошибка при удалении!</div>';
                }
            }
            break;
    }
}

// Получаем данные для отображения
if (!empty($search_query)) {
    $people = searchPeople($search_query, $db);
} else {
    $people = getAllPeople($db);
}

$stats = getStats($db);
?>
<!DOCTYPE html>
<html lang="ru" data-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>База данных людей | Perfectly</title>
    <style>
        :root {
            --bg-primary: #1a1a2e;
            --bg-secondary: #16213e;
            --bg-tertiary: #0f3460;
            --text-primary: #e6e6e6;
            --text-secondary: #a0a0a0;
            --accent-primary: #7c3aed;
            --accent-secondary: #4f46e5;
            --border-color: #2d3748;
            --card-bg: #1e293b;
            --success-bg: #064e3b;
            --error-bg: #7f1d1d;
            --shadow-color: rgba(0, 0, 0, 0.3);
        }

        [data-theme="light"] {
            --bg-primary: #ffffff;
            --bg-secondary: #f8fafc;
            --bg-tertiary: #e2e8f0;
            --text-primary: #1e293b;
            --text-secondary: #64748b;
            --accent-primary: #7c3aed;
            --accent-secondary: #4f46e5;
            --border-color: #cbd5e1;
            --card-bg: #ffffff;
            --success-bg: #d1fae5;
            --error-bg: #fee2e2;
            --shadow-color: rgba(0, 0, 0, 0.1);
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            transition: background-color 0.3s, border-color 0.3s, color 0.3s;
        }
        
        body {
            background: linear-gradient(135deg, var(--bg-secondary) 0%, var(--bg-primary) 100%);
            color: var(--text-primary);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            background: var(--bg-primary);
            border-radius: 20px;
            box-shadow: 0 20px 60px var(--shadow-color);
            overflow: hidden;
            border: 1px solid var(--border-color);
        }
        
        .header {
            background: linear-gradient(135deg, var(--accent-secondary) 0%, var(--accent-primary) 100%);
            color: white;
            padding: 40px;
            text-align: center;
            position: relative;
            overflow: hidden;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .header-content {
            position: relative;
            z-index: 1;
            text-align: left;
        }
        
        .header::before {
            content: '';
            position: absolute;
            top: -50%;
            left: -50%;
            width: 200%;
            height: 200%;
            background: radial-gradient(circle, rgba(255,255,255,0.1) 1px, transparent 1px);
            background-size: 50px 50px;
            animation: float 20s linear infinite;
        }
        
        @keyframes float {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        
        h1 {
            font-size: 2.8rem;
            margin-bottom: 10px;
            position: relative;
            z-index: 1;
        }
        
        .subtitle {
            font-size: 1.2rem;
            opacity: 0.9;
            position: relative;
            z-index: 1;
        }
        
        .theme-switcher {
            position: relative;
            z-index: 1;
        }
        
        .theme-toggle {
            background: rgba(255, 255, 255, 0.2);
            border: 2px solid rgba(255, 255, 255, 0.3);
            color: white;
            padding: 12px 24px;
            border-radius: 50px;
            cursor: pointer;
            font-weight: 600;
            display: flex;
            align-items: center;
            gap: 10px;
            transition: all 0.3s;
            backdrop-filter: blur(10px);
            font-size: 1rem;
        }
        
        .theme-toggle:hover {
            background: rgba(255, 255, 255, 0.3);
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2);
        }
        
        .content {
            display: grid;
            grid-template-columns: 350px 1fr;
            min-height: 800px;
        }
        
        .sidebar {
            background: var(--bg-secondary);
            padding: 30px;
            border-right: 1px solid var(--border-color);
        }
        
        .main-content {
            padding: 30px;
            overflow-y: auto;
            max-height: 800px;
            background: var(--bg-primary);
        }
        
        /* Стилизация скроллбара */
        .main-content::-webkit-scrollbar {
            width: 8px;
        }
        
        .main-content::-webkit-scrollbar-track {
            background: var(--bg-secondary);
            border-radius: 4px;
        }
        
        .main-content::-webkit-scrollbar-thumb {
            background: var(--accent-primary);
            border-radius: 4px;
        }
        
        .main-content::-webkit-scrollbar-thumb:hover {
            background: var(--accent-secondary);
        }
        
        .form-group {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: var(--text-primary);
        }
        
        input, textarea {
            width: 100%;
            padding: 12px 15px;
            background: var(--bg-primary);
            color: var(--text-primary);
            border: 2px solid var(--border-color);
            border-radius: 10px;
            font-size: 16px;
            transition: all 0.3s;
        }
        
        input:focus, textarea:focus {
            outline: none;
            border-color: var(--accent-primary);
            box-shadow: 0 0 0 3px rgba(124, 58, 237, 0.2);
        }
        
        .btn {
            background: linear-gradient(135deg, var(--accent-secondary) 0%, var(--accent-primary) 100%);
            color: white;
            border: none;
            padding: 14px 25px;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            width: 100%;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 25px rgba(124, 58, 237, 0.3);
        }
        
        .btn-secondary {
            background: #64748b;
        }
        
        .alert {
            padding: 15px 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            animation: slideIn 0.5s ease-out;
        }
        
        @keyframes slideIn {
            from { transform: translateY(-20px); opacity: 0; }
            to { transform: translateY(0); opacity: 1; }
        }
        
        .alert.success {
            background: var(--success-bg);
            color: #d1fae5;
            border: 1px solid #065f46;
        }
        
        .alert.error {
            background: var(--error-bg);
            color: #fecaca;
            border: 1px solid #991b1b;
        }
        
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
            margin-bottom: 30px;
        }
        
        .stat-card {
            background: var(--card-bg);
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 5px 15px var(--shadow-color);
            text-align: center;
            border: 1px solid var(--border-color);
        }
        
        .stat-number {
            font-size: 2.5rem;
            font-weight: bold;
            color: var(--accent-primary);
            margin-bottom: 5px;
        }
        
        .stat-label {
            color: var(--text-secondary);
            font-size: 0.9rem;
        }
        
        .people-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 20px;
            margin-top: 30px;
        }
        
        .person-card {
            background: var(--card-bg);
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 10px 30px var(--shadow-color);
            border: 1px solid var(--border-color);
            transition: all 0.3s;
            position: relative;
            overflow: hidden;
        }
        
        .person-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.2);
            border-color: var(--accent-primary);
        }
        
        .person-card::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 4px;
            background: linear-gradient(135deg, var(--accent-secondary) 0%, var(--accent-primary) 100%);
        }
        
        .person-name {
            font-size: 1.4rem;
            font-weight: bold;
            color: var(--text-primary);
            margin-bottom: 10px;
        }
        
        .person-details {
            color: var(--text-secondary);
            margin-bottom: 15px;
            font-size: 0.95rem;
        }
        
        .person-details div {
            margin-bottom: 5px;
            display: flex;
            align-items: center;
        }
        
        .person-details i {
            width: 20px;
            margin-right: 10px;
            color: var(--text-secondary);
        }
        
        .person-actions {
            display: flex;
            gap: 10px;
            margin-top: 20px;
        }
        
        .action-btn {
            padding: 10px 15px;
            border-radius: 8px;
            border: none;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.2s;
            flex: 1;
            font-weight: 600;
        }
        
        .edit-btn {
            background: rgba(59, 130, 246, 0.2);
            color: #60a5fa;
            border: 1px solid rgba(59, 130, 246, 0.3);
        }
        
        .edit-btn:hover {
            background: rgba(59, 130, 246, 0.3);
            transform: translateY(-2px);
        }
        
        .delete-btn {
            background: rgba(239, 68, 68, 0.2);
            color: #f87171;
            border: 1px solid rgba(239, 68, 68, 0.3);
        }
        
        .delete-btn:hover {
            background: rgba(239, 68, 68, 0.3);
            transform: translateY(-2px);
        }
        
        .search-box {
            position: relative;
            margin-bottom: 30px;
        }
        
        .search-input {
            padding-left: 45px;
            background: var(--bg-primary) url('data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="%237c3aed" stroke-width="2"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>') no-repeat 15px center;
        }
        
        .empty-state {
            text-align: center;
            padding: 60px 20px;
            color: var(--text-secondary);
        }
        
        .empty-state i {
            font-size: 4rem;
            margin-bottom: 20px;
            color: var(--border-color);
        }
        
        .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.5);
            z-index: 1000;
            align-items: center;
            justify-content: center;
            backdrop-filter: blur(5px);
        }
        
        .modal-content {
            background: var(--bg-primary);
            border-radius: 20px;
            padding: 40px;
            max-width: 500px;
            width: 90%;
            max-height: 90vh;
            overflow-y: auto;
            border: 1px solid var(--border-color);
            box-shadow: 0 25px 50px rgba(0, 0, 0, 0.3);
        }
        
        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 30px;
        }
        
        .close-btn {
            background: none;
            border: none;
            font-size: 24px;
            cursor: pointer;
            color: var(--text-secondary);
            transition: color 0.3s;
        }
        
        .close-btn:hover {
            color: var(--accent-primary);
        }
        
        .tabs {
            display: flex;
            margin-bottom: 30px;
            border-bottom: 2px solid var(--border-color);
        }
        
        .tab {
            padding: 15px 30px;
            background: none;
            border: none;
            font-size: 16px;
            color: var(--text-secondary);
            cursor: pointer;
            position: relative;
            transition: color 0.3s;
        }
        
        .tab:hover {
            color: var(--accent-primary);
        }
        
        .tab.active {
            color: var(--accent-primary);
            font-weight: 600;
        }
        
        .tab.active::after {
            content: '';
            position: absolute;
            bottom: -2px;
            left: 0;
            right: 0;
            height: 3px;
            background: var(--accent-primary);
            border-radius: 3px 3px 0 0;
        }
        
        @media (max-width: 1024px) {
            .content {
                grid-template-columns: 1fr;
            }
            
            .sidebar {
                border-right: none;
                border-bottom: 1px solid var(--border-color);
            }
            
            .header {
                flex-direction: column;
                gap: 20px;
                text-align: center;
            }
            
            .header-content {
                text-align: center;
            }
        }
        
        @media (max-width: 768px) {
            .people-grid {
                grid-template-columns: 1fr;
            }
            
            .stats-grid {
                grid-template-columns: 1fr;
            }
            
            .header {
                padding: 30px 20px;
            }
            
            h1 {
                font-size: 2rem;
            }
            
            .theme-toggle {
                padding: 10px 20px;
                font-size: 0.9rem;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="header-content">
                <h1>📊 База данных людей</h1>
                <div class="subtitle">Perfectly - система управления персональными данными</div>
            </div>
            <div class="theme-switcher">
                <button class="theme-toggle" onclick="toggleTheme()">
                    <span id="theme-icon">☀️</span>
                    <span id="theme-text">Светлая</span>
                </button>
            </div>
        </div>
        
        <div class="content">
            <div class="sidebar">
                <div class="tabs">
                    <button class="tab active" onclick="showTab('add')">Добавить</button>
                    <button class="tab" onclick="showTab('stats')">Статистика</button>
                </div>
                
                <div id="addTab">
                    <?php echo $message; ?>
                    <form method="POST" id="personForm">
                        <input type="hidden" name="action" value="add">
                        <input type="hidden" name="id" id="editId">
                        
                        <div class="form-group">
                            <label for="first_name">Имя *</label>
                            <input type="text" id="first_name" name="first_name" required>
                        </div>
                        
                        <div class="form-group">
                            <label for="last_name">Фамилия *</label>
                            <input type="text" id="last_name" name="last_name" required>
                        </div>
                        
                        <div class="form-group">
                            <label for="birth_date">Дата рождения</label>
                            <input type="date" id="birth_date" name="birth_date">
                        </div>
                        
                        <div class="form-group">
                            <label for="email">Email</label>
                            <input type="email" id="email" name="email">
                        </div>
                        
                        <div class="form-group">
                            <label for="phone">Телефон</label>
                            <input type="tel" id="phone" name="phone">
                        </div>
                        
                        <div class="form-group">
                            <label for="address">Адрес</label>
                            <textarea id="address" name="address" rows="2"></textarea>
                        </div>
                        
                        <div class="form-group">
                            <label for="notes">Заметки</label>
                            <textarea id="notes" name="notes" rows="3"></textarea>
                        </div>
                        
                        <div class="form-group">
                            <label for="photo_url">Ссылка на фото</label>
                            <input type="url" id="photo_url" name="photo_url" placeholder="https://...">
                        </div>
                        
                        <div class="form-group">
                            <label>
                                <input type="checkbox" name="is_active" checked> Активен
                            </label>
                        </div>
                        
                        <button type="submit" class="btn">Сохранить</button>
                        <button type="button" class="btn btn-secondary" onclick="resetForm()" style="margin-top: 10px;">Очистить форму</button>
                    </form>
                </div>
                
                <div id="statsTab" style="display: none;">
                    <div class="stats-grid">
                        <div class="stat-card">
                            <div class="stat-number"><?php echo $stats['total']; ?></div>
                            <div class="stat-label">Всего людей</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-number"><?php echo $stats['age_groups']['under_18']; ?></div>
                            <div class="stat-label">До 18 лет</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-number"><?php echo $stats['age_groups']['age_18_30']; ?></div>
                            <div class="stat-label">18-30 лет</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-number"><?php echo $stats['age_groups']['age_31_60']; ?></div>
                            <div class="stat-label">31-60 лет</div>
                        </div>
                    </div>
                    
                    <div style="text-align: center; margin-top: 20px;">
                        <button class="btn" onclick="exportData()">📥 Экспорт данных</button>
                    </div>
                </div>
            </div>
            
            <div class="main-content">
                <div class="search-box">
                    <form method="GET" action="">
                        <input type="text" 
                               name="search" 
                               class="search-input" 
                               placeholder="Поиск по имени, фамилии, email или телефону..."
                               value="<?php echo htmlspecialchars($search_query); ?>">
                    </form>
                </div>
                
                <?php if (empty($people)): ?>
                    <div class="empty-state">
                        <div>👤</div>
                        <h3>Люди не найдены</h3>
                        <p><?php echo empty($search_query) ? 'Добавьте первого человека с помощью формы слева' : 'Попробуйте другой поисковый запрос'; ?></p>
                    </div>
                <?php else: ?>
                    <div class="people-grid">
                        <?php foreach ($people as $person): ?>
                            <div class="person-card">
                                <div class="person-name">
                                    <?php echo htmlspecialchars($person['last_name'] . ' ' . $person['first_name']); ?>
                                    <?php if (!$person['is_active']): ?>
                                        <span style="color: #f87171; font-size: 0.8rem;">(неактивен)</span>
                                    <?php endif; ?>
                                </div>
                                
                                <div class="person-details">
                                    <?php if (!empty($person['birth_date'])): ?>
                                        <div>
                                            <span>🎂</span>
                                            <?php 
                                                $birth_date = new DateTime($person['birth_date']);
                                                $today = new DateTime();
                                                $age = $today->diff($birth_date)->y;
                                                echo htmlspecialchars($person['birth_date']) . " ($age лет)";
                                            ?>
                                        </div>
                                    <?php endif; ?>
                                    
                                    <?php if (!empty($person['email'])): ?>
                                        <div>
                                            <span>📧</span>
                                            <?php echo htmlspecialchars($person['email']); ?>
                                        </div>
                                    <?php endif; ?>
                                    
                                    <?php if (!empty($person['phone'])): ?>
                                        <div>
                                            <span>📱</span>
                                            <?php echo htmlspecialchars($person['phone']); ?>
                                        </div>
                                    <?php endif; ?>
                                    
                                    <?php if (!empty($person['address'])): ?>
                                        <div>
                                            <span>🏠</span>
                                            <?php echo htmlspecialchars(mb_strimwidth($person['address'], 0, 50, '...')); ?>
                                        </div>
                                    <?php endif; ?>
                                    
                                    <?php if (!empty($person['notes'])): ?>
                                        <div>
                                            <span>📝</span>
                                            <?php echo htmlspecialchars(mb_strimwidth($person['notes'], 0, 100, '...')); ?>
                                        </div>
                                    <?php endif; ?>
                                    
                                    <div style="margin-top: 10px; color: var(--text-secondary); font-size: 0.8rem;">
                                        ID: <?php echo $person['id']; ?> | 
                                        Добавлен: <?php echo date('d.m.Y', strtotime($person['created_at'])); ?>
                                    </div>
                                </div>
                                
                                <div class="person-actions">
                                    <button class="action-btn edit-btn" onclick="editPerson(<?php echo $person['id']; ?>)">
                                        ✏️ Редактировать
                                    </button>
                                    <button class="action-btn delete-btn" onclick="deleteConfirm(<?php echo $person['id']; ?>, '<?php echo htmlspecialchars($person['last_name'] . ' ' . $person['first_name']); ?>')">
                                        🗑️ Удалить
                                    </button>
                                </div>
                            </div>
                        <?php endforeach; ?>
                    </div>
                <?php endif; ?>
            </div>
        </div>
    </div>
    
    <!-- Модальное окно подтверждения удаления -->
    <div id="deleteModal" class="modal">
        <div class="modal-content">
            <div class="modal-header">
                <h2>Подтверждение удаления</h2>
                <button class="close-btn" onclick="closeModal()">×</button>
            </div>
            <p id="deleteMessage">Вы уверены, что хотите удалить этого человека?</p>
            <form method="POST" id="deleteForm">
                <input type="hidden" name="action" value="delete">
                <input type="hidden" name="id" id="deleteId">
                <div style="display: flex; gap: 10px; margin-top: 30px;">
                    <button type="button" class="btn btn-secondary" onclick="closeModal()" style="flex: 1;">Отмена</button>
                    <button type="submit" class="btn" style="background: linear-gradient(135deg, #dc2626 0%, #991b1b 100%); flex: 1;">Удалить</button>
                </div>
            </form>
        </div>
    </div>

    <script>
        // Управление темой
        function toggleTheme() {
            const html = document.documentElement;
            const currentTheme = html.getAttribute('data-theme');
            const newTheme = currentTheme === 'light' ? 'dark' : 'light';
            
            html.setAttribute('data-theme', newTheme);
            localStorage.setItem('theme', newTheme);
            
            updateThemeButton(newTheme);
        }
        
        function updateThemeButton(theme) {
            const icon = document.getElementById('theme-icon');
            const text = document.getElementById('theme-text');
            
            if (theme === 'light') {
                icon.textContent = '🌙';
                text.textContent = 'Темная';
            } else {
                icon.textContent = '☀️';
                text.textContent = 'Светлая';
            }
        }
        
        // Загружаем сохраненную тему
        (function() {
            const savedTheme = localStorage.getItem('theme') || 'dark';
            document.documentElement.setAttribute('data-theme', savedTheme);
            updateThemeButton(savedTheme);
        })();
        
        // Функции для работы с вкладками
        function showTab(tabName) {
            document.querySelectorAll('.tab').forEach(tab => {
                tab.classList.remove('active');
            });
            document.querySelectorAll('.tab').forEach(tab => {
                if (tab.textContent.includes(tabName === 'add' ? 'Добавить' : 'Статистика')) {
                    tab.classList.add('active');
                }
            });
            
            document.getElementById('addTab').style.display = tabName === 'add' ? 'block' : 'none';
            document.getElementById('statsTab').style.display = tabName === 'stats' ? 'block' : 'none';
            
            if (tabName === 'add') {
                resetForm();
            }
        }
        
        // Редактирование человека
        function editPerson(id) {
            fetch(`?ajax=get_person&id=${id}`)
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        const person = data.person;
                        document.getElementById('editId').value = person.id;
                        document.getElementById('first_name').value = person.first_name;
                        document.getElementById('last_name').value = person.last_name;
                        document.getElementById('birth_date').value = person.birth_date || '';
                        document.getElementById('email').value = person.email || '';
                        document.getElementById('phone').value = person.phone || '';
                        document.getElementById('address').value = person.address || '';
                        document.getElementById('notes').value = person.notes || '';
                        document.getElementById('photo_url').value = person.photo_url || '';
                        document.querySelector('[name="is_active"]').checked = person.is_active == 1;
                        document.querySelector('[name="action"]').value = 'update';
                        document.querySelector('.btn').textContent = 'Обновить';
                        
                        showTab('add');
                        window.scrollTo({top: 0, behavior: 'smooth'});
                    }
                })
                .catch(error => console.error('Error:', error));
        }
        
        // Подтверждение удаления
        function deleteConfirm(id, name) {
            document.getElementById('deleteId').value = id;
            document.getElementById('deleteMessage').textContent = `Вы уверены, что хотите удалить "${name}"?`;
            document.getElementById('deleteModal').style.display = 'flex';
        }
        
        // Закрытие модального окна
        function closeModal() {
            document.getElementById('deleteModal').style.display = 'none';
        }
        
        // Сброс формы
        function resetForm() {
            document.getElementById('personForm').reset();
            document.getElementById('editId').value = '';
            document.querySelector('[name="action"]').value = 'add';
            document.querySelector('.btn').textContent = 'Сохранить';
        }
        
        // Экспорт данных
        function exportData() {
            window.open('?export=csv', '_blank');
        }
        
        // Автозакрытие сообщений
        setTimeout(() => {
            const alerts = document.querySelectorAll('.alert');
            alerts.forEach(alert => {
                alert.style.opacity = '0';
                alert.style.transition = 'opacity 0.5s';
                setTimeout(() => alert.remove(), 500);
            });
        }, 5000);
        
        // AJAX для получения данных человека
        if (window.location.search.includes('ajax=get_person')) {
            <?php
            if (isset($_GET['ajax']) && $_GET['ajax'] === 'get_person' && isset($_GET['id'])) {
                $person = getPerson($_GET['id'], $db);
                if ($person) {
                    echo 'echo json_encode(["success" => true, "person" => $person]);';
                } else {
                    echo 'echo json_encode(["success" => false, "error" => "Person not found"]);';
                }
                exit;
            }
            
            // Экспорт в CSV
            if (isset($_GET['export']) && $_GET['export'] === 'csv') {
                header('Content-Type: text/csv; charset=utf-8');
                header('Content-Disposition: attachment; filename=people_' . date('Y-m-d') . '.csv');
                
                $output = fopen('php://output', 'w');
                fputcsv($output, ['ID', 'Фамилия', 'Имя', 'Дата рождения', 'Email', 'Телефон', 'Адрес', 'Заметки', 'Дата добавления']);
                
                $people = getAllPeople($db, false);
                foreach ($people as $person) {
                    fputcsv($output, [
                        $person['id'],
                        $person['last_name'],
                        $person['first_name'],
                        $person['birth_date'],
                        $person['email'],
                        $person['phone'],
                        $person['address'],
                        $person['notes'],
                        $person['created_at']
                    ]);
                }
                fclose($output);
                exit;
            }
            ?>
        }
        
        // Закрытие модального окна по клику вне его
        window.onclick = function(event) {
            const modal = document.getElementById('deleteModal');
            if (event.target === modal) {
                closeModal();
            }
        }
        
        // Закрытие по ESC
        document.addEventListener('keydown', function(event) {
            if (event.key === 'Escape') {
                closeModal();
            }
        });
    </script>
</body>
</html>
