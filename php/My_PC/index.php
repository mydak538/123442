<?php
// ============================================
// УДАЛЕННОЕ УПРАВЛЕНИЕ ЛИНУКС - ЗИМНИЙ ТЕРМИНАЛ С SUDO
// ============================================

session_start();

// Конфигурация
$config = [
    'username' => 'root',
    'password' => 'yellow_215_999nea!', // Пароль для входа на сайт
    'session_timeout' => 7200, // 2 часа неактивности - разлогин
    'sudo_timeout' => 300,     // 5 минут для sudo пароля
    'start_dir' => '/'
];

// Текущая директория в сессии
if (!isset($_SESSION['current_dir'])) {
    $_SESSION['current_dir'] = $config['start_dir'];
}

// Проверка авторизации
function isAuthenticated() {
    if (isset($_SESSION['authenticated']) && $_SESSION['authenticated'] === true) {
        $_SESSION['last_activity'] = time();
        return true;
    }
    return false;
}

// Проверка таймаута сессии
if (isset($_SESSION['last_activity'])) {
    $inactive = time() - $_SESSION['last_activity'];
    if ($inactive > $config['session_timeout']) {
        session_unset();
        session_destroy();
        header("Location: ?");
        exit;
    }
}

// Обработка входа
if (isset($_POST['login'])) {
    $username = $_POST['username'] ?? '';
    $password = $_POST['password'] ?? '';
    
    if ($username === $config['username'] && $password === $config['password']) {
        $_SESSION['authenticated'] = true;
        $_SESSION['last_activity'] = time();
        $_SESSION['username'] = $username;
        $_SESSION['current_dir'] = $config['start_dir'];
        unset($_SESSION['sudo_password']);
        unset($_SESSION['sudo_time']);
        header("Location: ?");
        exit;
    } else {
        $error = "❄️ Неверный логин или пароль!";
    }
}

// Выход
if (isset($_GET['logout'])) {
    session_unset();
    session_destroy();
    header("Location: ?");
    exit;
}

// Обработка смены директории
if (isAuthenticated() && isset($_POST['change_dir'])) {
    $new_dir = trim($_POST['new_dir']);
    if (!empty($new_dir)) {
        $full_path = realpath($_SESSION['current_dir'] . '/' . $new_dir);
        
        if ($full_path && is_dir($full_path)) {
            $_SESSION['current_dir'] = $full_path;
            header("Location: ?");
            exit;
        } else {
            $dir_error = "❄️ Директория не существует: " . htmlspecialchars($new_dir);
        }
    }
}

// Обработка sudo пароля
if (isAuthenticated() && isset($_POST['sudo_password_submit'])) {
    $sudo_password = $_POST['sudo_password'] ?? '';
    if (!empty($sudo_password)) {
        $_SESSION['sudo_password'] = $sudo_password;
        $_SESSION['sudo_time'] = time();
        if (isset($_SESSION['pending_sudo_command'])) {
            $pending_command = $_SESSION['pending_sudo_command'];
            unset($_SESSION['pending_sudo_command']);
            header("Location: ?command=" . urlencode($pending_command));
            exit;
        }
    } else {
        $sudo_error = "⚠️ Введите sudo пароль!";
    }
}

// Проверка sudo пароля
function checkSudoPassword() {
    global $config;
    
    if (!isset($_SESSION['sudo_password']) || !isset($_SESSION['sudo_time'])) {
        return false;
    }
    
    $sudo_inactive = time() - $_SESSION['sudo_time'];
    if ($sudo_inactive > $config['sudo_timeout']) {
        unset($_SESSION['sudo_password']);
        unset($_SESSION['sudo_time']);
        return false;
    }
    
    return true;
}

// Безопасное выполнение sudo команд
function executeSudoCommand($cmd, $sudo_password) {
    $clean_cmd = substr($cmd, 5);
    
    $tmp_file = tempnam(sys_get_temp_dir(), 'sudo_');
    $script = "#!/bin/bash\n";
    $script .= "echo '" . addslashes($sudo_password) . "' | sudo -S " . escapeshellcmd($clean_cmd) . " 2>&1\n";
    $script .= "echo -e \"\\n\\033[0;32m✅ Команда выполнена с sudo\\033[0m\"\n";
    
    file_put_contents($tmp_file, $script);
    chmod($tmp_file, 0700);
    
    $output = shell_exec("bash " . escapeshellarg($tmp_file));
    unlink($tmp_file);
    
    return $output;
}

// Выполнение обычных команд
function executeCommand($cmd) {
    $original_dir = getcwd();
    chdir($_SESSION['current_dir']);
    
    $output = '';
    if (function_exists('shell_exec')) {
        $output = @shell_exec($cmd . ' 2>&1');
    }
    
    chdir($original_dir);
    return $output;
}

// Функция для обработки цветов терминала
function processTerminalColors($text) {
    $colors = [
        '/\033\[0;30m(.*?)\033\[0m/s' => '<span class="color-black">$1</span>',
        '/\033\[0;31m(.*?)\033\[0m/s' => '<span class="color-red">$1</span>',
        '/\033\[0;32m(.*?)\033\[0m/s' => '<span class="color-green">$1</span>',
        '/\033\[0;33m(.*?)\033\[0m/s' => '<span class="color-yellow">$1</span>',
        '/\033\[0;34m(.*?)\033\[0m/s' => '<span class="color-blue">$1</span>',
        '/\033\[0;35m(.*?)\033\[0m/s' => '<span class="color-magenta">$1</span>',
        '/\033\[0;36m(.*?)\033\[0m/s' => '<span class="color-cyan">$1</span>',
        '/\033\[0;37m(.*?)\033\[0m/s' => '<span class="color-white">$1</span>',
        '/\033\[1;31m(.*?)\033\[0m/s' => '<span class="color-bright-red">$1</span>',
        '/\033\[1;32m(.*?)\033\[0m/s' => '<span class="color-bright-green">$1</span>',
        '/\033\[1;33m(.*?)\033\[0m/s' => '<span class="color-bright-yellow">$1</span>',
        '/\033\[0m/' => '</span>',
    ];
    
    $text = htmlspecialchars($text);
    foreach ($colors as $pattern => $replacement) {
        $text = preg_replace($pattern, $replacement, $text);
    }
    
    return $text;
}

// Выполнение команд
$output = '';
$current_dir_display = $_SESSION['current_dir'] ?? '/';
$show_sudo_form = false;

if (isAuthenticated() && (isset($_POST['command']) || isset($_GET['command']))) {
    $command = isset($_POST['command']) ? trim($_POST['command']) : trim($_GET['command'] ?? '');
    
    if (!empty($command)) {
        if (strpos($command, 'cd ') === 0) {
            $new_dir = trim(substr($command, 3));
            
            if ($new_dir === '') {
                $_SESSION['current_dir'] = '/';
            } elseif ($new_dir === '~') {
                $_SESSION['current_dir'] = '/home';
            } elseif ($new_dir === '..') {
                $current = $_SESSION['current_dir'];
                $_SESSION['current_dir'] = dirname($current) ?: '/';
            } elseif ($new_dir[0] === '/') {
                if (is_dir($new_dir)) {
                    $_SESSION['current_dir'] = $new_dir;
                } else {
                    $output = "cd: ❄️ директория не существует: " . htmlspecialchars($new_dir);
                }
            } else {
                $new_path = $_SESSION['current_dir'] . '/' . $new_dir;
                if (is_dir($new_path)) {
                    $_SESSION['current_dir'] = realpath($new_path);
                } else {
                    $output = "cd: ❄️ директория не существует: " . htmlspecialchars($new_dir);
                }
            }
            
            $current_dir_display = $_SESSION['current_dir'];
            
        } elseif ($command === 'clear' || $command === 'cls') {
            $_SESSION['last_command'] = null;
            
        } elseif (strpos($command, 'sudo ') === 0) {
            if (!checkSudoPassword()) {
                $_SESSION['pending_sudo_command'] = $command;
                $show_sudo_form = true;
                $output = "\033[0;33m[sudo] пароль для " . $_SESSION['username'] . ": \033[0m";
            } else {
                $output = executeSudoCommand($command, $_SESSION['sudo_password']);
            }
            
        } else {
            $output = executeCommand($command);
        }
        
        if (!empty($output)) {
            $output = processTerminalColors($output);
        }
    }
}
?>
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>❄️ Удаленый сервер на Linux</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Courier New', monospace;
            color: #e0e0ff;
            line-height: 1.4;
            min-height: 100vh;
            background: linear-gradient(to bottom, 
                #0a0a2a 0%, 
                #1a1a4a 20%, 
                #2a2a6a 40%, 
                #3a3a8a 60%, 
                #4a4aaa 80%, 
                #5a5aca 100%
            );
            overflow-x: hidden;
            position: relative;
        }
        
        /* Северное сияние */
        .aurora {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 300px;
            background: linear-gradient(to bottom,
                rgba(102, 255, 102, 0.1) 0%,
                rgba(178, 102, 255, 0.2) 25%,
                rgba(255, 102, 178, 0.3) 50%,
                rgba(102, 178, 255, 0.2) 75%,
                rgba(102, 255, 178, 0.1) 100%
            );
            animation: auroraFlow 15s ease-in-out infinite alternate;
            z-index: -2;
        }
        
        @keyframes auroraFlow {
            0% { transform: translateX(-10%) skewX(-10deg); opacity: 0.7; }
            100% { transform: translateX(10%) skewX(10deg); opacity: 0.9; }
        }
        
        /* Горы */
        .mountains {
            position: fixed;
            bottom: 0;
            left: 0;
            width: 100%;
            height: 200px;
            background: linear-gradient(to top,
                #2a1a5a 0%,
                #3a2a8a 30%,
                #4a3aaa 60%,
                #5a4aca 90%,
                #6a5aea 100%
            );
            clip-path: polygon(
                0% 100%, 
                10% 60%, 20% 80%, 30% 50%, 40% 70%, 
                50% 40%, 60% 60%, 70% 30%, 80% 50%, 
                90% 20%, 100% 100%
            );
            z-index: -1;
        }
        
        /* Снег */
        .snow {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
            z-index: -1;
        }
        
        .snowflake {
            position: absolute;
            background: white;
            border-radius: 50%;
            opacity: 0.8;
            animation: fall linear infinite;
        }
        
        @keyframes fall {
            to { transform: translateY(100vh) rotate(360deg); }
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            position: relative;
            z-index: 1;
        }
        
        /* Форма входа - стеклянный эффект */
        .login-form {
            max-width: 450px;
            margin: 100px auto;
            padding: 40px 30px;
            background: rgba(42, 26, 90, 0.7);
            backdrop-filter: blur(10px);
            border: 2px solid rgba(178, 102, 255, 0.5);
            border-radius: 15px;
            box-shadow: 
                0 0 30px rgba(102, 255, 102, 0.3),
                0 0 60px rgba(178, 102, 255, 0.2),
                inset 0 0 20px rgba(255, 255, 255, 0.1);
        }
        
        .login-form h2 {
            text-align: center;
            margin-bottom: 30px;
            color: #aaffaa;
            font-size: 28px;
            text-shadow: 
                0 0 10px #66ff66,
                0 0 20px #66ff66,
                0 0 30px #66ff66;
            letter-spacing: 2px;
        }
        
        .login-form h2:before {
            content: "❄️ ";
        }
        
        .login-form h2:after {
            content: " ❄️";
        }
        
        .form-group {
            margin-bottom: 25px;
        }
        
        .form-group label {
            display: block;
            margin-bottom: 8px;
            color: #b266ff;
            font-weight: bold;
            font-size: 16px;
            text-shadow: 0 0 5px rgba(178, 102, 255, 0.5);
        }
        
        .form-group input {
            width: 100%;
            padding: 14px;
            background: rgba(26, 16, 58, 0.8);
            border: 2px solid #66ff66;
            color: #e0e0ff;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 16px;
            transition: all 0.3s;
        }
        
        .form-group input:focus {
            outline: none;
            border-color: #b266ff;
            box-shadow: 
                0 0 15px rgba(102, 255, 102, 0.5),
                0 0 30px rgba(178, 102, 255, 0.3);
            background: rgba(32, 20, 70, 0.9);
        }
        
        .btn {
            display: block;
            width: 100%;
            padding: 16px;
            background: linear-gradient(135deg, #66ff66, #b266ff);
            color: #0a0a2a;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-family: 'Courier New', monospace;
            font-size: 18px;
            font-weight: bold;
            text-transform: uppercase;
            letter-spacing: 2px;
            transition: all 0.3s;
            margin-top: 10px;
            text-shadow: 0 0 10px rgba(255, 255, 255, 0.5);
        }
        
        .btn:hover {
            background: linear-gradient(135deg, #b266ff, #66ff66);
            transform: translateY(-2px);
            box-shadow: 
                0 5px 20px rgba(102, 255, 102, 0.4),
                0 10px 30px rgba(178, 102, 255, 0.3);
        }
        
        .error {
            color: #ff66aa;
            text-align: center;
            margin-top: 15px;
            font-size: 16px;
            text-shadow: 0 0 10px rgba(255, 102, 170, 0.5);
            padding: 10px;
            background: rgba(90, 20, 50, 0.3);
            border-radius: 5px;
            border: 1px solid rgba(255, 102, 170, 0.3);
        }
        
        /* Sudo форма */
        .sudo-overlay {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.85);
            backdrop-filter: blur(5px);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 1000;
            animation: fadeIn 0.3s ease;
        }
        
        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }
        
        .sudo-form {
            background: rgba(42, 26, 90, 0.9);
            padding: 35px 30px;
            border: 3px solid #ffaa00;
            border-radius: 15px;
            max-width: 400px;
            width: 90%;
            box-shadow: 
                0 0 40px rgba(255, 170, 0, 0.4),
                0 0 80px rgba(255, 170, 0, 0.2);
            backdrop-filter: blur(10px);
            animation: slideUp 0.3s ease;
        }
        
        @keyframes slideUp {
            from { transform: translateY(30px); opacity: 0; }
            to { transform: translateY(0); opacity: 1; }
        }
        
        .sudo-form h3 {
            color: #ffaa00;
            margin-bottom: 15px;
            text-align: center;
            font-size: 22px;
            text-shadow: 0 0 10px rgba(255, 170, 0, 0.5);
        }
        
        .sudo-form p {
            color: #cccccc;
            margin-bottom: 25px;
            text-align: center;
            font-size: 16px;
        }
        
        /* Терминал */
        .terminal {
            background: rgba(10, 10, 42, 0.85);
            backdrop-filter: blur(5px);
            border: 3px solid;
            border-image: linear-gradient(45deg, #66ff66, #b266ff, #ff66aa) 1;
            border-radius: 12px;
            height: 82vh;
            display: flex;
            flex-direction: column;
            box-shadow: 
                0 0 40px rgba(102, 255, 102, 0.3),
                0 0 80px rgba(178, 102, 255, 0.2),
                inset 0 0 30px rgba(255, 255, 255, 0.05);
        }
        
        .terminal-header {
            background: linear-gradient(90deg, #1a1a4a, #2a2a6a);
            padding: 15px 20px;
            border-bottom: 2px solid #66ff66;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-radius: 10px 10px 0 0;
        }
        
        .terminal-title {
            color: #aaffaa;
            font-weight: bold;
            font-size: 18px;
            text-shadow: 0 0 10px rgba(102, 255, 102, 0.7);
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .terminal-title:before {
            content: "🐧 ";
        }
        
        .logout-btn {
            background: linear-gradient(135deg, #ff66aa, #ff3366);
            color: white;
            border: none;
            padding: 8px 20px;
            border-radius: 6px;
            cursor: pointer;
            text-decoration: none;
            font-family: 'Courier New', monospace;
            font-weight: bold;
            transition: all 0.3s;
        }
        
        .logout-btn:hover {
            background: linear-gradient(135deg, #ff3366, #ff66aa);
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(255, 102, 170, 0.4);
        }
        
        .terminal-body {
            flex: 1;
            overflow-y: auto;
            padding: 20px;
            background: rgba(0, 0, 20, 0.6);
        }
        
        .output {
            margin-bottom: 20px;
            white-space: pre-wrap;
            word-break: break-all;
            color: #c0c0ff;
            font-family: 'Consolas', monospace;
            font-size: 14px;
            line-height: 1.5;
        }
        
        .prompt {
            color: #66ff66;
            margin-bottom: 10px;
            font-family: 'Consolas', monospace;
            font-size: 15px;
        }
        
        .prompt span.user {
            color: #b266ff;
            font-weight: bold;
        }
        
        .prompt span.path {
            color: #ffaa66;
        }
        
        .current-dir {
            color: #ffcc66;
            font-weight: bold;
            margin-bottom: 15px;
            padding: 12px;
            background: rgba(42, 26, 90, 0.5);
            border: 1px solid rgba(178, 102, 255, 0.3);
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 16px;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .current-dir:before {
            content: "📁 ";
            font-size: 20px;
        }
        
        .command-input {
            display: flex;
            background: rgba(26, 16, 58, 0.8);
            border-top: 2px solid #66ff66;
            padding: 15px;
            align-items: center;
            border-radius: 0 0 10px 10px;
        }
        
        .command-input span {
            color: #b266ff;
            padding: 10px 12px 10px 0;
            white-space: nowrap;
            font-family: 'Consolas', monospace;
            font-size: 15px;
            font-weight: bold;
        }
        
        #command {
            flex: 1;
            background: transparent;
            border: none;
            color: #e0e0ff;
            font-family: 'Consolas', monospace;
            font-size: 16px;
            outline: none;
            padding: 10px 0;
        }
        
        /* Цвета терминала */
        .color-black { color: #000000; }
        .color-red { color: #ff5555; font-weight: bold; }
        .color-green { color: #55ff55; font-weight: bold; }
        .color-yellow { color: #ffff55; font-weight: bold; }
        .color-blue { color: #5555ff; font-weight: bold; }
        .color-magenta { color: #ff55ff; font-weight: bold; }
        .color-cyan { color: #55ffff; font-weight: bold; }
        .color-white { color: #ffffff; }
        .color-bright-red { color: #ff8888; font-weight: bold; }
        .color-bright-green { color: #88ff88; font-weight: bold; }
        .color-bright-yellow { color: #ffff88; font-weight: bold; }
        
        /* Информация о сессии */
        .system-info {
            background: linear-gradient(135deg, rgba(42, 26, 90, 0.7), rgba(58, 42, 138, 0.7));
            padding: 20px;
            margin-bottom: 25px;
            border: 2px solid rgba(102, 255, 102, 0.3);
            border-radius: 12px;
            color: #aaffaa;
            backdrop-filter: blur(5px);
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
            gap: 15px;
        }
        
        .info-item {
            padding: 10px;
            background: rgba(26, 16, 58, 0.4);
            border-radius: 8px;
            border: 1px solid rgba(178, 102, 255, 0.2);
        }
        
        .info-label {
            color: #ffcc66;
            font-weight: bold;
            font-size: 14px;
            margin-bottom: 5px;
            text-shadow: 0 0 5px rgba(255, 204, 102, 0.3);
        }
        
        /* Быстрые команды */
        .quick-commands {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(130px, 1fr));
            gap: 12px;
            margin-bottom: 25px;
        }
        
        .quick-btn {
            padding: 12px 8px;
            background: linear-gradient(135deg, #2a2a6a, #3a3a8a);
            color: #aaffaa;
            border: 1px solid #66ff66;
            border-radius: 8px;
            cursor: pointer;
            font-family: 'Courier New', monospace;
            text-align: center;
            transition: all 0.3s;
            font-size: 14px;
            font-weight: bold;
        }
        
        .quick-btn:hover {
            background: linear-gradient(135deg, #3a3a8a, #2a2a6a);
            color: #ffffff;
            border-color: #b266ff;
            transform: translateY(-3px);
            box-shadow: 0 5px 15px rgba(102, 255, 102, 0.3);
        }
        
        /* Sudo статус */
        .sudo-status {
            position: fixed;
            bottom: 20px;
            right: 20px;
            background: rgba(42, 26, 90, 0.9);
            color: #ffcc66;
            padding: 12px 18px;
            border-radius: 10px;
            font-size: 14px;
            border: 2px solid rgba(255, 204, 102, 0.4);
            box-shadow: 0 0 25px rgba(255, 204, 102, 0.3);
            backdrop-filter: blur(5px);
            z-index: 100;
            display: flex;
            align-items: center;
            gap: 10px;
            min-width: 200px;
            transition: all 0.3s;
        }
        
        .sudo-status.expiring {
            color: #ff6666;
            border-color: #ff6666;
            animation: pulse 1s infinite;
        }
        
        @keyframes pulse {
            0%, 100% { box-shadow: 0 0 25px rgba(255, 102, 102, 0.3); }
            50% { box-shadow: 0 0 40px rgba(255, 102, 102, 0.6); }
        }
        
        /* Время сессии */
        .time-status {
            position: fixed;
            bottom: 20px;
            left: 20px;
            background: rgba(42, 26, 90, 0.9);
            color: #66ff66;
            padding: 12px 18px;
            border-radius: 10px;
            font-size: 14px;
            border: 2px solid rgba(102, 255, 102, 0.4);
            box-shadow: 0 0 25px rgba(102, 255, 102, 0.3);
            backdrop-filter: blur(5px);
            z-index: 100;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        /* Предупреждение о таймауте */
        .timeout-warning {
            position: fixed;
            top: 20px;
            right: 20px;
            background: linear-gradient(135deg, #ffaa00, #ff6600);
            color: #000;
            padding: 15px 25px;
            border-radius: 10px;
            font-size: 15px;
            font-weight: bold;
            box-shadow: 0 0 30px rgba(255, 170, 0, 0.5);
            backdrop-filter: blur(5px);
            display: none;
            z-index: 1000;
            animation: shake 0.5s ease;
        }
        
        @keyframes shake {
            0%, 100% { transform: translateX(0); }
            25% { transform: translateX(-10px); }
            75% { transform: translateX(10px); }
        }
        
        /* Мобильная адаптация */
        @media (max-width: 768px) {
            .container {
                padding: 10px;
            }
            
            .terminal {
                height: 75vh;
            }
            
            .quick-commands {
                grid-template-columns: repeat(2, 1fr);
            }
            
            .sudo-status, .time-status {
                position: static;
                margin: 10px auto;
                width: calc(100% - 20px);
                max-width: 400px;
            }
            
            .sudo-status {
                margin-top: 20px;
            }
        }
    </style>
</head>
<body>
    <!-- Северное сияние -->
    <div class="aurora"></div>
    
    <!-- Горы -->
    <div class="mountains"></div>
    
    <!-- Снег -->
    <div class="snow" id="snow"></div>
    
    <?php if (!isAuthenticated()): ?>
        <!-- Форма входа -->
        <div class="container">
            <div class="login-form">
                <h2>Т
                
                
                
                
                ерминал Linux</h2>
                <?php if (isset($error)): ?>
                    <div class="error"><?php echo htmlspecialchars($error); ?></div>
                <?php endif; ?>
                <form method="POST" action="">
                    <div class="form-group">
                        <label for="username">❄️ Логин:</label>
                        <input type="text" id="username" name="username" required 
                               placeholder="root" autocomplete="off">
                    </div>
                    <div class="form-group">
                        <label for="password">❄️ Пароль:</label>
                        <input type="password" id="password" name="password" required 
                               placeholder="Введите пароль" autocomplete="off">
                    </div>
                    <button type="submit" name="login" class="btn">
                        ❄️ Войти в систему ❄️
                    </button>
                </form>
            </div>
        </div>
        
    <?php else: ?>
        <!-- Форма для sudo пароля -->
        <?php if ($show_sudo_form): ?>
        <div class="sudo-overlay" id="sudoOverlay">
            <div class="sudo-form">
                <h3>🔐 [sudo] пароль для <?php echo htmlspecialchars($_SESSION['username']); ?></h3>
                <p>Введите пароль пользователя Linux для выполнения команды с правами root</p>
                <?php if (isset($sudo_error)): ?>
                    <div class="error"><?php echo htmlspecialchars($sudo_error); ?></div>
                <?php endif; ?>
                <form method="POST" action="">
                    <div class="form-group">
                        <input type="password" name="sudo_password" 
                               placeholder="Пароль для [sudo]:" required autocomplete="off"
                               autofocus style="text-align: center; font-family: 'Consolas';">
                    </div>
                    <div style="display: flex; gap: 10px; margin-top: 20px;">
                        <button type="submit" name="sudo_password_submit" class="btn" style="flex: 1;">
                            ✅ Подтвердить
                        </button>
                        <button type="button" class="btn" style="background: linear-gradient(135deg, #666, #888); flex: 1;"
                                onclick="cancelSudo()">
                            ❌ Отмена
                        </button>
                    </div>
                </form>
                <p style="margin-top: 15px; font-size: 12px; color: #888; text-align: center;">
                    ⏱️ Sudo пароль будет действителен 5 минут
                </p>
            </div>
        </div>
        <?php endif; ?>
        
        <!-- Основной интерфейс -->
        <div class="container">
            <!-- Информация о системе -->
            <div class="system-info">
                <div class="info-grid">
                    <div class="info-item">
                        <div class="info-label">👤 Пользователь:</div> 
                        <span style="color: #b266ff;"><?php echo htmlspecialchars($_SESSION['username']); ?></span>
                    </div>
                    <div class="info-item">
                        <div class="info-label">📁 Директория:</div> 
                        <span style="color: #ffcc66;"><?php echo htmlspecialchars($current_dir_display); ?></span>
                    </div>
                    <div class="info-item">
                        <div class="info-label">⏰ Время сервера:</div> 
                        <?php echo date('H:i:s'); ?>
                    </div>
                    <div class="info-item">
                        <div class="info-label">💾 Свободно:</div> 
                        <?php 
                            $free = disk_free_space($_SESSION['current_dir']);
                            $total = disk_total_space($_SESSION['current_dir']);
                            $percent = $total > 0 ? round(($free / $total) * 100, 1) : 0;
                            echo round($free / 1024 / 1024 / 1024, 2) . ' GB (' . $percent . '%)';
                        ?>
                    </div>
                </div>
            </div>
            
            <!-- Быстрые команды -->
            <div class="quick-commands">
                <button class="quick-btn" onclick="setCommand('pwd')">📍 pwd</button>
                <button class="quick-btn" onclick="setCommand('ls -la')">📋 ls -la</button>
                <button class="quick-btn" onclick="setCommand('df -h')">💾 df -h</button>
                <button class="quick-btn" onclick="setCommand('free -h')">🧠 free -h</button>
                <button class="quick-btn" onclick="setCommand('sudo apt update')">🔄 apt update</button>
                <button class="quick-btn" onclick="setCommand('sudo apt upgrade -y')">⬆️ apt upgrade</button>
                <button class="quick-btn" onclick="setCommand('sudo reboot')">🔄 reboot</button>
                <button class="quick-btn" onclick="setCommand('clear')">✨ clear</button>
                <button class="quick-btn" onclick="setCommand('neofetch')">🖥️ neofetch</button>
                <button class="quick-btn" onclick="setCommand('htop')">📊 htop</button>
            </div>
            
            <!-- Терминал -->
            <div class="terminal">
                <div class="terminal-header">
                    <div class="terminal-title">
                        ❄️ Терминал Linux - Удаленное управление
                    </div>
                    <a href="?logout=1" class="logout-btn">❄️ Выход</a>
                </div>
                
                <div class="terminal-body">
                    <div class="current-dir">
                        📁 <?php echo htmlspecialchars($current_dir_display); ?>
                    </div>
                    
                    <?php if (!empty($output)): ?>
                        <div class="output">
                            <div class="prompt">
                                <span class="user"><?php echo htmlspecialchars($_SESSION['username']); ?>@server</span>:
                                <span class="path"><?php echo htmlspecialchars($current_dir_display); ?></span>$ 
                                <?php echo htmlspecialchars($command ?? ''); ?>
                            </div>
                            <div class="terminal-output"><?php echo $output; ?></div>
                        </div>
                    <?php endif; ?>
                    
                    <form method="POST" action="" id="commandForm">
                        <div class="command-input">
                            <span>
                                <span class="user"><?php echo htmlspecialchars($_SESSION['username']); ?>@server</span>:
                                <span class="path"><?php echo htmlspecialchars($current_dir_display); ?></span>$
                            </span>
                            <input type="text" name="command" id="command" 
                                   placeholder="Введите команду..." autocomplete="off" autofocus>
                        </div>
                    </form>
                </div>
            </div>
            
            <!-- Sudo статус -->
            <div class="sudo-status" id="sudoStatus">
                <?php 
                    if (checkSudoPassword()) {
                        $sudo_left = $config['sudo_timeout'] - (time() - $_SESSION['sudo_time']);
                        $minutes = ceil($sudo_left / 60);
                        echo "🔐 Sudo: {$minutes} мин";
                    } else {
                        echo "🔐 Sudo: требуется пароль";
                    }
                ?>
            </div>
            
            <!-- Время сессии -->
            <div class="time-status" id="timeStatus">
                <?php 
                    $session_left = $config['session_timeout'] - (time() - $_SESSION['last_activity']);
                    $hours = floor($session_left / 3600);
                    $minutes = floor(($session_left % 3600) / 60);
                    echo "⏰ Сессия: {$hours}ч {$minutes}м";
                ?>
            </div>
            
            <!-- Предупреждение о таймауте -->
            <div class="timeout-warning" id="timeoutWarning">
                ⚠️ Внимание! Сессия завершится через 5 минут из-за неактивности
            </div>
        </div>
        
        <script>
            // Создаем снежинки
            function createSnow() {
                const snowContainer = document.getElementById('snow');
                const snowflakeCount = 120;
                
                for (let i = 0; i < snowflakeCount; i++) {
                    const snowflake = document.createElement('div');
                    snowflake.className = 'snowflake';
                    
                    const size = Math.random() * 5 + 2;
                    snowflake.style.width = size + 'px';
                    snowflake.style.height = size + 'px';
                    snowflake.style.left = Math.random() * 100 + 'vw';
                    snowflake.style.top = Math.random() * -100 + 'px';
                    snowflake.style.opacity = Math.random() * 0.7 + 0.3;
                    
                    const duration = Math.random() * 10 + 10;
                    snowflake.style.animationDuration = duration + 's';
                    snowflake.style.animationDelay = Math.random() * 10 + 's';
                    
                    snowContainer.appendChild(snowflake);
                }
            }
            
            // Автофокус на поле ввода команды
            document.getElementById('command').focus();
            
            // Прокрутка вниз терминала
            const terminalBody = document.querySelector('.terminal-body');
            terminalBody.scrollTop = terminalBody.scrollHeight;
            
            // Установка команды
            function setCommand(cmd) {
                document.getElementById('command').value = cmd;
                document.getElementById('command').focus();
            }
            
            // Отмена sudo
            function cancelSudo() {
                window.location.href = '?';
            }
            
            // Автоподтверждение формы при нажатии Enter
            document.getElementById('command').addEventListener('keydown', function(e) {
                if (e.key === 'Enter') {
                    e.preventDefault();
                    document.getElementById('commandForm').submit();
                }
            });
            
            // Обновление статусов времени
            function updateStatuses() {
                const sudoStatus = document.getElementById('sudoStatus');
                const timeStatus = document.getElementById('timeStatus');
                const timeoutWarning = document.getElementById('timeoutWarning');
                
                // Обновляем время сессии
                <?php
                    $session_left = $config['session_timeout'] - (time() - $_SESSION['last_activity']);
                    $sudo_left = checkSudoPassword() ? $config['sudo_timeout'] - (time() - $_SESSION['sudo_time']) : 0;
                ?>
                
                let sessionLeft = <?php echo $session_left; ?>;
                let sudoLeft = <?php echo $sudo_left; ?>;
                
                // Обновление времени сессии
                let sessionHours = Math.floor(sessionLeft / 3600);
                let sessionMinutes = Math.floor((sessionLeft % 3600) / 60);
                if (timeStatus) {
                    timeStatus.innerHTML = `⏰ Сессия: ${sessionHours}ч ${sessionMinutes}м`;
                    
                    // Предупреждение за 5 минут
                    if (sessionLeft < 300 && sessionLeft > 0) {
                        if (!timeoutWarning.classList.contains('show')) {
                            timeoutWarning.style.display = 'block';
                            timeoutWarning.classList.add('show');
                        }
                    }
                }
                
                // Обновление sudo статуса
                if (sudoStatus) {
                    if (sudoLeft > 0) {
                        let sudoMinutes = Math.ceil(sudoLeft / 60);
                        sudoStatus.innerHTML = `🔐 Sudo: ${sudoMinutes} мин`;
                        
                        // Мигающее предупреждение за 1 минуту до окончания sudo
                        if (sudoLeft < 60) {
                            sudoStatus.classList.add('expiring');
                        } else {
                            sudoStatus.classList.remove('expiring');
                        }
                    } else {
                        sudoStatus.innerHTML = `🔐 Sudo: требуется пароль`;
                        sudoStatus.classList.remove('expiring');
                    }
                }
                
                // Автоматический выход при таймауте
                if (sessionLeft <= 0) {
                    window.location.href = '?logout=1';
                }
            }
            
            // Инициализация
            window.onload = function() {
                createSnow();
                updateStatuses();
                setInterval(updateStatuses, 60000); // Обновляем каждую минуту
                
                // Автофокус на sudo поле если оно есть
                const sudoPasswordField = document.querySelector('input[name="sudo_password"]');
                if (sudoPasswordField) {
                    sudoPasswordField.focus();
                }
            };
            
            // Быстрое обновление времени каждую секунду
            setInterval(function() {
                const now = new Date();
                const timeStr = now.toLocaleTimeString('ru-RU');
                // Можно добавить отображение текущего времени где-нибудь
            }, 1000);
        </script>
    <?php endif; ?>
</body>
</html>
