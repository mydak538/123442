<?php
// Конфигурация
$site_title = "WexIB BIOS";
$version = "4.51";
$github_main = "https://github.com/mydak538/WexIB";
$github_old = "https://github.com/mydak538/WexIB-1.00";
$github_profile = "https://github.com/mydak538";

// Массив файлов для скачивания
$files = [
    [
        'name' => 'Исходный код v1.53',
        'filename' => 'SourceCode-WexIB-1.53.zip',
        'size' => '24.5 MB',
        'version' => '1.53',
        'type' => 'source'
    ],
    [
        'name' => 'Образ BIOS',
        'filename' => 'bios.img',
        'size' => '16.0 MB',
        'version' => '4.51',
        'type' => 'image'
    ]
];

// Массив картинок
$images = [
    '001.png' => 'Главный экран BIOS',
    '002.png' => 'Настройки разгона',
    '003.png' => 'Меню восстановления',
    '004.png' => 'Информация о системе'
];

// Обработка скачивания файлов
if (isset($_GET['download'])) {
    $filename = basename($_GET['download']);
    $filepath = __DIR__ . '/downloads/' . $filename;
    
    if (file_exists($filepath)) {
        header('Content-Description: File Transfer');
        header('Content-Type: application/octet-stream');
        header('Content-Disposition: attachment; filename="' . $filename . '"');
        header('Expires: 0');
        header('Cache-Control: must-revalidate');
        header('Pragma: public');
        header('Content-Length: ' . filesize($filepath));
        readfile($filepath);
        exit;
    } else {
        header("HTTP/1.0 404 Not Found");
        die("Файл не найден");
    }
}

// Лицензия The Unlicense (полная версия)
$license_text = <<<LICENSE
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
LICENSE;
?>
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?php echo $site_title; ?></title>
    <style>
        :root {
            --bg-primary: #0a0a0a;
            --bg-secondary: #111111;
            --text-primary: #00ff00;
            --text-secondary: #00cc00;
            --text-muted: #008800;
            --border-color: #00aa00;
            --danger: #ff3300;
            --warning: #ffaa00;
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Courier New', monospace;
            background-color: var(--bg-primary);
            color: var(--text-primary);
            line-height: 1.6;
            padding: 0;
            margin: 0;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        
        /* Header */
        .header {
            background-color: var(--bg-secondary);
            border-bottom: 3px solid var(--border-color);
            padding: 20px 0;
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            font-size: 2.5em;
            text-transform: uppercase;
            letter-spacing: 3px;
            margin-bottom: 10px;
            text-shadow: 0 0 10px var(--text-primary);
        }
        
        .header .version {
            color: var(--text-secondary);
            font-size: 1.2em;
        }
        
        /* Navigation */
        .nav {
            display: flex;
            justify-content: center;
            flex-wrap: wrap;
            gap: 15px;
            margin: 30px 0;
            padding: 20px;
            background-color: var(--bg-secondary);
            border: 1px solid var(--border-color);
        }
        
        .nav a {
            color: var(--text-primary);
            text-decoration: none;
            padding: 10px 20px;
            border: 1px solid var(--border-color);
            transition: all 0.3s;
            font-weight: bold;
        }
        
        .nav a:hover {
            background-color: var(--border-color);
            color: var(--bg-primary);
        }
        
        /* Sections */
        .section {
            margin: 40px 0;
            padding: 20px;
            background-color: var(--bg-secondary);
            border: 1px solid var(--border-color);
        }
        
        .section h2 {
            color: var(--text-primary);
            border-bottom: 2px solid var(--border-color);
            padding-bottom: 10px;
            margin-bottom: 20px;
            font-size: 1.8em;
        }
        
        /* Files */
        .files-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 20px;
            margin-top: 20px;
        }
        
        .file-card {
            background-color: var(--bg-primary);
            border: 1px solid var(--border-color);
            padding: 20px;
            transition: transform 0.3s;
        }
        
        .file-card:hover {
            transform: translateY(-5px);
            border-color: var(--text-primary);
        }
        
        .file-card h3 {
            color: var(--text-primary);
            margin-bottom: 10px;
        }
        
        .file-info {
            color: var(--text-muted);
            font-size: 0.9em;
            margin-bottom: 15px;
        }
        
        .btn-download {
            display: inline-block;
            background-color: var(--border-color);
            color: var(--bg-primary);
            padding: 8px 16px;
            text-decoration: none;
            border: none;
            cursor: pointer;
            font-weight: bold;
            transition: background-color 0.3s;
        }
        
        .btn-download:hover {
            background-color: var(--text-primary);
        }
        
        /* Gallery */
        .gallery-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-top: 20px;
        }
        
        .gallery-item {
            background-color: var(--bg-primary);
            border: 1px solid var(--border-color);
            padding: 10px;
            text-align: center;
        }
        
        .gallery-item img {
            max-width: 100%;
            height: 200px;
            object-fit: cover;
            border: 1px solid var(--border-color);
        }
        
        .gallery-item p {
            margin-top: 10px;
            color: var(--text-secondary);
            font-size: 0.9em;
        }
        
        /* License */
        .license-box {
            background-color: var(--bg-primary);
            border: 1px solid var(--border-color);
            padding: 20px;
            max-height: 400px;
            overflow-y: auto;
            font-size: 0.9em;
            line-height: 1.8;
            white-space: pre-wrap;
        }
        
        /* Error Guide */
        .error-box {
            background-color: var(--bg-primary);
            border: 2px solid var(--danger);
            padding: 20px;
            margin: 20px 0;
        }
        
        .error-box h3 {
            color: var(--danger);
            margin-bottom: 15px;
        }
        
        .steps {
            margin-left: 20px;
            margin-top: 20px;
        }
        
        .steps li {
            margin-bottom: 15px;
        }
        
        /* Warning */
        .warning {
            background-color: #331100;
            border: 2px solid var(--warning);
            padding: 20px;
            margin: 20px 0;
            color: var(--warning);
        }
        
        /* Links */
        .links-list {
            list-style-type: none;
            padding-left: 20px;
        }
        
        .links-list li {
            margin-bottom: 10px;
        }
        
        .links-list a {
            color: var(--text-primary);
            text-decoration: none;
            border-bottom: 1px dashed var(--border-color);
        }
        
        .links-list a:hover {
            color: var(--text-secondary);
        }
        
        /* Footer */
        .footer {
            text-align: center;
            padding: 30px 0;
            margin-top: 50px;
            border-top: 1px solid var(--border-color);
            color: var(--text-muted);
            font-size: 0.9em;
        }
        
        /* Responsive */
        @media (max-width: 768px) {
            .nav {
                flex-direction: column;
                align-items: center;
            }
            
            .nav a {
                width: 100%;
                text-align: center;
            }
            
            .header h1 {
                font-size: 2em;
            }
        }
        
        /* Terminal effect */
        .blink {
            animation: blink 1s infinite;
        }
        
        @keyframes blink {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header">
            <h1><?php echo $site_title; ?></h1>
            <div class="version">Версия <?php echo $version; ?></div>
            <div class="blink">Custom BIOS Firmware</div>
        </div>
        
        <!-- Navigation -->
        <div class="nav">
            <a href="#license">The Unlicense</a>
            <a href="#files">Файлы</a>
            <a href="#error">Ошибка Disk Error</a>
            <a href="#gallery">Галерея</a>
            <a href="#links">Ссылки</a>
        </div>
        
        <!-- Warning -->
        <div class="warning">
            ⚠ ВНИМАНИЕ: Прошивка BIOS может повредить материнскую плату!<br>
            Все действия выполняются на ваш страх и риск.
        </div>
        
        <!-- License Section -->
        <section id="license" class="section">
            <h2>THE UNLICENSE</h2>
            <div class="license-box">
                <?php echo htmlspecialchars($license_text); ?>
            </div>
        </section>
        
        <!-- Files Section -->
        <section id="files" class="section">
            <h2>ФАЙЛЫ ДЛЯ СКАЧИВАНИЯ</h2>
            <div class="files-grid">
                <?php foreach ($files as $file): ?>
                <div class="file-card">
                    <h3><?php echo $file['name']; ?></h3>
                    <div class="file-info">
                        Версия: <?php echo $file['version']; ?><br>
                        Размер: <?php echo $file['size']; ?><br>
                        Тип: <?php echo $file['type']; ?>
                    </div>
                    <?php if (file_exists(__DIR__ . '/downloads/' . $file['filename'])): ?>
                        <a href="?download=<?php echo urlencode($file['filename']); ?>" class="btn-download">
                            📥 Скачать
                        </a>
                    <?php else: ?>
                        <a href="<?php echo $github_main; ?>" class="btn-download" target="_blank">
                            📥 Скачать с GitHub
                        </a>
                    <?php endif; ?>
                </div>
                <?php endforeach; ?>
            </div>
        </section>
        
        <!-- Error Guide Section -->
        <section id="error" class="section">
            <h2>ОШИБКА "DISK ERROR!"</h2>
            <div class="error-box">
                <h3>Disk Error! The main BIOS firmware is damaged or not found!</h3>
                <p>Try to reboot if it doesn't work then reflash to an older BIOS firmware current version <?php echo $version; ?></p>
                
                <h3>Решение проблемы:</h3>
                <ol class="steps">
                    <li><strong>Перезагрузка:</strong> Нажмите Ctrl+Alt+Delete для перезагрузки</li>
                    <li><strong>Подготовка флешки:</strong> Отформатируйте USB-флешку в FAT32</li>
                    <li><strong>Скачивание прошивки:</strong> Скачайте файл прошивки с GitHub</li>
                    <li><strong>Копирование:</strong> Скопируйте файл на флешку</li>
                    <li><strong>Восстановление:</strong> Вставьте флешку в порт материнской платы и включите ПК</li>
                    <li><strong>Ожидание:</strong> Дождитесь автоматического восстановления (2-5 минут)</li>
                </ol>
            </div>
        </section>
        
        <!-- Gallery Section -->
        <section id="gallery" class="section">
            <h2>ГАЛЕРЕЯ</h2>
            <div class="gallery-grid">
                <?php foreach ($images as $img => $desc): ?>
                <div class="gallery-item">
                    <?php 
                    $img_path = __DIR__ . '/images/' . $img;
                    if (file_exists($img_path)): 
                    ?>
                        <img src="data:image/png;base64,<?php echo base64_encode(file_get_contents($img_path)); ?>" 
                             alt="<?php echo $desc; ?>">
                    <?php else: ?>
                        <div style="background:#111; height:200px; display:flex; align-items:center; justify-content:center; border:1px solid var(--border-color);">
                            [<?php echo $img; ?>]
                        </div>
                    <?php endif; ?>
                    <p><?php echo $desc; ?></p>
                </div>
                <?php endforeach; ?>
            </div>
        </section>
        
        <!-- Links Section -->
        <section id="links" class="section">
            <h2>ССЫЛКИ</h2>
            <ul class="links-list">
                <li>📁 <a href="<?php echo $github_main; ?>" target="_blank">Основной репозиторий WexIB</a></li>
                <li>📁 <a href="<?php echo $github_old; ?>" target="_blank">WexIB версия 1.00</a></li>
                <li>👤 <a href="<?php echo $github_profile; ?>" target="_blank">Профиль mydak538 на GitHub</a></li>
                <li>📄 <a href="https://unlicense.org" target="_blank">Официальный сайт The Unlicense</a></li>
            </ul>
        </section>
        
        <!-- Footer -->
        <div class="footer">
            <div>WexIB BIOS • The Unlicense • Public Domain</div>
            <div><?php echo $github_main; ?></div>
            <div style="margin-top: 10px; font-size: 0.8em;">
                Серверное время: <?php echo date('Y-m-d H:i:s'); ?>
            </div>
        </div>
    </div>
    
    <script>
        // Плавная прокрутка
        document.querySelectorAll('.nav a').forEach(anchor => {
            anchor.addEventListener('click', function(e) {
                e.preventDefault();
                const targetId = this.getAttribute('href');
                const targetElement = document.querySelector(targetId);
                
                if (targetElement) {
                    window.scrollTo({
                        top: targetElement.offsetTop - 20,
                        behavior: 'smooth'
                    });
                    
                    // Обновляем URL без перезагрузки
                    history.pushState(null, null, targetId);
                }
            });
        });
        
        // Подсветка активного раздела при прокрутке
        window.addEventListener('scroll', function() {
            const sections = document.querySelectorAll('.section');
            const navLinks = document.querySelectorAll('.nav a');
            
            let current = '';
            sections.forEach(section => {
                const sectionTop = section.offsetTop;
                const sectionHeight = section.clientHeight;
                if (scrollY >= (sectionTop - 100)) {
                    current = section.getAttribute('id');
                }
            });
            
            navLinks.forEach(link => {
                link.classList.remove('active');
                if (link.getAttribute('href') === '#' + current) {
                    link.classList.add('active');
                }
            });
        });
        
        // Эффект мигающего курсора
        const blinkElement = document.querySelector('.blink');
        if (blinkElement) {
            setInterval(() => {
                blinkElement.style.opacity = blinkElement.style.opacity === '0.5' ? '1' : '0.5';
            }, 500);
        }
    </script>
</body>
</html>
